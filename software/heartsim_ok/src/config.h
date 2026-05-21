// ============================================================================
// config.h — Central configuration for HeartSim
// ----------------------------------------------------------------------------
// All hardware pins, sample rates, clinical timing constants and FreeRTOS
// priorities are collected in this single file. Only #defines and constexpr
// values, no code — this lets us reconfigure the simulator (e.g. change a
// pin assignment) by editing one place and recompiling.
//
// References:
//   - DRV2605L datasheet (Texas Instruments, 2014): SLOS854B
//   - MAX98357A datasheet (Maxim/Analog Devices)
//   - ESP32-S3 Technical Reference Manual, chapter on ADC + I2S
//   - Phonocardiogram theory: see docs/PCG_MODEL.md
// ============================================================================
#pragma once

#include <Arduino.h>

// ============================================================================
// I2C BUS — Haptic drivers + I2C multiplexer
// ----------------------------------------------------------------------------
// The Arduino Nano ESP32 maps SDA → A4 and SCL → A5 by default, so a plain
// Wire.begin() works without pin overrides. We run the bus at 400 kHz
// (Fast-mode I2C) which is well within the DRV2605L and TCA9548A specs.
//
// All four DRV2605L drivers share the same default I2C address (0x5A). The
// TCA9548A multiplexer isolates them on separate channels (0..3) so we can
// address them individually without re-wiring jumpers.
// ============================================================================
constexpr uint8_t  TCA9548A_ADDR = 0x70;     // I2C address of the multiplexer
constexpr uint8_t  DRV2605_ADDR  = 0x5A;     // Default DRV2605L I2C address
constexpr uint32_t I2C_FREQ      = 400000;   // 400 kHz Fast-mode

// Four haptic modules — one per auscultation point.
// Channel order corresponds to clinical auscultation positions:
//   0 = Aortic     (2nd intercostal space, right sternal border)
//   1 = Pulmonic   (2nd intercostal space, left sternal border)
//   2 = Tricuspid  (4th intercostal space, left sternal border)
//   3 = Mitral     (5th intercostal space, midclavicular line — "apex")
constexpr uint8_t NUM_MODULES = 4;
enum AuscultationPoint : uint8_t {
    POINT_AORTIC    = 0,
    POINT_PULMONIC  = 1,
    POINT_TRICUSPID = 2,
    POINT_MITRAL    = 3
};

// HAPTIC_PWM_PIN drives the PWM/IN input of ALL four DRV2605L drivers in
// parallel. Only the driver whose channel is currently enabled on the
// TCA9548A actually receives I2C commands; the PWM input itself is shared.
//
// Each DRV2605L drives its coin motors directly through its built-in
// H-bridge (rated ~250 mA). Up to three motors in parallel is feasible
// without external MOSFETs or flyback diodes because the chip has internal
// flyback protection for inductive loads.
constexpr uint8_t HAPTIC_PWM_PIN = 13;

// ============================================================================
// I2S AUDIO — 2 × MAX98357A class-D amplifiers
// ----------------------------------------------------------------------------
// Despite the 4 auscultation points, we use only 2 physical speakers. The
// firmware combines them into two pairs:
//   Speaker 0 (upper)  = Aortic   + Pulmonic   (averaged spatial weight)
//   Speaker 1 (lower)  = Tricuspid + Mitral    (averaged spatial weight)
//
// A single I2S bus feeds both amplifiers in parallel (mono signal). Per-
// speaker volume is controlled by feeding a PWM duty cycle through an RC
// low-pass filter into each MAX98357's SD/GAIN pin. The chip interprets the
// resulting DC voltage as either MUTE (<0.16 V), single-channel (0.16-1.4 V)
// or mono full (>1.4 V).
// ============================================================================
constexpr uint8_t NUM_AUDIO_MODULES = 2;

constexpr uint8_t I2S_BCK_PIN = 7;   // Bit Clock
constexpr uint8_t I2S_WS_PIN  = 8;   // Word Select / LRC
constexpr uint8_t I2S_DO_PIN  = 9;   // Data Out (DIN)

// One PWM pin per speaker → 10 kΩ resistor + 1 µF capacitor → SD pin.
// 0 = mute, 255 = ≈3.3 V (full mono gain at the MAX98357's default 9 dB).
constexpr uint8_t MAX98357_GAIN_PINS[NUM_AUDIO_MODULES] = {
    2,   // Speaker 0 — Aortic + Pulmonic
    3    // Speaker 1 — Tricuspid + Mitral
};

// 22050 Hz is sufficient bandwidth for heart sounds — virtually all PCG
// spectral energy lies below 600 Hz, and the Nyquist limit (11 kHz) leaves
// plenty of margin. A smaller rate keeps DSP cost and I2S DMA bandwidth low.
constexpr uint32_t AUDIO_SAMPLE_RATE = 22050;
constexpr uint16_t AUDIO_BLOCK_SIZE  = 256;   // Samples per I2S DMA buffer

// ============================================================================
// PPG SENSOR — DFRobot SEN020 (photoplethysmography on a fingertip)
// ----------------------------------------------------------------------------
// Analog output is sampled on GPIO A0 (= ADC1 channel 0). The ESP32-S3 ADC
// has 12-bit resolution (0..4095). We oversample at 250 Hz which gives
// plenty of headroom for the 0.5-5 Hz heart-rate band after band-pass
// filtering.
// ============================================================================
constexpr uint8_t  PPG_ADC_PIN        = A0;
constexpr uint32_t PPG_SAMPLE_RATE    = 250;   // Hz
constexpr float    PPG_HP_CUTOFF      = 0.5f;  // High-pass cutoff (remove DC drift)
constexpr float    PPG_LP_CUTOFF      = 5.0f;  // Low-pass cutoff (heart-rate band)
constexpr uint32_t PPG_REFRACTORY_MS  = 300;   // Minimum gap between accepted peaks
                                               // (prevents counting the dicrotic
                                               //  notch as a second beat).

// Pulse Transit Time (PTT) — the time it takes for a pressure wave to
// travel from the heart to the fingertip. A fingertip PPG peak therefore
// lags the cardiac S1 sound by roughly 150-250 ms (depends on age, height,
// blood pressure). The CycleEngine compensates by predicting the next S1
// as (next_predicted_PPG_peak − PTT_OFFSET). The student then hears the
// simulated heart sound at the *correct* moment relative to a real S1.
constexpr uint16_t PPG_PTT_OFFSET_MS  = 200;   // ms, tunable via web UI (0..400)

// ============================================================================
// WIFI ACCESS POINT — self-hosted (no router required)
// ============================================================================
constexpr const char* WIFI_AP_SSID = "HeartSim";
constexpr const char* WIFI_AP_PASS = "heartsim2026";
constexpr uint8_t     WIFI_AP_CHAN = 6;        // 2.4 GHz channel

// ============================================================================
// FREERTOS TASK PRIORITIES AND CORE ASSIGNMENTS
// ----------------------------------------------------------------------------
// We pin tasks to specific cores to keep timing predictable:
//   Core 0 — sensors, cycle scheduling, WiFi/HTTP, telemetry
//   Core 1 — dedicated to audio DSP + I2S streaming
// Audio runs at high priority because any starvation of the I2S DMA buffer
// produces an audible click. The telemetry task is low priority — late
// packets are visually unnoticeable.
// ============================================================================
constexpr UBaseType_t TASK_PRIO_AUDIO     = 5;   // Highest
constexpr UBaseType_t TASK_PRIO_PPG       = 4;
constexpr UBaseType_t TASK_PRIO_TELEMETRY = 2;
constexpr BaseType_t  CORE_AUDIO          = 1;
constexpr BaseType_t  CORE_SENSOR         = 0;

// ============================================================================
// CARDIAC CYCLE PARAMETERS — from clinical PCG literature
// ----------------------------------------------------------------------------
// Typical durations of the four heart sounds (see docs/PCG_MODEL.md for
// the references). These set the envelope durations of the damped sinusoids
// used by HeartSoundSynthesizer.
// ============================================================================
constexpr uint16_t S1_DURATION_MS = 100;   // S1 ("lub") typically 70-150 ms
constexpr uint16_t S2_DURATION_MS = 80;    // S2 ("dub") typically 60-120 ms
constexpr uint16_t S3_DURATION_MS = 60;    // S3 early diastolic, 40-100 ms
constexpr uint16_t S4_DURATION_MS = 50;    // S4 presystolic, 40-80 ms

// Fraction of the RR interval occupied by systole (S1 → S2). At 60 BPM
// systole lasts ≈350 ms; at 120 BPM ≈170 ms. The ratio shortens slightly
// at high heart rates in reality, but a fixed 35 % is a clinically
// acceptable approximation for simulation purposes.
constexpr float SYSTOLE_FRACTION = 0.35f;

// Fundamental centre frequencies of each heart sound (Hz). S1 is dominated
// by mitral + tricuspid valve closure (M1 + T1); S2 by aortic + pulmonic
// closure (A2 + P2). S3 and S4 are lower-frequency ventricular and atrial
// vibrations respectively.
constexpr float S1_FREQ_HZ = 50.0f;
constexpr float S2_FREQ_HZ = 70.0f;
constexpr float S3_FREQ_HZ = 35.0f;
constexpr float S4_FREQ_HZ = 25.0f;
