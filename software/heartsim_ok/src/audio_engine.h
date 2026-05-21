// ============================================================================
// audio_engine.h — I2S audio output + per-speaker volume via PWM on SD pin
// ----------------------------------------------------------------------------
// ARCHITECTURE
//   - HeartSoundSynthesizer produces a mono PCM stream at 22050 Hz.
//   - We push those samples through the ESP32-S3 I2S peripheral to two
//     MAX98357A class-D amplifiers wired in parallel (both receive the
//     same digital audio).
//   - Per-speaker volume is set by feeding a PWM duty cycle through an
//     RC low-pass filter (10 kΩ + 1 µF, ≈10 ms time constant) into the
//     SD/GAIN pin of each MAX98357. The chip reads the DC voltage on
//     that pin and interprets it as a mute/gain command.
//
// MAX98357 SD-PIN BEHAVIOUR (from the datasheet)
//     < 0.16 V    : SHUTDOWN (output muted)
//     0.16-0.77 V : enable, left  channel only (we ignore)
//     0.77-1.40 V : enable, right channel only (we ignore)
//     > 1.40 V    : enable, mono = (L+R) / 2  ← what we use
//   Therefore PWM=0 → mute and PWM=255 → ≈3.3 V → full mono gain at the
//   default 9 dB. PWM resolution is set to 8 bits to match the 0..255
//   convention used throughout the codebase.
//
// SPATIAL WEIGHTS — clinical realism
//   In real auscultation each heart sound has a characteristic loudness
//   profile across the four chest positions. An aortic stenosis murmur is
//   loudest at the aortic point, a mitral regurgitation murmur at the
//   apex (mitral), etc. PATHOLOGY_WEIGHTS encodes these intensities as a
//   per-pathology 4-vector; the audio engine averages the four values
//   into a pair of speakers (Speaker 0 = upper points, Speaker 1 = lower).
// ============================================================================
#pragma once
#include <Arduino.h>
#include "config.h"
#include "heart_sound_synth.h"
#include "recorder.h"

class AudioEngine {
public:
    AudioEngine();
    bool begin();

    /** Start de audio-task die continu het synth-buffer naar I2S streamt. */
    void start();
    void stop();

    /**
     * Geef toegang tot de synthesizer zodat CycleEngine triggerNewCycle()
     * kan aanroepen. (We zouden ook callback kunnen doen — keep it simple.)
     */
    HeartSoundSynthesizer* synthesizer() { return &_synth; }

    /** Geeft toegang tot de Recorder voor web interface. */
    Recorder* recorder() { return &_recorder; }

    /** Master volume = digitale schaling op de samples (in de synth). */
    void setMasterVolume(uint8_t v);

    /**
     * Per-speaker gain via PWM op SD-pin van die MAX98357.
     * @param module 0..NUM_AUDIO_MODULES-1 (0=upper speaker, 1=lower speaker)
     * @param vol    0=mute, 255=max
     */
    void setModuleVolume(uint8_t module, uint8_t vol);
    uint8_t getModuleVolume(uint8_t module) const;

    /**
     * Stel de "spatial weight"-vector in voor de huidige pathologie.
     * Wordt typisch aangeroepen vanuit CycleEngine.setPathology().
     */
    void setSpatialWeights(const uint8_t weights[NUM_MODULES]);

private:
    HeartSoundSynthesizer _synth;
    Recorder              _recorder;
    uint8_t _module_volume[NUM_AUDIO_MODULES];
    bool    _running;
    TaskHandle_t _audio_task;

    static void _audioTaskTrampoline(void* arg);
    void _audioTaskLoop();
};

// Per-pathologie spatial weights (welk auscultatiepunt het luidst is)
// Index = AuscultationPoint (Aortic, Pulmonic, Tricuspid, Mitral)
// Waarden zijn 0..255. Wordt automatisch toegepast door CycleEngine.
extern const uint8_t PATHOLOGY_WEIGHTS[NUM_PATHOLOGIES][NUM_MODULES];
