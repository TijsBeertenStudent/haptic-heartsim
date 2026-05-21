// ============================================================================
// heart_sound_synth.cpp — Implementation of the procedural PCG synthesizer
// ----------------------------------------------------------------------------
// IMPLEMENTATION NOTES
//   - All filters are Direct-Form-II-Transposed biquads (2 state variables
//     each, numerically stable and fast).
//   - The white-noise source is a 32-bit Galois LFSR with taps at
//     bits 31, 22, 2, 1 — maximal-length pseudo-random sequence with
//     ~2^32-1 samples (≈54 hours at 22 kHz) for only a few XOR/shift ops.
//   - Damped sinusoids use sinf() + expf() in float; the ESP32-S3 has a
//     hardware FPU so this is faster than a table lookup.
//   - Per-cycle parameters (S1/S2 onset, systole duration, murmur window)
//     are recomputed in triggerNewCycle() and stored as integer micro-
//     seconds, then compared sample-by-sample inside render().
//   - Soft clipping at ±0.95 with tanh() prevents harsh distortion when
//     severity is high while preserving the natural envelope shape.
//
// For the underlying mathematical models and clinical references see the
// matching header file and docs/PCG_MODEL.md.
// ============================================================================
#include "heart_sound_synth.h"
#include <math.h>

// Pathologie naam-tabel — moet matchen met enum PathologyId
static const char* PATH_NAMES[NUM_PATHOLOGIES] = {
    "NORMAL",
    "AORTIC_STENOSIS",
    "MITRAL_REGURGITATION",
    "MITRAL_STENOSIS",
    "AORTIC_REGURGITATION",
    "S3_GALLOP",
    "S4_GALLOP",
    "CRACKLES_FINE",
    "CRACKLES_COARSE",
    "WHEEZE_INSPIRATORY",
    "WHEEZE_EXPIRATORY"
};

const char* pathologyName(PathologyId id) {
    if (id < NUM_PATHOLOGIES) return PATH_NAMES[id];
    return "UNKNOWN";
}

PathologyId pathologyFromString(const char* name) {
    for (uint8_t i = 0; i < NUM_PATHOLOGIES; i++) {
        if (strcmp(PATH_NAMES[i], name) == 0) return (PathologyId)i;
    }
    return PATH_NORMAL;
}

HeartSoundSynthesizer::HeartSoundSynthesizer()
    : _pathology(PATH_NORMAL),
      _severity(3),
      _master_volume(180),
      _cycle_start_us(0),
      _ibi_us(800000),                // 75 BPM default
      _s1_start_us(0),
      _s2_start_us(280000),           // 0.35 * 800ms
      _s3_start_us(400000),
      _s4_start_us(720000),
      _samples_into_cycle(0),
      _lfsr(0xACE1u),
      _resp_rate_bpm(12.0f),
      _resp_phase(0.0f),
      _crackle_countdown(0),
      _crackle_active(0),
      _crackle_hp_y(0.0f),
      _crackle_hp_x(0.0f),
      _wheeze_phase(0.0f),
      _wheeze_fm_phase(0.0f) {
    _bp_state[0] = _bp_state[1] = 0;
    _updateBandpass(300.0f, 2.0f);
}

void HeartSoundSynthesizer::triggerNewCycle(uint16_t ibi_ms) {
    if (ibi_ms < 300) ibi_ms = 300;     // safety bounds (200 BPM max)
    if (ibi_ms > 2000) ibi_ms = 2000;   // 30 BPM min

    _ibi_us = (uint32_t)ibi_ms * 1000UL;
    _cycle_start_us = micros();

    // Plan de timings binnen deze cyclus
    _s1_start_us = 0;
    _s2_start_us = (uint32_t)(SYSTOLE_FRACTION * _ibi_us);
    _s3_start_us = _s2_start_us + (uint32_t)(0.15f * _ibi_us);
    _s4_start_us = (_ibi_us > 80000UL) ? (_ibi_us - 80000UL) : 0;

    _samples_into_cycle = 0;
}

void HeartSoundSynthesizer::setPathology(PathologyId p, uint8_t severity) {
    _pathology = p;
    _severity  = constrain(severity, 1, 5);

    if (p >= PATH_CRACKLES_FINE) {
        // Pulmonaire paden — reset respiratoire synthese state
        _resp_phase      = 0.0f;
        _crackle_countdown = 0;
        _crackle_active  = 0;
        _crackle_hp_y    = 0.0f;
        _crackle_hp_x    = 0.0f;
        _wheeze_phase    = 0.0f;
        _wheeze_fm_phase = 0.0f;
        return;
    }

    // Cardiale paden — stel biquad-bandpass in
    switch (p) {
        case PATH_AORTIC_STENOSIS:      _updateBandpass(300.0f, 2.5f); break;
        case PATH_MITRAL_REGURGITATION: _updateBandpass(350.0f, 2.0f); break;
        case PATH_MITRAL_STENOSIS:      _updateBandpass(120.0f, 4.0f); break;
        case PATH_AORTIC_REGURGITATION: _updateBandpass(350.0f, 2.5f); break;
        default:                        _updateBandpass(300.0f, 2.0f); break;
    }
}

void HeartSoundSynthesizer::setRespiratoryRate(float bpm) {
    _resp_rate_bpm = (bpm >= 4.0f && bpm <= 60.0f) ? bpm : 12.0f;
}

// ============================================================================
// DSP: white noise generator (Galois LFSR — snel en deterministisch)
// ============================================================================
inline float HeartSoundSynthesizer::_whiteNoise() {
    // 32-bit LFSR met taps op 32, 22, 2, 1
    uint32_t bit = ((_lfsr >> 0) ^ (_lfsr >> 10) ^ (_lfsr >> 30) ^ (_lfsr >> 31)) & 1u;
    _lfsr = (_lfsr >> 1) | (bit << 31);
    // Convert naar [-1, +1]
    return ((float)(int32_t)_lfsr) / 2147483648.0f;
}

// ============================================================================
// DSP: bandpass filter coefficienten berekenen
// Robert Bristow-Johnson's audio cookbook biquad formulas
// ============================================================================
void HeartSoundSynthesizer::_updateBandpass(float fc, float Q) {
    float omega = 2.0f * (float)M_PI * fc / (float)AUDIO_SAMPLE_RATE;
    float cs    = cosf(omega);
    float sn    = sinf(omega);
    float alpha = sn / (2.0f * Q);

    float a0 = 1.0f + alpha;
    _bp_b0   = alpha / a0;
    _bp_b1   = 0.0f;
    _bp_b2   = -alpha / a0;
    _bp_a1   = -2.0f * cs / a0;
    _bp_a2   = (1.0f - alpha) / a0;
}

// ============================================================================
// Damped sinusoide — kern-formule voor S1/S2/S3/S4
// ============================================================================
float HeartSoundSynthesizer::_dampedSine(float t, float freq, float alpha, float amp) {
    if (t < 0.0f || t > 0.3f) return 0.0f;      // ouder dan 300ms: stilte
    return amp * sinf(2.0f * (float)M_PI * freq * t) * expf(-alpha * t);
}

// ============================================================================
// Envelopes voor murmurs (PCG-literatuur)
// ============================================================================
float HeartSoundSynthesizer::_diamondEnvelope(float phase) {
    // Crescendo-decrescendo (Aortic Stenosis pattern)
    // Driehoekige envelope met piek bij phase=0.5
    if (phase < 0.0f || phase > 1.0f) return 0.0f;
    return (phase < 0.5f) ? (phase * 2.0f) : ((1.0f - phase) * 2.0f);
}

float HeartSoundSynthesizer::_decrescendoEnvelope(float phase) {
    // Aortic Regurgitation: max bij begin, vervalt exponentieel
    if (phase < 0.0f || phase > 1.0f) return 0.0f;
    return expf(-3.0f * phase);
}

// ============================================================================
// Hoofdfunctie: bereken één sample op gegeven cycle-time
// ============================================================================
int16_t HeartSoundSynthesizer::_sampleAtCycleTime(uint32_t cycle_time_us) {
    float sample = 0.0f;
    float severity_gain = 0.4f + 0.15f * _severity;   // 0.55..1.15

    // ---- S1 (altijd aanwezig) ----
    float t_s1 = (cycle_time_us - _s1_start_us) / 1.0e6f;
    sample += _dampedSine(t_s1, S1_FREQ_HZ, 25.0f, 0.85f);

    // ---- S2 (altijd aanwezig) ----
    if (cycle_time_us >= _s2_start_us) {
        float t_s2 = (cycle_time_us - _s2_start_us) / 1.0e6f;
        // S2 iets zachter dan S1 in normaal hart
        sample += _dampedSine(t_s2, S2_FREQ_HZ, 30.0f, 0.65f);
    }

    // ---- Pathologie-afhankelijke componenten ----
    switch (_pathology) {
        case PATH_NORMAL:
            // Niets extra
            break;

        case PATH_AORTIC_STENOSIS: {
            // Systolic crescendo-decrescendo murmur tussen S1 en S2
            uint32_t murmur_start = _s1_start_us + 30000;        // 30ms na S1
            uint32_t murmur_end   = _s2_start_us - 20000;        // 20ms vóór S2
            if (cycle_time_us >= murmur_start && cycle_time_us < murmur_end) {
                float phase = (float)(cycle_time_us - murmur_start) /
                              (float)(murmur_end - murmur_start);
                float env = _diamondEnvelope(phase) * 0.5f * severity_gain;

                // Genereer ruis door bandpass
                float n = _whiteNoise();
                float w = n - _bp_a1 * _bp_state[0] - _bp_a2 * _bp_state[1];
                float y = _bp_b0 * w + _bp_b1 * _bp_state[0] + _bp_b2 * _bp_state[1];
                _bp_state[1] = _bp_state[0];
                _bp_state[0] = w;

                sample += y * env;
            }
            break;
        }

        case PATH_MITRAL_REGURGITATION: {
            // Holosystolic blowing murmur — rechthoek envelope, hele systole
            uint32_t murmur_start = _s1_start_us + 10000;
            uint32_t murmur_end   = _s2_start_us;
            if (cycle_time_us >= murmur_start && cycle_time_us < murmur_end) {
                float env = 0.45f * severity_gain;     // constant niveau

                float n = _whiteNoise();
                float w = n - _bp_a1 * _bp_state[0] - _bp_a2 * _bp_state[1];
                float y = _bp_b0 * w + _bp_b1 * _bp_state[0] + _bp_b2 * _bp_state[1];
                _bp_state[1] = _bp_state[0];
                _bp_state[0] = w;

                sample += y * env;
            }
            break;
        }

        case PATH_MITRAL_STENOSIS: {
            // 1. Opening snap kort na S2 (~80ms na S2)
            uint32_t snap_start = _s2_start_us + 80000;
            float t_snap = (cycle_time_us - snap_start) / 1.0e6f;
            if (t_snap >= 0.0f && t_snap < 0.05f) {
                sample += _dampedSine(t_snap, 100.0f, 60.0f, 0.4f * severity_gain);
            }

            // 2. Diastolic rumble tot vlak voor volgende S1
            uint32_t rumble_start = snap_start + 50000;
            uint32_t rumble_end   = (_ibi_us > 50000) ? (_ibi_us - 50000) : _ibi_us;
            if (cycle_time_us >= rumble_start && cycle_time_us < rumble_end) {
                float phase = (float)(cycle_time_us - rumble_start) /
                              (float)(rumble_end - rumble_start);
                // Decrescendo-crescendo: dipt in midden, harder bij eind (presystolic)
                float env = (0.3f + 0.4f * fabsf(phase - 0.5f) * 2.0f) * severity_gain;

                float n = _whiteNoise();
                float w = n - _bp_a1 * _bp_state[0] - _bp_a2 * _bp_state[1];
                float y = _bp_b0 * w + _bp_b1 * _bp_state[0] + _bp_b2 * _bp_state[1];
                _bp_state[1] = _bp_state[0];
                _bp_state[0] = w;

                sample += y * env;
            }
            break;
        }

        case PATH_AORTIC_REGURGITATION: {
            // Early diastolic decrescendo — direct na S2
            uint32_t murmur_start = _s2_start_us;
            uint32_t murmur_end   = _s2_start_us + (uint32_t)(0.4f * _ibi_us);
            if (cycle_time_us >= murmur_start && cycle_time_us < murmur_end) {
                float phase = (float)(cycle_time_us - murmur_start) /
                              (float)(murmur_end - murmur_start);
                float env = _decrescendoEnvelope(phase) * 0.55f * severity_gain;

                float n = _whiteNoise();
                float w = n - _bp_a1 * _bp_state[0] - _bp_a2 * _bp_state[1];
                float y = _bp_b0 * w + _bp_b1 * _bp_state[0] + _bp_b2 * _bp_state[1];
                _bp_state[1] = _bp_state[0];
                _bp_state[0] = w;

                sample += y * env;
            }
            break;
        }

        case PATH_S3_GALLOP: {
            // Extra hartgeluid in vroege diastole, ~150ms na S2
            uint32_t s3 = _s2_start_us + 150000;
            float t = (cycle_time_us - s3) / 1.0e6f;
            sample += _dampedSine(t, S3_FREQ_HZ, 20.0f, 0.5f * severity_gain);
            break;
        }

        case PATH_S4_GALLOP: {
            // Extra hartgeluid vlak voor S1 (~80ms ervoor)
            float t = (cycle_time_us - _s4_start_us) / 1.0e6f;
            sample += _dampedSine(t, S4_FREQ_HZ, 18.0f, 0.45f * severity_gain);
            break;
        }

        default:
            break;
    }

    // Master volume + soft-clipping om distortie bij hoge severity te voorkomen
    sample *= (_master_volume / 255.0f);
    if (sample > 0.95f) sample = 0.95f + 0.05f * tanhf(sample - 0.95f);
    if (sample < -0.95f) sample = -0.95f + 0.05f * tanhf(sample + 0.95f);

    return (int16_t)(sample * 30000.0f);
}

// ============================================================================
// Pulmonaire synthese — crackles en wheezes
// ============================================================================
int16_t HeartSoundSynthesizer::_pulmonarySample() {
    float sample = 0.0f;
    const float severity_gain = 0.4f + 0.15f * _severity;

    // Respiratoire fase:  0..INSP_FRAC = inspiratie,  INSP_FRAC..1 = expiratie
    constexpr float INSP_FRAC = 0.4f;
    const bool  insp       = (_resp_phase < INSP_FRAC);
    const float insp_phase = _resp_phase / INSP_FRAC;                          // 0→1 tijdens inspiratie
    const float exp_phase  = (_resp_phase - INSP_FRAC) / (1.0f - INSP_FRAC);  // 0→1 tijdens expiratie

    switch (_pathology) {

        case PATH_CRACKLES_FINE:
        case PATH_CRACKLES_COARSE: {
            const bool     fine         = (_pathology == PATH_CRACKLES_FINE);
            const uint16_t burst_len    = fine ? 110u : 441u;   // 5 ms of 20 ms
            const uint32_t mean_period  = fine ? 2205u : 4410u; // ~100 ms of ~200 ms

            // ── Actieve burst: output 1e-orde HP-gefilterde ruis ─────────────
            if (_crackle_active > 0) {
                float n = _whiteNoise();
                // y[n] = α*(y[n-1] + x[n] − x[n-1])
                // α ≈ 0.75 (fine, fc≈1 kHz) of 0.87 (coarse, fc≈500 Hz)
                const float alpha = fine ? 0.75f : 0.87f;
                float hp = alpha * (_crackle_hp_y + n - _crackle_hp_x);
                _crackle_hp_x = n;
                _crackle_hp_y = hp;
                // Halve-sinus-envelope over de burst-duur
                float progress = 1.0f - (float)_crackle_active / (float)burst_len;
                sample += hp * sinf((float)M_PI * progress) * 0.7f * severity_gain;
                _crackle_active--;
            }

            // ── Burst-scheduler ──────────────────────────────────────────────
            if (_crackle_countdown > 0) {
                _crackle_countdown--;
            } else if (insp && _crackle_active == 0) {
                // Vuur een nieuwe burst
                _crackle_active = burst_len;
                // Volgende burst: mean_period ±20 % jitter via LFSR-bits
                uint8_t rbyte = (uint8_t)((_lfsr >> 8) & 0xFF);
                _crackle_countdown = (uint32_t)(mean_period * (0.8f + 0.4f * rbyte / 255.0f));
            } else if (!insp) {
                // Buiten inspiratie: kort wachten en opnieuw checken
                _crackle_countdown = 441; // 20 ms
            }
            break;
        }

        case PATH_WHEEZE_INSPIRATORY:
        case PATH_WHEEZE_EXPIRATORY: {
            const bool insp_type    = (_pathology == PATH_WHEEZE_INSPIRATORY);
            const float phase_local = insp_type ? insp_phase : exp_phase;
            const bool  active      = insp_type ? insp : !insp;
            // Halve-sinus-envelope over de actieve fase
            const float env = active ? (sinf((float)M_PI * phase_local) * severity_gain * 0.45f)
                                      : 0.0f;

            // FM-wheeze: fc=300 Hz, FM-diepte ±60 Hz bij 2 Hz modulatiefrequentie
            const float fm_freq    = 300.0f + 60.0f * sinf(_wheeze_fm_phase);
            _wheeze_phase   += 2.0f * (float)M_PI * fm_freq / (float)AUDIO_SAMPLE_RATE;
            if (_wheeze_phase >= 2.0f * (float)M_PI) _wheeze_phase -= 2.0f * (float)M_PI;
            _wheeze_fm_phase += 2.0f * (float)M_PI * 2.0f / (float)AUDIO_SAMPLE_RATE;
            if (_wheeze_fm_phase >= 2.0f * (float)M_PI) _wheeze_fm_phase -= 2.0f * (float)M_PI;

            sample += sinf(_wheeze_phase) * env;
            break;
        }

        default: break;
    }

    sample *= (_master_volume / 255.0f);
    if (sample >  0.95f) sample =  0.95f + 0.05f * tanhf(sample - 0.95f);
    if (sample < -0.95f) sample = -0.95f + 0.05f * tanhf(sample + 0.95f);
    return (int16_t)(sample * 30000.0f);
}

void HeartSoundSynthesizer::render(int16_t* buf, size_t n_samples) {
    constexpr uint32_t US_PER_SAMPLE = 1000000UL / AUDIO_SAMPLE_RATE;
    // Hoe ver de respiratoire fase per sample vooruitgaat
    const float resp_inc = _resp_rate_bpm / (60.0f * (float)AUDIO_SAMPLE_RATE);

    for (size_t i = 0; i < n_samples; i++) {
        if (_pathology >= PATH_CRACKLES_FINE) {
            // Pulmonaire synthese — volledig ademhaling-gedreven, geen cardiaal
            buf[i] = _pulmonarySample();
        } else {
            // Cardiale synthese — cyclus-gedreven
            uint32_t cycle_time_us = _samples_into_cycle * US_PER_SAMPLE;
            buf[i] = (cycle_time_us < _ibi_us) ? _sampleAtCycleTime(cycle_time_us) : 0;
        }

        _samples_into_cycle++;
        _resp_phase += resp_inc;
        if (_resp_phase >= 1.0f) _resp_phase -= 1.0f;
    }
}
