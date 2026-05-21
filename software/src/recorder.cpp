// ============================================================================
// recorder.cpp — Implementation of the in-memory WAV recorder
// ----------------------------------------------------------------------------
// CAPTURE FLOW
//   1. startRecording() resets the buffer head and sets _recording = true.
//   2. AudioEngine calls feedSamples() for every DMA buffer it generates,
//      so the recorder receives EXACTLY the same samples that are sent to
//      the speakers (sample-accurate capture).
//   3. The SRAM buffer is 64 KB = 32768 samples = ~1.49 s at 22050 Hz.
//      We stop capturing automatically once it is full (instead of
//      overwriting old data) so a single recording is always a contiguous
//      clip starting at startRecording().
//   4. stopRecording() flushes the buffer to LittleFS under
//      /recordings/rec_NNNNNN.wav with a standard 44-byte PCM WAV header.
//
// WAV HEADER LAYOUT (see _writeWavHeader)
//   Offset  Size  Field         Value used here
//     0     4     "RIFF" tag    'RIFF'
//     4     4     ChunkSize     36 + data_bytes
//     8     8     "WAVEfmt "    'WAVEfmt '
//    16     4     Subchunk1Size 16 (PCM)
//    20     2     AudioFormat   1 (PCM)
//    22     2     NumChannels   1 (mono)
//    24     4     SampleRate    22050
//    28     4     ByteRate      44100 (fs * channels * 2)
//    32     2     BlockAlign    2 (channels * 2)
//    34     2     BitsPerSample 16
//    36     4     "data" tag    'data'
//    40     4     Subchunk2Size data_bytes
//    44     N     samples       int16 little-endian
// ============================================================================
#include "recorder.h"

Recorder::Recorder()
    : _head(0), _captured(0), _recording(false), _rec_counter(0) {}

bool Recorder::begin() {
    if (!LittleFS.exists("/recordings")) {
        LittleFS.mkdir("/recordings");
    }
    return true;
}

void Recorder::startRecording() {
    _head     = 0;
    _captured = 0;
    _recording = true;
    Serial.println("[REC] Opname gestart");
}

String Recorder::stopRecording() {
    _recording = false;
    if (_captured == 0) {
        Serial.println("[REC] Niets opgenomen");
        return "";
    }

    _rec_counter++;
    char fname[40];
    snprintf(fname, sizeof(fname), "/recordings/rec_%06lu.wav",
             (unsigned long)_rec_counter);

    File f = LittleFS.open(fname, "w");
    if (!f) {
        Serial.printf("[REC] FOUT: kan %s niet openen\n", fname);
        return "";
    }

    _writeWavHeader(f, (uint32_t)_captured);

    // Write samples — buffer is linear (not a true ring; we stop at BUF_SAMPLES)
    f.write(reinterpret_cast<const uint8_t*>(_buf), _captured * sizeof(int16_t));
    f.close();

    Serial.printf("[REC] Opgeslagen: %s (%u samples, %.2f s)\n",
                  fname, (unsigned)_captured,
                  (float)_captured / (float)AUDIO_SAMPLE_RATE);
    return String(fname);
}

void Recorder::feedSamples(const int16_t* buf, size_t n_samples) {
    if (!_recording) return;
    size_t space = BUF_SAMPLES - _head;
    size_t to_copy = (n_samples < space) ? n_samples : space;
    if (to_copy == 0) {
        // Buffer full — auto-stop so stopRecording() can flush
        _recording = false;
        return;
    }
    memcpy(_buf + _head, buf, to_copy * sizeof(int16_t));
    _head     += to_copy;
    _captured += to_copy;
}

String Recorder::listRecordingsJson() const {
    String out = "[";
    bool first = true;
    File dir = LittleFS.open("/recordings");
    if (dir) {
        File entry = dir.openNextFile();
        while (entry) {
            if (!entry.isDirectory()) {
                if (!first) out += ",";
                out += "{\"name\":\"";
                out += entry.name();
                out += "\",\"size\":";
                out += entry.size();
                out += "}";
                first = false;
            }
            entry = dir.openNextFile();
        }
    }
    out += "]";
    return out;
}

bool Recorder::deleteRecording(const String& filename) {
    // Sanitise: must be under /recordings/ and end with .wav
    if (!filename.startsWith("/recordings/") || !filename.endsWith(".wav")) {
        return false;
    }
    return LittleFS.remove(filename);
}

// ============================================================================
// 44-byte standard PCM WAV header
// ============================================================================
void Recorder::_writeWavHeader(File& f, uint32_t n_samples) const {
    const uint32_t byte_rate    = AUDIO_SAMPLE_RATE * 1 * 2; // fs * ch * bytes/sample
    const uint32_t data_bytes   = n_samples * sizeof(int16_t);
    const uint32_t chunk2_size  = data_bytes;
    const uint32_t chunk1_size  = 36 + chunk2_size;

    auto w32 = [&](uint32_t v) { f.write(reinterpret_cast<const uint8_t*>(&v), 4); };
    auto w16 = [&](uint16_t v) { f.write(reinterpret_cast<const uint8_t*>(&v), 2); };

    f.write(reinterpret_cast<const uint8_t*>("RIFF"), 4);
    w32(chunk1_size);
    f.write(reinterpret_cast<const uint8_t*>("WAVEfmt "), 8);
    w32(16);            // PCM sub-chunk size
    w16(1);             // PCM format
    w16(1);             // mono
    w32(AUDIO_SAMPLE_RATE);
    w32(byte_rate);
    w16(2);             // block align (1 ch × 2 bytes)
    w16(16);            // bits per sample
    f.write(reinterpret_cast<const uint8_t*>("data"), 4);
    w32(chunk2_size);
}
