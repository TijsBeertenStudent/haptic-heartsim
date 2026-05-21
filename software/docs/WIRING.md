# WIRING — Hardware Connections

All ESP32 Nano pins mapped to the components. Refers to `src/config.h` for the exact same values.

## I²C bus (haptic path)

| From | To | Note |
|---|---|---|
| ESP32 A4 (SDA) | TCA9548A SDA → SDA of 4× DRV2605L | I²C data, 4.7 kΩ pull-up to 3.3 V |
| ESP32 A5 (SCL) | TCA9548A SCL → SCL of 4× DRV2605L | I²C clock, 4.7 kΩ pull-up to 3.3 V |
| 3.3 V | TCA9548A VCC, DRV2605L VCC | Logic supply |
| GND | TCA9548A GND, DRV2605L GND | Common ground |

TCA9548A channels:
- SD0/SC0 → DRV2605L module 0 (Aortic)
- SD1/SC1 → DRV2605L module 1 (Pulmonic)
- SD2/SC2 → DRV2605L module 2 (Tricuspid)
- SD3/SC3 → DRV2605L module 3 (Mitral)

## Haptic PWM path

| From | To | Note |
|---|---|---|
| ESP32 D13 | IN/PWM of ALL 4 DRV2605L drivers in parallel | One pin → 4 inputs — DRV decides via I²C which output is active |

Per module — **NO external MOSFET, NO external diode**:

```
DRV2605L OUT+ ──┬── motor 1 + ── motor 1 -
                ├── motor 2 + ── motor 2 -
                └── motor 3 + ── motor 3 -
DRV2605L OUT- ──┘ (motors in parallel between OUT+ and OUT-)
```

The DRV2605L has a **built-in H-bridge** and **internal flyback protection** for inductive loads. The coin motors are connected directly between `OUT+` and `OUT-` of the driver.

**Current limit**: the DRV2605L delivers up to ±250 mA continuous. With 2–3 coin motors in parallel (60–90 mA per motor) this stays within the limit. For 1 motor per module there is no issue at all.

## I²S audio path — 2 amplifiers, 2 speakers

We have physically **2 MAX98357A amplifiers** and **2 speakers**. The 4 auscultation points are mapped onto these 2 speakers:

| Speaker # | Covers points | PWM gain pin |
|---|---|---|
| Speaker 0 (upper) | Aortic + Pulmonic | ESP32 D2 |
| Speaker 1 (lower) | Tricuspid + Mitral | ESP32 D3 |

In `audio_engine.cpp` the 4 spatial weights are averaged per pair:
- Speaker 0 volume = (Aortic_weight + Pulmonic_weight) / 2
- Speaker 1 volume = (Tricuspid_weight + Mitral_weight) / 2

| From | To | Note |
|---|---|---|
| ESP32 D7 (BCK) | BCLK of both MAX98357 | Bit clock |
| ESP32 D8 (WS) | LRC of both MAX98357 | Word select (left/right) |
| ESP32 D9 (DOUT) | DIN of both MAX98357 | Data — both amplifiers receive the same audio |
| 5 V | VIN of both MAX98357 | Amplifier power supply |
| GND | GND of both MAX98357 | Common ground |

## Per-speaker volume control

Per MAX98357 we drive the SD/GAIN pin (mute/volume) with PWM through a simple RC low-pass filter:

```
ESP32 PWM ── 10 kΩ ──┬── MAX98357 SD/GAIN
                     │
                    1 µF
                     │
                    GND
```

5 kHz PWM, 8-bit (0..255), RC = 10 ms time constant → clean DC level.

| MAX98357 # | ESP32 PWM pin | Speaker position |
|---|---|---|
| 0 | D2 | Upper (Aortic + Pulmonic) |
| 1 | D3 | Lower (Tricuspid + Mitral) |

**Voltage table** (from MAX98357A datasheet):
- < 0.16 V: shutdown (mute)
- 0.16–0.77 V: left channel only
- 0.77–1.40 V: right channel only
- > 1.40 V: stereo averaged (mono full)

Since we run in mono, only the "> 1.40 V" region is relevant. PWM=128 (= 1.65 V at 3.3 V) → already full gain. PWM < 40 → mute. Our code uses 0=mute, 180=normal, 255=max.

## PPG sensor

| From | To | Note |
|---|---|---|
| Sensor analogue out | ESP32 A0 | 0..3.3 V signal |
| Sensor VCC | 3.3 V | |
| Sensor GND | GND | |

## Power

- USB-C → ESP32 Nano VIN
- ESP32 3V3 pin → 3.3 V rail (logic, DRV2605L, TCA9548A)
- MB102 on a separate breadboard rail supplies 5 V for the speakers (otherwise USB 5 V sags during audio peaks)
- At each MAX98357: 100 nF ceramic + 470 µF electrolytic across VIN–GND for current spikes

## Table: all ESP32 Nano pins in use

| Pin label | GPIO | Function |
|---|---|---|
| A0 | GPIO1 (ADC1_CH0) | PPG analogue input |
| A4 | GPIO11 | I²C SDA (haptics) |
| A5 | GPIO12 | I²C SCL (haptics) |
| D2 | GPIO5 | PWM gain MAX98357 upper speaker |
| D3 | GPIO6 | PWM gain MAX98357 lower speaker |
| D7 | GPIO9 | I²S BCLK |
| D8 | GPIO10 | I²S WS/LRC |
| D9 | GPIO17 | I²S DOUT |
| D13 | GPIO48 | DRV2605L PWM input (shared) |

**Note**: the Nano ESP32 labelling (`D2..D13`, `A0..A7`) differs from the underlying GPIO numbers. The GPIO numbers above are for reference; in code we use the Arduino-style `A0`, `13`, etc. and the board variant handles the mapping.

D4 and D5 are currently **free** — potentially available for future expansion.
