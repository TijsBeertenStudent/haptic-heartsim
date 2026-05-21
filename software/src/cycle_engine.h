// ============================================================================
// cycle_engine.h — Cardiac cycle scheduler (the "conductor")
// ----------------------------------------------------------------------------
// RESPONSIBILITY
//   On every detected R-peak (or on its internal demo-clock tick) the cycle
//   engine:
//     1. Triggers a new cycle in the audio synthesizer (S1 starts NOW)
//     2. Schedules haptic bursts for S1, S2 and, depending on the active
//        pathology, optional extras (S3, S4, opening snap, murmur taps)
//     3. Updates per-pathology spatial weights in the audio engine so each
//        amplifier reproduces its anatomically-correct loudness profile
//
// THREADING MODEL
//   - update() is called from the main loop on core 0. It only performs
//     short non-blocking work (read PPG flag, compare timestamps).
//   - The synthesizer itself runs in a dedicated task on core 1 so DSP
//     CPU load never starves the WiFi stack on core 0.
//   - Haptic pulses are produced in a third task (low priority, core 0)
//     because the DRV2605L pulse routine uses a blocking delay() — we do
//     not want that to stall the main loop or the audio task.
//
// PULSE TRANSIT TIME (PTT) COMPENSATION
//   The PPG sensor sits at the fingertip, so its detected peak lags the
//   real cardiac S1 by roughly 150-250 ms (depends on age, height, BP).
//   For a clinically convincing simulation the haptic S1 should hit
//   *before* the next PPG peak by exactly that lag. The engine therefore
//   maintains a 4-beat rolling average IBI and predicts the next S1 as:
//       predicted_S1 = last_PPG_peak + (avg_IBI − PTT_OFFSET)
//   If the IBI is shorter than PTT_OFFSET (very fast heart rate) or a new
//   peak arrives before the scheduled prediction (arrhythmia), it falls
//   back to firing immediately.
//
// DEMO / PLAYBACK MODE
//   For a controlled demonstration the engine can ignore the PPG entirely
//   and run on its own clock at any chosen BPM (30..200). Useful when
//   the sensor is unreliable or the operator wants to show that the
//   simulator follows whatever rate is chosen.
// ============================================================================
#pragma once
#include <Arduino.h>
#include "config.h"
#include "ppg_monitor.h"
#include "audio_engine.h"
#include "haptic_manager.h"
#include "heart_sound_synth.h"

class CycleEngine {
public:
    CycleEngine(PPGMonitor* ppg, AudioEngine* audio, HapticManager* haptic);

    void begin();

    /** Roep aan vanuit main loop. */
    void update();

    /** Start de simulatie (haptiek + audio actief). */
    void start();
    void stop();
    bool isRunning() const { return _running; }

    /** Pathologie + severity */
    void setPathology(PathologyId p, uint8_t severity);
    PathologyId getPathology() const { return _pathology; }
    uint8_t     getSeverity()  const { return _severity; }

    /**
     * Playback mode: geen PPG nodig, simuleer interne BPM.
     * Belangrijke backup voor demo day!
     */
    void setPlaybackMode(bool enabled);
    bool isPlaybackMode() const { return _playback_mode; }

    /**
     * Stel de BPM in voor playback-mode (30..200). Default = 70.
     */
    void     setPlaybackBPM(uint16_t bpm);
    uint16_t getPlaybackBPM() const { return _playback_bpm; }

    /**
     * Baseline-haptic: of de "gewone hartslag" (S1+S2) door de motoren wordt
     * gespeeld. Als false: alleen pathologie-specifieke trillingen (S3/S4/snap)
     * worden gepulseerd; de student moet dan op de stethoscoop vertrouwen voor
     * de basis-hartslag. Default = true.
     */
    void setBaselineEnabled(bool enabled) { _baseline_enabled = enabled; }
    bool isBaselineEnabled() const { return _baseline_enabled; }

    /**
     * Intensiteit van de baseline-haptic (0..10). 0 = uit, 10 = max.
     * Onafhankelijk van de master Haptic slider (die is een algemene multiplier).
     */
    void    setBaselineIntensity(uint8_t v);
    uint8_t getBaselineIntensity() const { return _baseline_intensity; }

    /**
     * PTT (Pulse Transit Time) correctie.
     *
     * De PPG-vinger-piek arriveert ~150-250 ms NA de echte cardiale S1.
     * CycleEngine voorspelt de volgende S1 als:
     *   next_ppg_peak ≈ last_ppg_peak + avg_ibi
     *   predicted_S1  = next_ppg_peak - ptt_offset
     *                 = now + (avg_ibi - ptt_offset)
     *
     * Als de IBI korter is dan de PTT-offset (extreem snelle hartslag of
     * hoge offset), of als een nieuwe piek arriveert vóór de geplande trigger
     * (aritmie), valt het systeem terug op onmiddellijk triggeren.
     *
     * Bereik: 0..400 ms. Default: PPG_PTT_OFFSET_MS (200 ms).
     */
    void     setPTTOffset(uint16_t ms);
    uint16_t getPTTOffset() const { return _ptt_offset_ms; }

    /** Gemiddeld IBI over de laatste 4 slagen — voor telemetrie. */
    uint16_t getAvgIBI() const { return _avg_ibi_ms; }

private:
    PPGMonitor*    _ppg;
    AudioEngine*   _audio;
    HapticManager* _haptic;

    PathologyId _pathology;
    uint8_t     _severity;
    bool        _running;
    bool        _playback_mode;

    // Haptiek scheduling — wat is gepland en wanneer
    struct ScheduledPulse {
        uint8_t  module;
        float    intensity;
        uint16_t duration_ms;
        uint32_t fire_at_ms;
        bool     pending;
    };
    static constexpr uint8_t MAX_PULSES = 16;
    ScheduledPulse _pulses[MAX_PULSES];

    // Playback-mode internal clock
    uint32_t _last_playback_beat_ms;
    uint16_t _playback_ibi_ms;
    uint16_t _playback_bpm = 70;          // 30..200

    // Baseline-haptic state
    bool    _baseline_enabled  = true;
    uint8_t _baseline_intensity = 8;       // 0..10

    // Vorige R-piek tijd + meest recente gemeten IBI
    uint32_t _last_r_peak_ms;
    uint16_t _last_ibi_ms;

    // Rolling-average IBI (laatste N slagen) voor PTT-predictie
    static constexpr uint8_t IBI_HISTORY_LEN = 4;
    uint16_t _ibi_history[IBI_HISTORY_LEN];
    uint8_t  _ibi_history_idx;
    uint16_t _avg_ibi_ms;

    // PTT correctie-state
    uint16_t _ptt_offset_ms;
    bool     _trigger_pending;         // Is er een geplande voorspelde trigger?
    uint32_t _predicted_trigger_at_ms; // Tijdstip (millis) waarop die trigger vuurt

    // Haptiek task: zorgt dat blocking pulses niet de main loop blokkeren
    TaskHandle_t _haptic_task;
    SemaphoreHandle_t _pulse_mutex;
    static void _hapticTaskTrampoline(void* arg);
    void _hapticTaskLoop();

    void _onRPeak(uint16_t ibi_ms);
    void _handlePPGPeak(uint32_t peak_ms);
    void _updateAvgIBI();
    void _scheduleCycleEvents(uint16_t ibi_ms);
    void _schedulePulse(uint8_t module, float intensity, uint16_t duration_ms, uint32_t delay_ms);
};
