// ============================================================================
// cycle_engine.cpp — Implementation of the cardiac cycle scheduler
// ----------------------------------------------------------------------------
// PULSE SCHEDULING — _scheduleCycleEvents()
//   Called once per cardiac cycle. Computes systole duration from the IBI
//   then queues haptic pulses with millisecond offsets:
//     S1  → time 0,                duration 60 ms
//     S2  → time systole_ms,       duration 50 ms
//     Extras (depending on pathology):
//       S3 Gallop  → systole_ms + 0.15·IBI
//       S4 Gallop  → IBI − 80 ms (presystolic)
//       Mitral Stenosis → systole_ms + 80 ms (opening snap)
//       AS / MR / AR → 3 small tactile pulses spread across the murmur
//                       window (so the audio-less haptic feel still
//                       conveys "something extra is happening")
//
// HAPTIC TASK — _hapticTaskLoop()
//   Walks the _pulses[] array every 2 ms and fires any pulse whose
//   fire_at_ms has elapsed. Pulses are issued via HapticManager.pulse(),
//   which uses a blocking delay() to keep the motor energised for the
//   required duration. Running in its own task means that delay() never
//   blocks the main loop or the audio task.
//
// PPG PEAK HANDLING — _handlePPGPeak()
//   On every detected fingertip peak we update the rolling IBI history
//   and schedule the predicted next cardiac S1 (peak_ms + advance_ms,
//   where advance_ms = avg_IBI − ptt_offset). Three fallback paths handle
//   edge cases: very fast IBI (advance ≤ 0 → fire now), early new peak
//   while a prediction is still pending (cancel and refire), and the
//   first peak after startup (no history yet → no schedule, just record).
// ============================================================================
#include "cycle_engine.h"

CycleEngine::CycleEngine(PPGMonitor* ppg, AudioEngine* audio, HapticManager* haptic)
    : _ppg(ppg), _audio(audio), _haptic(haptic),
      _pathology(PATH_NORMAL), _severity(3),
      _running(false), _playback_mode(false),
      _last_playback_beat_ms(0), _playback_ibi_ms(857),  // 70 BPM
      _last_r_peak_ms(0), _last_ibi_ms(800),
      _ibi_history_idx(0), _avg_ibi_ms(800),
      _ptt_offset_ms(PPG_PTT_OFFSET_MS),
      _trigger_pending(false), _predicted_trigger_at_ms(0),
      _haptic_task(nullptr) {
    for (uint8_t i = 0; i < MAX_PULSES; i++) {
        _pulses[i].pending = false;
    }
    // Pre-fill history with 75 BPM so avg is valid before any peaks arrive
    for (uint8_t i = 0; i < IBI_HISTORY_LEN; i++) {
        _ibi_history[i] = 800;
    }
}

void CycleEngine::begin() {
    _pulse_mutex = xSemaphoreCreateMutex();

    // Start haptiek task op core 0 (lage prio — geen kritisch timing-budget)
    xTaskCreatePinnedToCore(
        _hapticTaskTrampoline,
        "haptic_task",
        2048,
        this,
        2,                  // lage priority
        &_haptic_task,
        CORE_SENSOR
    );
}

void CycleEngine::start() {
    _running = true;
    _audio->start();
    _last_playback_beat_ms = millis();
    Serial.println("[CYCLE] Simulatie gestart");
}

void CycleEngine::stop() {
    _running = false;
    _trigger_pending = false;
    _audio->stop();
    // Wis alle gepende pulses
    xSemaphoreTake(_pulse_mutex, portMAX_DELAY);
    for (uint8_t i = 0; i < MAX_PULSES; i++) {
        _pulses[i].pending = false;
    }
    xSemaphoreGive(_pulse_mutex);
    Serial.println("[CYCLE] Simulatie gestopt");
}

void CycleEngine::setPathology(PathologyId p, uint8_t severity) {
    _pathology = p;
    _severity  = constrain(severity, 1, 5);

    _audio->synthesizer()->setPathology(p, _severity);
    _audio->setSpatialWeights(PATHOLOGY_WEIGHTS[p]);

    Serial.printf("[CYCLE] Pathologie: %s (severity %u)\n",
                  pathologyName(p), _severity);
}

void CycleEngine::setPlaybackMode(bool enabled) {
    _playback_mode = enabled;
    _last_playback_beat_ms = millis();
    Serial.printf("[CYCLE] Playback mode %s\n", enabled ? "AAN" : "UIT");
}

void CycleEngine::setPlaybackBPM(uint16_t bpm) {
    if (bpm < 30)  bpm = 30;
    if (bpm > 200) bpm = 200;
    _playback_bpm    = bpm;
    _playback_ibi_ms = 60000UL / bpm;
    Serial.printf("[CYCLE] Playback BPM: %u (IBI %u ms)\n", _playback_bpm, _playback_ibi_ms);
}

void CycleEngine::setBaselineIntensity(uint8_t v) {
    if (v > 10) v = 10;
    _baseline_intensity = v;
    Serial.printf("[CYCLE] Baseline intensity: %u/10\n", v);
}

void CycleEngine::update() {
    if (!_running) return;

    uint32_t now = millis();

    // ---- Mode 1: Playback (geen sensor) — PTT-correctie niet van toepassing ----
    if (_playback_mode) {
        if (now - _last_playback_beat_ms >= _playback_ibi_ms) {
            _last_playback_beat_ms = now;
            _onRPeak(_playback_ibi_ms);
        }
        return;
    }

    // ---- Mode 2: Live PPG met PTT-correctie ----

    // Stap 1: Controleer of de geplande voorspelde trigger moet vuren.
    // Cast naar int32_t zodat negatieve delta (nog niet time) correct werkt.
    if (_trigger_pending &&
        (int32_t)(now - _predicted_trigger_at_ms) >= 0) {
        _trigger_pending = false;
        _onRPeak(_avg_ibi_ms);
    }

    // Stap 2: Verwerk een nieuwe PPG-piek indien aanwezig.
    if (_ppg->peakDetectedThisLoop()) {
        _handlePPGPeak(_ppg->getLastPeakMillis());
    }
}

void CycleEngine::_onRPeak(uint16_t ibi_ms) {
    _audio->synthesizer()->triggerNewCycle(ibi_ms);
    _scheduleCycleEvents(ibi_ms);
}

// ============================================================================
// PTT-correctie: verwerk een gemeten PPG-piek en plan de volgende S1-trigger
// ============================================================================
void CycleEngine::_handlePPGPeak(uint32_t peak_ms) {
    // Update IBI-meting en rolling-average
    if (_last_r_peak_ms > 0) {
        uint16_t new_ibi = (uint16_t)constrain(
            (int32_t)(peak_ms - _last_r_peak_ms), 300, 2000);
        _last_ibi_ms = new_ibi;
        _ibi_history[_ibi_history_idx] = new_ibi;
        _ibi_history_idx = (_ibi_history_idx + 1) % IBI_HISTORY_LEN;
        _updateAvgIBI();
    }
    _last_r_peak_ms = peak_ms;

    // advance_ms = hoeveel eerder dan de volgende PPG-piek de echte S1 optreedt
    int32_t advance_ms = (int32_t)_avg_ibi_ms - (int32_t)_ptt_offset_ms;

    if (advance_ms <= 0) {
        // Fallback: IBI is kleiner dan PTT-offset (zeer snelle hartslag of
        // hoge instelling). Trigger onmiddellijk, geen vooruitlopen mogelijk.
        _trigger_pending = false;
        _onRPeak(_avg_ibi_ms);
        return;
    }

    if (_trigger_pending) {
        int32_t ms_until = (int32_t)(_predicted_trigger_at_ms - peak_ms);
        if (ms_until > 0) {
            // De vorige voorspelde trigger heeft nog niet gevuurd, maar een
            // nieuwe piek is al binnengekomen — het interval was korter dan
            // verwacht (aritmie / plotse versnelling).
            // Fallback: annuleer de voorspelling en trigger direct.
            _trigger_pending = false;
            _onRPeak(_avg_ibi_ms);
            // Daarna meteen volgende cyclus plannen (zie hieronder)
        }
        // Als ms_until <= 0: de trigger heeft al gevuurd. Gewoon doorlopen
        // naar het plannen van de volgende.
    }

    // Plan de volgende S1: next_PPG_peak − PTT  ≈  now + (avg_ibi − ptt_offset)
    _trigger_pending = true;
    _predicted_trigger_at_ms = peak_ms + (uint32_t)advance_ms;
}

void CycleEngine::_updateAvgIBI() {
    uint32_t sum = 0;
    for (uint8_t i = 0; i < IBI_HISTORY_LEN; i++) {
        sum += _ibi_history[i];
    }
    _avg_ibi_ms = (uint16_t)(sum / IBI_HISTORY_LEN);
}

void CycleEngine::setPTTOffset(uint16_t ms) {
    _ptt_offset_ms = (uint16_t)constrain((int)ms, 0, 400);
    // Reschedule pending trigger with new offset if one is waiting
    if (_trigger_pending && _last_r_peak_ms > 0) {
        int32_t advance_ms = (int32_t)_avg_ibi_ms - (int32_t)_ptt_offset_ms;
        if (advance_ms > 0) {
            _predicted_trigger_at_ms = _last_r_peak_ms + (uint32_t)advance_ms;
        } else {
            _trigger_pending = false;
        }
    }
    Serial.printf("[CYCLE] PTT offset: %u ms\n", _ptt_offset_ms);
}

void CycleEngine::_scheduleCycleEvents(uint16_t ibi_ms) {
    // Bereken systolische duur (zelfde formule als in synth)
    uint16_t systole_ms = (uint16_t)(SYSTOLE_FRACTION * ibi_ms);
    if (systole_ms < 200) systole_ms = 200;
    if (systole_ms > 350) systole_ms = 350;

    const uint8_t* weights = PATHOLOGY_WEIGHTS[_pathology];

    // ===== Baseline-haptic (gewone hartslag S1+S2) =====
    // Alleen schedulen als de student de baseline AAN heeft op de HMI.
    // Intensiteit-knop (0..10) schaalt de basis-amplitude.
    if (_baseline_enabled && _baseline_intensity > 0) {
        float base_scale = (_baseline_intensity / 10.0f);  // 0.0 .. 1.0

        // S1: NU, alle modules, gewogen volgens pathologie
        for (uint8_t m = 0; m < NUM_MODULES; m++) {
            float intensity = (weights[m] / 255.0f) * 0.85f * base_scale;
            _schedulePulse(m, intensity, 60, 0);
        }
        // S2: op systole_ms, iets zachter
        for (uint8_t m = 0; m < NUM_MODULES; m++) {
            float intensity = (weights[m] / 255.0f) * 0.55f * base_scale;
            _schedulePulse(m, intensity, 50, systole_ms);
        }
    }

    // ===== Pathologie-specifieke trillingen (los van baseline) =====
    // Deze blijven altijd actief, ook als baseline UIT staat — anders heeft de
    // student niks om naar te luisteren als de speakers het niet doen.

    if (_pathology == PATH_S3_GALLOP) {
        float intensity = 0.4f * (_severity / 5.0f) + 0.2f;
        uint32_t s3_offset = systole_ms + (uint16_t)(0.15f * ibi_ms);
        for (uint8_t m = 0; m < NUM_MODULES; m++) {
            _schedulePulse(m, intensity, 40, s3_offset);
        }
    } else if (_pathology == PATH_S4_GALLOP) {
        float intensity = 0.4f * (_severity / 5.0f) + 0.2f;
        uint32_t s4_offset = (ibi_ms > 80) ? (ibi_ms - 80) : 0;
        for (uint8_t m = 0; m < NUM_MODULES; m++) {
            _schedulePulse(m, intensity, 40, s4_offset);
        }
    } else if (_pathology == PATH_MITRAL_STENOSIS) {
        // Opening snap kort na S2 — voelbaar op alle modules nu (speakers stuk)
        float intensity = 0.35f * (_severity / 5.0f) + 0.15f;
        uint32_t snap_offset = systole_ms + 80;
        for (uint8_t m = 0; m < NUM_MODULES; m++) {
            _schedulePulse(m, intensity, 25, snap_offset);
        }
    } else if (_pathology == PATH_AORTIC_STENOSIS ||
               _pathology == PATH_MITRAL_REGURGITATION ||
               _pathology == PATH_AORTIC_REGURGITATION) {
        // Murmur — continu "gevoel" tijdens systole/diastole.
        // Drie korte tikjes verspreid over de murmur-window om de duur tactiel
        // te suggereren. Lage intensity zodat het niet S1/S2 overstemt.
        float intensity = 0.25f + 0.05f * _severity;     // 0.30..0.50
        uint16_t window_ms = (_pathology == PATH_AORTIC_REGURGITATION)
            ? (uint16_t)(0.4f * ibi_ms) : (systole_ms - 30);
        uint16_t base_off = (_pathology == PATH_AORTIC_REGURGITATION)
            ? systole_ms : 30;
        uint16_t step = window_ms / 4;
        for (uint8_t k = 0; k < 3; k++) {
            uint32_t off = base_off + step * (k + 1);
            for (uint8_t m = 0; m < NUM_MODULES; m++) {
                _schedulePulse(m, intensity, 20, off);
            }
        }
    }
}

void CycleEngine::_schedulePulse(uint8_t module, float intensity,
                                  uint16_t duration_ms, uint32_t delay_ms) {
    xSemaphoreTake(_pulse_mutex, portMAX_DELAY);
    for (uint8_t i = 0; i < MAX_PULSES; i++) {
        if (!_pulses[i].pending) {
            _pulses[i].module      = module;
            _pulses[i].intensity   = intensity;
            _pulses[i].duration_ms = duration_ms;
            _pulses[i].fire_at_ms  = millis() + delay_ms;
            _pulses[i].pending     = true;
            break;
        }
    }
    xSemaphoreGive(_pulse_mutex);
}

void CycleEngine::_hapticTaskTrampoline(void* arg) {
    static_cast<CycleEngine*>(arg)->_hapticTaskLoop();
    vTaskDelete(nullptr);
}

void CycleEngine::_hapticTaskLoop() {
    Serial.printf("[CYCLE] Haptic task gestart op core %d\n", xPortGetCoreID());
    while (true) {
        uint32_t now = millis();
        bool fired = false;

        // Zoek de eerstvolgende pulse die "moet vuren"
        xSemaphoreTake(_pulse_mutex, portMAX_DELAY);
        for (uint8_t i = 0; i < MAX_PULSES; i++) {
            if (_pulses[i].pending && now >= _pulses[i].fire_at_ms) {
                // Kopieer de waarden naar locale variabelen
                uint8_t  m  = _pulses[i].module;
                float    in = _pulses[i].intensity;
                uint16_t d  = _pulses[i].duration_ms;
                _pulses[i].pending = false;
                xSemaphoreGive(_pulse_mutex);

                // Voer de pulse uit (dit blokkeert duration_ms lang —
                // maar we zitten in eigen task, dus geen probleem)
                _haptic->pulse(m, in, d);
                fired = true;
                break;
            }
        }
        if (!fired) {
            xSemaphoreGive(_pulse_mutex);
        }

        vTaskDelay(pdMS_TO_TICKS(2));
    }
}
