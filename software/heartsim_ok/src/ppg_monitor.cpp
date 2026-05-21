// ============================================================================
// ppg_monitor.cpp — Implementation of PPG sampling and heart-rate detection
// ----------------------------------------------------------------------------
// SIGNAL CHAIN
//   raw ADC sample  →  first-order HP (0.5 Hz)  →  2nd-order Butterworth LP
//                  (DC removal)               (5 Hz, in-band)
//                  →  adaptive-threshold peak detector  →  IBI + BPM
//
// HIGH-PASS — first order recursive (alpha pole)
//   alpha = exp(-2π·fc/fs)   with fc = 0.5 Hz, fs = 250 Hz  →  α ≈ 0.9874
//   y[n] = α · (y[n-1] + x[n] - x[n-1])
//   Removes the DC pedestal that the PPG sensor outputs (~1.5 V baseline).
//
// LOW-PASS — 2nd-order Butterworth, fc = 5 Hz @ fs = 250 Hz
//   Coefficients precomputed with scipy.signal.butter(2, 5/(250/2)):
//     b = [0.00362168, 0.00724336, 0.00362168]
//     a = [1,         -1.82269493, 0.83718165]
//   Direct Form II Transposed implementation (only 2 state variables).
//   Keeps the heart-rate band (40-180 BPM = 0.67-3.0 Hz) and removes
//   higher-frequency noise + harmonics.
//
// PEAK DETECTION — adaptive amplitude threshold
//   We track an "envelope" that follows the signal: it climbs instantly
//   when |sample| exceeds the current envelope, and decays geometrically
//   (α=0.995) otherwise. The detection threshold is 50 % of the envelope.
//   A refractory period (PPG_REFRACTORY_MS) prevents the dicrotic notch
//   from being counted as a separate beat.
//
// SAFETY NOTE
//   analogRead() is NOT safe inside an ISR on ESP32-S3 (acquires a FreeRTOS
//   semaphore that may not be available in interrupt context). The hardware
//   timer ISR therefore only sets a flag (_sample_pending); the actual ADC
//   read happens in poll(), which runs from the main loop.
// ============================================================================
#include "ppg_monitor.h"

PPGMonitor* PPGMonitor::_instance = nullptr;
static hw_timer_t* s_timer = nullptr;
static portMUX_TYPE s_mux = portMUX_INITIALIZER_UNLOCKED;

static constexpr float LP_B0 = 0.00362168f;
static constexpr float LP_B1 = 0.00724336f;
static constexpr float LP_B2 = 0.00362168f;
static constexpr float LP_A1 = -1.82269493f;
static constexpr float LP_A2 = 0.83718165f;
static constexpr float HP_ALPHA = 0.9874f;

PPGMonitor::PPGMonitor()
    : _hp_prev_in(0), _hp_prev_out(0),
      _envelope(0), _above_threshold(false),
      _last_peak_ms(0), _ibi_ms(0), _ibi_index(0), _bpm(0),
      _sample_head(0), _latest_filtered(0), _latest_adc(0),
      _peak_detected_flag(false), _sample_pending(false) {
    _lp_state[0] = _lp_state[1] = 0;
    for (uint8_t i = 0; i < 5; i++) _ibi_buffer[i] = 0;
    for (uint16_t i = 0; i < SAMPLE_BUFFER_SIZE; i++) _samples[i] = 0;
}

void PPGMonitor::begin() {
    _instance = this;

    analogReadResolution(12);
    pinMode(PPG_ADC_PIN, INPUT);

    // Hardware timer @ 250 Hz — ISR zet enkel _sample_pending
    s_timer = timerBegin(0, 80, true);             // 1 MHz tick
    timerAttachInterrupt(s_timer, &PPGMonitor::_timerISR, true);
    timerAlarmWrite(s_timer, 1000000UL / PPG_SAMPLE_RATE, true);
    timerAlarmEnable(s_timer);

    Serial.printf("[PPG] Sampling op %lu Hz gestart\n", (unsigned long)PPG_SAMPLE_RATE);
}

void IRAM_ATTR PPGMonitor::_timerISR() {
    if (_instance) {
        _instance->_sample_pending = true;          // ALLEEN dit — kort en veilig
    }
}

void PPGMonitor::poll() {
    if (!_sample_pending) return;
    _sample_pending = false;
    _processSample();
}

void PPGMonitor::_processSample() {
    uint16_t adc = analogRead(PPG_ADC_PIN);         // Veilig — buiten ISR
    _latest_adc = adc;

    float x = (float)adc;
    float hp_out = HP_ALPHA * (_hp_prev_out + x - _hp_prev_in);
    _hp_prev_in = x;
    _hp_prev_out = hp_out;

    float w = hp_out - LP_A1 * _lp_state[0] - LP_A2 * _lp_state[1];
    float lp_out = LP_B0 * w + LP_B1 * _lp_state[0] + LP_B2 * _lp_state[1];
    _lp_state[1] = _lp_state[0];
    _lp_state[0] = w;

    int16_t filtered = (int16_t)lp_out;
    _latest_filtered = filtered;

    portENTER_CRITICAL(&s_mux);
    _samples[_sample_head] = filtered;
    _sample_head = (_sample_head + 1) % SAMPLE_BUFFER_SIZE;
    portEXIT_CRITICAL(&s_mux);

    float abs_val = fabsf(lp_out);
    if (abs_val > _envelope) _envelope = abs_val;
    else                     _envelope *= 0.995f;

    float threshold = 0.5f * _envelope;
    uint32_t now = millis();
    uint32_t since_last_peak = now - _last_peak_ms;

    if (lp_out > threshold && !_above_threshold &&
        since_last_peak > PPG_REFRACTORY_MS) {
        _above_threshold = true;
        if (_last_peak_ms > 0) {
            _ibi_ms = since_last_peak;
            _ibi_buffer[_ibi_index] = _ibi_ms;
            _ibi_index = (_ibi_index + 1) % 5;
            _updateBPM();
        }
        _last_peak_ms = now;
        _peak_detected_flag = true;
    } else if (lp_out < 0.3f * threshold) {
        _above_threshold = false;
    }
}

void PPGMonitor::_updateBPM() {
    uint16_t sorted[5];
    memcpy(sorted, (const void*)_ibi_buffer, sizeof(sorted));
    for (uint8_t i = 1; i < 5; i++) {
        uint16_t key = sorted[i];
        int8_t j = i - 1;
        while (j >= 0 && sorted[j] > key) {
            sorted[j+1] = sorted[j];
            j--;
        }
        sorted[j+1] = key;
    }
    uint16_t median = sorted[2];
    if (median > 0) _bpm = 60000 / median;
}

bool PPGMonitor::peakDetectedThisLoop() {
    bool detected = false;
    portENTER_CRITICAL(&s_mux);
    if (_peak_detected_flag) {
        detected = true;
        _peak_detected_flag = false;
    }
    portEXIT_CRITICAL(&s_mux);
    return detected;
}

uint16_t PPGMonitor::getBPM() const { return _bpm; }
uint16_t PPGMonitor::getIBImillis() const { return _ibi_ms; }
int16_t  PPGMonitor::getLatestRawSample() const { return _latest_filtered; }

void PPGMonitor::getRecentSamples(int16_t* out, size_t n) {
    if (n > SAMPLE_BUFFER_SIZE) n = SAMPLE_BUFFER_SIZE;
    portENTER_CRITICAL(&s_mux);
    uint16_t head = _sample_head;
    // Chronologische volgorde: oudste eerst, nieuwste laatst
    for (size_t i = 0; i < n; i++) {
        uint16_t idx = (head + SAMPLE_BUFFER_SIZE - n + i) % SAMPLE_BUFFER_SIZE;
        out[i] = _samples[idx];
    }
    portEXIT_CRITICAL(&s_mux);
}
