// ============================================================================
// recorder.h — Audio capture to LittleFS WAV
// ----------------------------------------------------------------------------
// Taps into the AudioEngine render buffer (same int16 samples that go to I2S)
// and accumulates them in a fixed 64 KB SRAM ring buffer.
//
// At 22050 Hz mono int16 (2 bytes/sample) that is:
//   65536 / 2 = 32768 samples → ~1.49 seconds of audio.
//
// WAV layout written to LittleFS /recordings/rec_NNNNNN.wav:
//   44-byte PCM WAV header + raw int16 LE samples
// ============================================================================
#pragma once
#include <Arduino.h>
#include <LittleFS.h>
#include "config.h"

class Recorder {
public:
    Recorder();

    /** Mount check + ensure /recordings directory exists. */
    bool begin();

    /** Start capturing — resets buffer, sets _recording flag. */
    void startRecording();

    /**
     * Stop capturing and flush WAV to LittleFS.
     * @return filename written (e.g. "/recordings/rec_000123.wav"), or "" on error.
     */
    String stopRecording();

    bool isRecording() const { return _recording; }

    /**
     * Called by AudioEngine after every render() call.
     * Copies up to n_samples into the capture buffer (stops silently when full).
     * ISR-safe: only writes _head; reader uses _head snapshot.
     */
    void feedSamples(const int16_t* buf, size_t n_samples);

    /** List all /recordings/*.wav files as a JSON array string. */
    String listRecordingsJson() const;

    /** Delete a single recording by filename. Returns true on success. */
    bool deleteRecording(const String& filename);

private:
    static constexpr size_t BUF_SAMPLES = 32768;  // 64 KB
    int16_t  _buf[BUF_SAMPLES];
    size_t   _head;       // write position (monotonic, mod BUF_SAMPLES)
    size_t   _captured;   // samples actually captured in this take
    bool     _recording;
    uint32_t _rec_counter; // increments per recording for unique filenames

    void _writeWavHeader(File& f, uint32_t n_samples) const;
};
