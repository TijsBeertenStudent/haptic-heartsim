# HeartSim — Haptic Heart Auscultation Simulator

**KU Leuven Master's Project — Joppe Baert & Tijs Beerten (2026)**

A wearable haptic simulator that reproduces a variety of cardiac pathologies on a healthy volunteer through a combination of **vibration motors** (DRV2605L haptic drivers) and **small speakers** (MAX98357A I²S amplifiers) distributed across the four standard auscultation points on the chest. The system is controlled by an **Arduino Nano ESP32** that sets up a WiFi access point, allowing the instructor to operate everything from a phone or laptop via a real-time web interface.

## What does this system do?

1. **Measures** the real heart rate of the volunteer via a PPG sensor (fingertip).
2. **Synchronises** simulated S1/S2/murmurs/extra sounds to that natural rhythm.
3. **Generates** heart sounds in **real time** on the ESP32 itself (no samples!) — based on a mathematical PCG synthesis model from the literature (see `docs/PCG_MODEL.md`). The `severity` slider therefore works truly parametrically.
4. **Drives** the volume and haptic intensity per auscultation point according to **clinically correct spatial weights** (an Aortic Stenosis is loudest at the aortic point, a Mitral Regurgitation at the mitral apex, etc.).
5. **Displays** everything in a mobile-first web interface including a live PPG waveform and BPM.

## Hardware bill of materials

| Component | Quantity | Function |
|---|---|---|
| Arduino Nano ESP32 | 1 | Brain (ESP32-S3, WiFi/BLE) |
| DRV2605L haptic driver | 4 | One per haptic module |
| TCA9548A I²C multiplexer | 1 | Selects between the four DRV drivers |
| MAX98357A I²S DAC + amp | 2 | One per audio module (upper / lower pair) |
| Mini speaker 40 mm 2 W | 2 | Audio output per speaker pair |
| Coin vibration motor 10 × 2.7 mm | up to 12 | 1–3 per haptic module |
| MB102 power supply | 1 | 3.3 V + 5 V rails |
| Essentiel-B chest harness | 1 | Carrier for the four modules |
| 3D-printed module housings | 5 | 4 module shells + 1 Arduino base |

Total material cost: **approximately €155** (see `docs/BOM.md`).

## Wiring overview

```
ESP32 Nano             →  TCA9548A      →  4× DRV2605L (channel 0..3, all at 0x5A)
  A4 (SDA)             →  SDA           →  SDA
  A5 (SCL)             →  SCL           →  SCL

ESP32 pin 13           →  IN/PWM pin of ALL 4 DRV drivers in parallel (DRV selects active via I²C)

DRV OUT+/OUT-          →  1–3 coin motors in parallel (internal H-bridge + flyback)

ESP32 pin 7 (BCK)      →  2× MAX98357 BCLK
ESP32 pin 8 (WS/LRC)   →  2× MAX98357 LRC
ESP32 pin 9 (DIN)      →  2× MAX98357 DIN

ESP32 pin 2 (PWM)      →  MAX98357 #0 SD pin via 10 kΩ + 1 µF RC filter (volume upper)
ESP32 pin 3 (PWM)      →  MAX98357 #1 SD pin via RC filter (volume lower)

PPG sensor analogue out →  ESP32 A0
```

## Build & flash

```bash
# One-time: download Chart.js for offline use
curl -L -o data/chart.min.js \
  https://cdn.jsdelivr.net/npm/chart.js@4.4.6/dist/chart.umd.min.js

# Compile and upload code (USB, first time)
pio run -t upload

# Upload filesystem (HTML/CSS/JS) to LittleFS (USB, first time)
pio run -t uploadfs

# Serial monitor
pio device monitor
```

### OTA — flashing over WiFi (without a USB connection)

Connect your laptop to the **HeartSim** WiFi network, then:

```bash
# Firmware OTA
pio run -t upload --upload-port heartsim.local

# Filesystem OTA
pio run -t uploadfs --upload-port heartsim.local
```

PlatformIO will ask for the OTA password: **`heartsim2026`**

To set the password permanently (so you do not have to type it each time), add this to `platformio.ini` in your build environment:

```ini
upload_protocol = espota
upload_port     = heartsim.local
upload_flags    = --auth=heartsim2026
```

> **Note:** the audio task is automatically stopped just before the flash write begins. The device restarts as soon as the upload is complete; the simulation resumes after the reboot. If the upload fails (authentication error, connection error), the simulation resumes without a restart.

## Usage

1. Fit the volunteer into the chest harness; attach the PPG sensor to a finger.
2. Plug in the Arduino (via USB-C or external 5 V).
3. Wait approximately 5 seconds — the ESP32 creates a WiFi network **HeartSim** with password **heartsim2026**.
4. Connect to this network on your phone or laptop. The OS pop-up should automatically open http://192.168.4.1 (captive portal).
5. In the UI: choose a pathology + severity, press **▶ Start simulation**. Hand the stethoscope to the student who will auscultate.
6. **Back-up for demo day**: open ⚙ top-right and enable **Playback mode** — the simulation then runs on an internal 70 BPM clock, independent of whether the PPG sensor works.

## Architecture overview

```
┌─────────────────┐    250 Hz ISR     ┌────────────────┐
│ PPG Sensor (A0) │ ───────────────▶  │ PPGMonitor     │
└─────────────────┘                   │  - bandpass    │
                                      │  - peak detect │
                                      └────────┬───────┘
                                               │ R-peak event
                                               ▼
┌─────────────────────────────────────────────────────────┐
│                  CycleEngine (core 0)                   │
│                                                          │
│  Per R-peak:                                             │
│    - audio.synthesizer.triggerNewCycle(IBI)             │
│    - schedule S1 burst @ t=0 on all 4 modules           │
│    - schedule S2 burst @ t=systole_ms                   │
│    - schedule S3/S4/snap per pathology                  │
└──────────┬────────────────────────────────────┬─────────┘
           │                                    │
           ▼                                    ▼
┌──────────────────────┐                ┌──────────────────────┐
│ AudioEngine (core 1) │                │ HapticManager (core 0)│
│                      │                │                       │
│  - HeartSoundSynth   │                │  - TCA9548A select    │
│    procedural DSP    │                │  - DRV2605L per mod   │
│  - I2S → 2× MAX98357 │                │  - coin motors direct │
│  - per-mod PWM gain  │                │  - 1–3 motors / mod   │
└──────────────────────┘                └──────────────────────┘

           ▲                                    ▲
           │       REST / WebSocket             │
           └────────────────┬───────────────────┘
                            │
                    ┌───────────────┐
                    │  WebInterface │  AsyncWebServer + WS
                    │   (core 0)    │  serves LittleFS files
                    └───────┬───────┘
                            │ WiFi AP "HeartSim"
                            ▼
                     Phone / Laptop
                       browser → 192.168.4.1
```

## Files

```
heartsim_ok/
├── platformio.ini              ← Build config, libraries, partitions
├── src/
│   ├── main.cpp                ← Top-level setup/loop
│   ├── config.h                ← Pin defines, constants
│   ├── tca9548a.h              ← I²C multiplexer wrapper
│   ├── haptic_manager.{h,cpp}  ← 4× DRV2605L control
│   ├── ppg_monitor.{h,cpp}     ← PPG ADC + bandpass + peak detection
│   ├── heart_sound_synth.{h,cpp} ← Procedural DSP synthesis
│   ├── audio_engine.{h,cpp}    ← I²S output + per-module PWM gain
│   ├── cycle_engine.{h,cpp}    ← Schedules everything on the cardiac cycle
│   ├── recorder.{h,cpp}        ← WAV recorder to LittleFS
│   └── web_interface.{h,cpp}   ← WiFi AP + AsyncWebServer + WebSocket
├── data/                       ← LittleFS content
│   ├── index.html
│   ├── style.css
│   ├── app.js
│   └── chart.min.js            ← Download with curl command above
├── docs/
│   ├── PCG_MODEL.md            ← Mathematical model of the heart sounds
│   ├── BOM.md                  ← Bill of materials (detailed)
│   ├── WIRING.md               ← Wiring diagram
│   ├── SETUP.md                ← Setup guide (PlatformIO)
│   └── DEMO_DAY.md             ← Checklist for the presentation
└── README.md                   ← This file
```

## Troubleshooting

| Problem | Possible cause | Solution |
|---|---|---|
| TCA9548A not found | I²C wiring | Check SDA/SCL pins, 3.3 V power, pull-up resistors |
| DRV on channel X missing | Solder / breadboard | `[HAPTIC]` lines in serial monitor indicate which one |
| No sound | I²S pin conflict | Verify that pins 7/8/9 are free (no UART overlap) |
| BPM stays at 0 | PPG signal too weak | Place sensor on fingertip, not on nail; darken ambient light |
| BPM jumps around | Threshold not settled | Wait 10 s for the envelope detector to adapt |
| WiFi AP visible but no IP | Captive portal not working | Manually open http://192.168.4.1/ in browser |
| Chart stays empty | chart.min.js placeholder | Run curl command from the "Build & flash" section |
| Audio stutters | Audio task not on core 1 | Check serial output `[AUDIO] Task started on core 1` |
| Latency too high (>50 ms) | I²C clock too slow | Increase `I2C_FREQ` in config.h to 1000000 |

## Key design decisions

1. **Procedural audio synthesis instead of WAV files.** The entire mathematical model is in `heart_sound_synth.cpp`. Advantages: the severity slider works parametrically, no megabytes of flash storage needed, and scientifically much stronger (the literature speaks of PCG models with ODEs — our approach is the discrete-time implementation of these).

2. **Spatial weights per pathology.** An Aortic Stenosis is loudest at the aortic point; a Mitral Regurgitation at the mitral apex. In `audio_engine.cpp` each pathology has a 4-element weight vector that determines how much volume each speaker gets. This makes the system clinically credible.

3. **Dual-core scheduling.** Audio on core 1 (constant CPU work for DSP), sensing + scheduling on core 0. Prevents audio glitches.

4. **PPG timing offset.** The PPG peak arrives **after** the real S1 (pulse transit time, peripheral pulse). For a training simulator this is acceptable because the student hears what we play, but for scientific transparency this should be noted in the report discussion. A configurable `PPG_PTT_OFFSET_MS` is available in `cycle_engine.cpp`.

5. **Playback mode.** Indispensable as a demo back-up if the PPG sensor fails.

## Web UI

> **Screenshot placeholder** — add `docs/screenshot_ui.png` after the first demo run.
>
> The interface is a mobile-first single-page app served from LittleFS at `192.168.4.1`.
> It includes a live PPG waveform chart, pathology selector with Cardiac/Pulmonary tabs,
> severity slider, per-module haptic controls, master audio/haptic sliders, recording
> section, and a ⚙ Settings panel with language switcher (NL/EN/FR), Playback mode
> toggle, and PTT offset adjustment.

## Demo video

> **Link placeholder** — add YouTube/KU Leuven Mediasite URL here after the presentation.

## Outstanding items

- [x] OTA update support — flash over WiFi without a USB connection (`heartsim.local`)
- [x] Pulmonary sounds — crackles (fine/coarse) and wheezes (inspiratory/expiratory)
- [x] PTT correction — S1 timing corrected for PPG pulse transit time
- [x] Recording mode — WAV capture to LittleFS for spectral validation
- [x] Multi-client operator lock — first connection has full control
- [x] i18n — NL / EN / FR language switcher in ⚙ Settings
- [x] Diagnostics overlay — triple-tap BPM for heap/stack/IBI info
- [x] Demo Day checklist — see `docs/DEMO_DAY.md`
- [ ] Refinement of murmur envelopes based on PhysioNet Challenge 2016 spectra
- [ ] Patient-specific PPG offset auto-calibration

## References

See `docs/PCG_MODEL.md` for the scientific underpinning. Key references:
- Jabloun M. et al. (2013). *A generating model of realistic synthetic heart sounds.*
- Schmidt S.E. et al. PhysioNet/CinC Challenge 2016 PCG dataset.
- 3M Littmann Heart & Lung Sounds Library.
- University of Michigan Heart Sound & Murmur Library.
