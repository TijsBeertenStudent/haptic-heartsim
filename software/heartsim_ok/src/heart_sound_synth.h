// ============================================================================
// heart_sound_synth.h — Procedural synthesis of heart and lung sounds
// ----------------------------------------------------------------------------
// WHY PROCEDURAL (NOT WAV PLAYBACK)
//   Instead of streaming pre-recorded WAV files we generate every sample in
//   real time from a mathematical model. Advantages:
//     1. No MB-sized audio assets in flash storage
//     2. The "severity" slider becomes PARAMETRIC (intensity, roughness,
//        duration scale continuously rather than crossfading between clips)
//     3. The pathology can switch mid-cycle without an audible glitch
//     4. The model maps directly onto published PCG literature, so the
//        synthesis can be defended scientifically (Jabloun et al. 2013;
//        Schmidt et al. PhysioNet/CinC Challenge 2016)
//
// MATHEMATICAL MODELS USED
//
//   S1, S2, S3, S4 — DAMPED SINUSOIDS
//     s(t) = A · sin(2π · f · t) · exp(-α · t)
//     This is the analytical solution of a 2nd-order spring-mass-damper
//     system, which is exactly how valve leaflet closure behaves (a brief
//     impulse followed by mechanical ringdown). Parameters chosen from
//     literature:
//       S1: f=50 Hz, α=25  → duration ≈80 ms, band 20-100 Hz
//       S2: f=70 Hz, α=30  → duration ≈70 ms, band 50-200 Hz
//       S3: f=35 Hz, α=20  → duration ≈60 ms, band 25-50 Hz
//       S4: f=25 Hz, α=18  → duration ≈50 ms, band 20-30 Hz
//
//   MURMURS — BAND-PASS-FILTERED WHITE NOISE × TIME-VARYING ENVELOPE
//     n(t) = noise(t) · BPF(fc, BW) · envelope(t)
//     The noise represents the turbulent blood flow through a defective
//     valve; the band-pass filter shapes its spectrum; the envelope
//     determines when the murmur grows and fades within one heart cycle.
//
//       Aortic Stenosis (systolic):
//         fc=300 Hz, BW=200 Hz, DIAMOND envelope (crescendo-decrescendo)
//
//       Mitral Regurgitation (systolic):
//         fc=350 Hz, BW=250 Hz, RECTANGULAR envelope (holosystolic
//         "blowing" — fills the entire systole at constant intensity)
//
//       Mitral Stenosis (diastolic):
//         Short opening snap (high-frequency click) immediately after S2,
//         followed by a low-frequency "rumble":
//         fc=120 Hz, BW=80 Hz, decrescendo into a crescendo near S1
//
//       Aortic Regurgitation (early diastolic):
//         fc=350 Hz, BW=250 Hz, EXPONENTIAL DECRESCENDO envelope after S2
//
//   PULMONARY SOUNDS — INDEPENDENT OF CARDIAC CYCLE
//     Crackles: stochastic bursts of HP-filtered noise (short impulses
//       spaced randomly during the inspiratory phase)
//     Wheezes:  a single sinusoidal oscillator whose frequency is
//       slowly modulated (FM ±60 Hz around 300 Hz at 2 Hz) and gated by
//       the inspiratory or expiratory half of the breathing cycle
// ============================================================================
#pragma once
#include <Arduino.h>
#include "config.h"

enum PathologyId : uint8_t {
    // ── Cardiac (hart-cyclus gestuurd) ──────────────────────────
    PATH_NORMAL               = 0,
    PATH_AORTIC_STENOSIS      = 1,
    PATH_MITRAL_REGURGITATION = 2,
    PATH_MITRAL_STENOSIS      = 3,
    PATH_AORTIC_REGURGITATION = 4,
    PATH_S3_GALLOP            = 5,
    PATH_S4_GALLOP            = 6,
    // ── Pulmonary (ademhalingsritme gestuurd) ───────────────────
    // Kortdurende ruisimpulsen over de inspiratoire fase.
    PATH_CRACKLES_FINE        = 7,   // ~20 × 5 ms hoge-frequentie knettergeluiden
    PATH_CRACKLES_COARSE      = 8,   // ~10 × 20 ms lage-frequentie gorgelgeluiden
    // Aanhoudend sinusgolfsignaal, gemoduleerd op ademhaling.
    PATH_WHEEZE_INSPIRATORY   = 9,   // 200-400 Hz piep tijdens inspiratie
    PATH_WHEEZE_EXPIRATORY    = 10,  // 200-400 Hz piep tijdens expiratie
    NUM_PATHOLOGIES
};

const char* pathologyName(PathologyId id);
PathologyId pathologyFromString(const char* name);

class HeartSoundSynthesizer {
public:
    HeartSoundSynthesizer();

    /**
     * Genereer N samples van de huidige hartcyclus en schrijf ze naar buf.
     * @param buf       output samples (int16, mono)
     * @param n_samples aantal samples te genereren
     */
    void render(int16_t* buf, size_t n_samples);

    /**
     * Start een nieuwe hartcyclus. Roep aan bij elke gedetecteerde R-piek.
     * @param ibi_ms voorspeld RR-interval — bepaalt timing van S2, murmur, etc.
     */
    void triggerNewCycle(uint16_t ibi_ms);

    /** Pathologie + severity (1..5). */
    void setPathology(PathologyId p, uint8_t severity);
    PathologyId getPathology() const { return _pathology; }
    uint8_t getSeverity() const { return _severity; }

    /** Master volume 0..255. */
    void setMasterVolume(uint8_t v) { _master_volume = v; }

    /**
     * Ademhalingsfrequentie voor pulmonaire geluiden.
     * Heeft geen effect op cardiale paden.
     * Default: 12 slagen/min (normaal rustig ademhalen).
     */
    void  setRespiratoryRate(float bpm);
    float getRespiratoryRate() const { return _resp_rate_bpm; }

private:
    // ===== State =====
    PathologyId _pathology;
    uint8_t     _severity;            // 1..5
    uint8_t     _master_volume;

    // Cyclus state — bijgewerkt bij triggerNewCycle()
    uint32_t _cycle_start_us;          // micros() bij start huidige cyclus
    uint32_t _ibi_us;                  // voorspeld RR-interval
    uint32_t _s1_start_us;             // 0 (begin van cyclus)
    uint32_t _s2_start_us;             // SYSTOLE_FRACTION * IBI
    uint32_t _s3_start_us;             // S2 + 0.15*IBI
    uint32_t _s4_start_us;             // IBI - 80ms

    // Sample teller binnen de huidige cyclus
    uint32_t _samples_into_cycle;

    // ===== DSP staat voor murmur (bandpass filter + LFSR voor noise) =====
    uint32_t _lfsr;                    // White noise generator (Galois LFSR)
    float    _bp_state[2];             // Biquad state voor murmur
    float    _bp_b0, _bp_b1, _bp_b2;   // Biquad coefficienten (dynamisch)
    float    _bp_a1, _bp_a2;

    // ===== Pulmonaire synthese state =====
    float    _resp_rate_bpm;           // Ademhalingsfrequentie (default 12)
    float    _resp_phase;              // 0..1, wraps per ademcyclus
    //   0.0..0.4 = inspiratie, 0.4..1.0 = expiratie

    // Crackle burst-generator
    uint32_t _crackle_countdown;       // Samples tot volgende burst mag vuren
    uint16_t _crackle_active;          // Samples resterend in huidige burst (0=stil)
    float    _crackle_hp_y;            // 1e-orde HP filter state (y[n-1])
    float    _crackle_hp_x;            // 1e-orde HP filter state (x[n-1])

    // Wheeze FM-oscillator
    float    _wheeze_phase;            // Audio-frequentie fase-accumulator
    float    _wheeze_fm_phase;         // FM-modulator fase-accumulator

    // ===== Generatie helpers =====
    inline float _whiteNoise();
    void  _updateBandpass(float fc, float Q);
    int16_t _pulmonarySample();        // Compleet sample voor pulmonaire paden

    /**
     * Damped sinusoide voor S1/S2/S3/S4.
     * @param t_in_sound tijd in seconden sinds start van dit geluid
     * @param freq       fundamentele frequentie (Hz)
     * @param alpha      decay rate (1/s)
     * @param amplitude  0..1
     * @return sample waarde (mono float)
     */
    float _dampedSine(float t_in_sound, float freq, float alpha, float amplitude);

    /**
     * Diamant envelope (crescendo-decrescendo) — Aortic Stenosis vorm.
     * @param phase 0..1 = positie binnen de murmur
     * @return amplitude factor 0..1
     */
    float _diamondEnvelope(float phase);

    /**
     * Decrescendo envelope — Aortic Regurgitation vorm.
     */
    float _decrescendoEnvelope(float phase);

    /**
     * Get sample at a specific cycle-time (us). Hoofdrendering functie.
     */
    int16_t _sampleAtCycleTime(uint32_t cycle_time_us);
};
