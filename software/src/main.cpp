// ============================================================================
// main.cpp — HeartSim top-level entry point
// ============================================================================
// PROJECT OVERVIEW
// ----------------------------------------------------------------------------
// HeartSim is a wearable haptic + acoustic cardiac auscultation simulator.
// It measures a subject's real heart rate via a PPG sensor and synthesises
// realistic heart sounds + tactile vibrations that follow that rhythm. The
// instructor (or examiner) selects a pathology via a web-interface on their
// phone — the system then reproduces the corresponding acoustic and tactile
// features so that a student can train auscultation skills on a living person
// who appears to have any chosen cardiac condition.
//
// MODULE OVERVIEW — what each subsystem does
// ----------------------------------------------------------------------------
//   PPGMonitor      Reads photoplethysmography (PPG) signal on ADC pin A0 at
//                   250 Hz (timer ISR sets flag, main loop reads + filters).
//                   Outputs: live BPM, IBI, latest filtered sample.
//
//   HapticManager   Controls up to 4 DRV2605L haptic drivers via an I2C
//                   multiplexer (TCA9548A). Each driver receives a PWM signal
//                   on D13 and drives 1-3 coin vibration motors directly
//                   (the DRV2605L has an internal H-bridge and flyback
//                   protection, so no external MOSFET or diode is required).
//
//   AudioEngine     Streams 16-bit mono PCM audio at 22050 Hz through the
//                   ESP32-S3 I2S peripheral to 2 MAX98357A class-D amplifiers.
//                   Per-speaker volume is adjusted via PWM → RC low-pass →
//                   SD-pin of each amp. Hosts the WAV-recorder.
//
//   HeartSoundSynth Procedural real-time DSP synthesizer. Generates S1/S2
//                   as damped sinusoids plus pathology-specific murmurs
//                   (bandpass-filtered noise with a per-pathology envelope).
//                   Pulmonary modes synthesise crackles (HP-filtered noise
//                   bursts) and wheezes (frequency-modulated sinusoids).
//
//   CycleEngine     The conductor. On every detected R-peak it triggers a
//                   new cardiac cycle in the synth, schedules haptic S1/S2
//                   pulses on all modules, and adds pathology-specific extras
//                   such as the S3 sound for gallop or the opening snap for
//                   mitral stenosis. Predicts the next S1 using a 4-beat
//                   rolling IBI average minus the configurable PTT offset
//                   (the PPG-fingertip pulse lags the cardiac S1 by ~200 ms).
//
//   WebInterface    AsyncWebServer + WebSocket on a self-hosted WiFi access
//                   point. Mobile-first single-page HMI served from LittleFS.
//                   Two operating modes: live (driven by real PPG) and demo
//                   (operator picks the BPM manually).
//
// DUAL-CORE SCHEDULING
// ----------------------------------------------------------------------------
//   Core 0  Main loop, PPG sample processing, cycle scheduling, WiFi/web
//           server, telemetry task, haptic task.
//   Core 1  Audio task — continuous DSP synthesis + I2S DMA streaming.
//           Kept on a dedicated core so audio never glitches when WiFi
//           traffic spikes on the other core.
// ============================================================================
#include <Arduino.h>
#include "config.h"
#include "haptic_manager.h"
#include "ppg_monitor.h"
#include "audio_engine.h"
#include "cycle_engine.h"
#include "web_interface.h"

// Singleton instances of each subsystem. Wired together via constructor
// arguments so each module can call its dependencies without a service
// locator pattern.
PPGMonitor      g_ppg;
HapticManager   g_haptic;
AudioEngine     g_audio;
CycleEngine     g_cycle(&g_ppg, &g_audio, &g_haptic);
WebInterface    g_web(&g_ppg, &g_cycle, &g_audio, &g_haptic);

// ============================================================================
// setup() — runs once at boot
// ----------------------------------------------------------------------------
// Initialisation order matters:
//   1. Haptic / I2C bus must come up first because subsequent diagnostic
//      messages from the audio and web modules rely on a working Serial
//      output and a stable system clock.
//   2. PPG comes next — its timer ISR starts immediately, but the heavy
//      ADC sampling happens in loop() so there is no risk of crashing
//      before WiFi is up.
//   3. Audio engine initialises the I2S driver and spawns its task on
//      core 1.
//   4. Cycle engine spawns its haptic task and sets a sensible default
//      pathology ("Normal").
//   5. WiFi AP + HTTP server come last so that all sensors and outputs
//      are ready by the time a client connects.
// ============================================================================
void setup() {
    Serial.begin(115200);
    delay(3000);                              // Give serial monitor time to attach
                                              // after a USB-CDC reset.
    Serial.println();
    Serial.println("========================================");
    Serial.println("  HeartSim - Haptic Auscultation Sim");
    Serial.println("  KU Leuven - Joppe Baert & Tijs Beerten");
    Serial.println("========================================");

    // (1) Haptics + I2C mux. Failure is non-fatal: if the breadboard is
    // disconnected we still want the WiFi UI to come up for demo purposes.
    if (!g_haptic.begin()) {
        Serial.println("[MAIN] HAPTIC init failed - check wiring!");
    }

    // (2) PPG ADC sampler — installs the 250 Hz hardware timer.
    g_ppg.begin();

    // (3) I2S audio engine. start() launches the dedicated audio task on
    // core 1; it runs continuously so the on-device WAV recorder can
    // capture samples at any time, even when no cardiac cycle is active.
    g_audio.begin();
    g_audio.start();

    // (4) Cycle engine — spawns the haptic task and primes the default
    // pathology so that "Start simulation" works immediately.
    g_cycle.begin();
    g_cycle.setPathology(PATH_NORMAL, 3);

    // (5) WiFi access point + HTTP/WebSocket server. Comes last so that
    // the first client to connect receives a fully-initialised system.
    g_web.begin();

    Serial.println("[MAIN] Setup complete. Connect to WiFi 'HeartSim'");
    Serial.println("[MAIN] then open http://192.168.4.1");
}

// ============================================================================
// loop() — runs continuously on core 0
// ----------------------------------------------------------------------------
// Three responsibilities per iteration, kept short to maintain the 250 Hz
// PPG sample rate:
//   1. Process any pending PPG sample (flag set by the timer ISR).
//      analogRead() is NOT safe inside an ISR on ESP32-S3, so the ISR
//      only sets a flag and the actual ADC read + filtering happens here.
//   2. Drive the cycle engine, which either fires a new beat on a detected
//      R-peak (live mode) or on its internal clock (demo / playback mode).
//   3. Service ArduinoOTA so firmware updates over WiFi work without us
//      having to block the loop.
// A short delay() yields to FreeRTOS so the other tasks on core 0 (web
// server, telemetry, haptic) get fair time slices.
// ============================================================================
void loop() {
    g_ppg.poll();
    g_cycle.update();
    g_web.handleOTA();
    delay(1);
}
