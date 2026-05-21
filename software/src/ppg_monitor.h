// ============================================================================
// ppg_monitor.h — Photoplethysmography sampling + heart-rate detection
// ----------------------------------------------------------------------------
// WHAT THIS MODULE DOES
//   - Samples the analogue PPG sensor on GPIO A0 at exactly 250 Hz
//   - Removes DC drift with a 0.5 Hz first-order high-pass filter
//   - Band-limits to 5 Hz with a 2nd-order Butterworth low-pass filter
//   - Detects R-peaks with an adaptive-threshold algorithm
//   - Reports BPM (median of last 5 IBIs) and the latest filtered samples
//
// WHY THE TIMER ISR ONLY SETS A FLAG
//   ESP32-S3 calls to analogRead() acquire a FreeRTOS semaphore and may
//   block waiting for the ADC peripheral. Doing that inside an interrupt
//   service routine causes "Guru Meditation" panics (cache disabled while
//   accessing flash). The accepted pattern is: ISR sets a flag, main
//   loop polls the flag, ADC read happens in normal task context.
// ============================================================================
#pragma once
#include <Arduino.h>
#include "config.h"

class PPGMonitor {
public:
    PPGMonitor();
    void begin();

    /** Roep aan vanuit loop() — leest ADC + verwerkt sample als de timer hem 'due' heeft gezet. */
    void poll();

    uint16_t getBPM() const;
    uint16_t getIBImillis() const;
    uint32_t getLastPeakMillis() const { return _last_peak_ms; }
    bool     peakDetectedThisLoop();
    int16_t  getLatestRawSample() const;
    uint16_t getLatestADC() const { return _latest_adc; }

    /** Kopieer de meest recente N samples naar out[] (chronologische volgorde). */
    void getRecentSamples(int16_t* out, size_t n);

private:
    static void IRAM_ATTR _timerISR();
    static PPGMonitor* _instance;

    float _hp_prev_in, _hp_prev_out;
    float _lp_state[2];

    float    _envelope;
    bool     _above_threshold;
    uint32_t _last_peak_ms;
    uint16_t _ibi_ms;
    uint16_t _ibi_buffer[5];
    uint8_t  _ibi_index;
    uint16_t _bpm;

    static constexpr uint16_t SAMPLE_BUFFER_SIZE = 128;
    volatile int16_t  _samples[SAMPLE_BUFFER_SIZE];
    volatile uint16_t _sample_head;

    volatile int16_t  _latest_filtered;
    volatile uint16_t _latest_adc;

    volatile bool _peak_detected_flag;
    volatile bool _sample_pending;     // gezet door ISR, gewist door poll()

    void _processSample();
    void _updateBPM();
};
