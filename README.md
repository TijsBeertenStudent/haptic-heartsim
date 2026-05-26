# HeartSim — Haptic Simulation of Heart Sounds for Cardiac Auscultation Training

This repository documents the Haptic HeartSim project for KU Leuven's Haptic Interfaces Experience (B-KUL-T4lMD2) in the MSc Industrial Engineering (Electromechanics, Clinical Engineering). Developed by Tijs Beerten and Joppe Baert under Prof. Carlos Rodriguez-Guerrero and TAs Marlon Rodriguez and Ewald Ury.

**Project:** HeartSim — Haptic Simulation of Heart Sounds for Cardiac Auscultation Training
**Authors:** Joppe Baert, Tijs Beerten
**Institution:** KU Leuven Faculty of Engineering Technology — Group T
**Course:** B-KUL-T4lMD2 — Haptic Interfaces Experience
**Supervisor:** Prof. dr. ir. Carlos Rodriguez-Guerrero
**Teaching Assistants:** Marlon Rodriguez, Ewald Ury
**Academic Year:** 2025–2026

---

## 1. Project Summary

HeartSim is a wearable haptic device that simulates the physical vibrations and acoustic signals of human heart sounds, enabling medical students to practise cardiac auscultation without requiring access to real patients. The prototype consists of four modular vibration-and-speaker units mounted on a chest harness at the four standard clinical auscultation points (aortic, pulmonary, tricuspid, mitral). A microcontroller drives the haptic motors via DRV2605L drivers multiplexed over I²C, while audio is generated through I²S amplifiers. The system reproduces both normal heart sounds (S1, S2) and pathological signals (murmurs, gallops, stenosis, regurgitation) synchronised to a measured or simulated heart rate.

---

## 2. Repository Structure

```
Haptic_HeartSim/
├── README.md                       This file (project entry point)
├── .gitignore                      Git exclusions
│
├── HeartSim-Report/
│   ├── 00_introduction/            General project introduction
│   ├── 01_medical_problem/         Clinical background and literature review
│   ├── 02_methodology/             Design methodology and system architecture
│   ├── 03_discussion/              Results, limitations, observations
│   ├── 04_future_work/             Conclusions and next steps
│   └── 05_references/              IEEE-style reference list
│
├── hardware/
│   ├── README.md                   Hardware overview
│   ├── Bill Of Materials/          Bill of Materials (.xlsx + README)
│   ├── cad/                        3D-printable parts (STL/STEP)
│   └── images/                     Hardware photographs
│
├── software/
│   ├── README.md                   Software overview and architecture
│   ├── platformio.ini              PlatformIO build configuration
│   ├── src/                        C++ firmware (Arduino Nano ESP32)
│   ├── data/                       Web interface (HTML/CSS/JS, LittleFS)
│   └── docs/                       PCG model, wiring, BOM, setup guide
│
└── videos/                         Demonstration recordings
```

---

## 3. Quick Navigation

### Written Report

The full academic report is located in [`HeartSim-Report/`](HeartSim-Report/):

| Chapter | Contents |
| ------- | -------- |
| [`00_introduction`](HeartSim-Report/00_introduction/README.md) | Project motivation, problem statement, objectives, scope |
| [`01_medical_problem`](HeartSim-Report/01_medical_problem/README.md) | Cardiac anatomy, heart cycle, heart sounds, auscultation practice, existing training methods |
| [`02_methodology`](HeartSim-Report/02_methodology/README.md) | Conceptual framework, component selection, mechanical and electrical design, software design |
| [`03_discussion`](HeartSim-Report/03_discussion/README.md) | Test results, performance observations, limitations of the prototype |
| [`04_future_work`](HeartSim-Report/04_future_work/README.md) | Conclusions and proposed extensions for future iterations |
| [`05_references`](HeartSim-Report/05_references/README.md) | Full IEEE-style reference list, used consistently across all other documents |

### Hardware

| Resource | Location |
| -------- | -------- |
| Hardware overview | [`hardware/README.md`](hardware/README.md) |
| **Bill of Materials** (component list, prices, suppliers) | [`hardware/Bill Of Materials/`](hardware/Bill%20Of%20Materials/README.md) |
| 3D-printable parts (STL/STEP) | [`hardware/cad/`](hardware/cad/) |
| Hardware photographs | [`hardware/images/`](hardware/images/) |

### Software

| Resource | Location |
| -------- | -------- |
| Software overview and architecture | [`software/README.md`](software/README.md) |
| PCG synthesis model (mathematical background) | [`software/docs/PCG_MODEL.md`](software/docs/PCG_MODEL.md) |
| Wiring diagram | [`software/docs/WIRING.md`](software/docs/WIRING.md) |
| Build and setup guide | [`software/docs/SETUP.md`](software/docs/SETUP.md) |

---

## 4. Terminology Standards

To keep writing consistent across documents, the terms in the left column are the **preferred** form. The right column lists variants to avoid.

### 4.1 Clinical

| Use                       | Avoid                                       |
| ------------------------- | ------------------------------------------- |
| heart sound               | heart noise, cardiac noise                  |
| murmur                    | heart whisper, heart hiss                   |
| gallop                    | extra beat                                  |
| auscultation              | listening with a stethoscope (in headings)  |
| auscultation point        | listening spot, stethoscope position        |
| stenosis                  | valve narrowing (in headings)               |
| regurgitation             | leak, leakage                               |
| cardiac cycle             | heart cycle                                 |
| phonocardiogram (PCG)     | heart sound recording (in headings)         |

### 4.2 Hardware

| Use                              | Avoid                                                   |
| -------------------------------- | ------------------------------------------------------- |
| haptic motor                     | vibration engine, buzzer                                |
| coin (vibration) motor           | flat motor, disc motor                                  |
| LRA (Linear Resonant Actuator)   | linear motor                                            |
| ERM (Eccentric Rotating Mass)    | unbalanced motor                                        |
| speaker                          | loudspeaker (use only when emphasising acoustic output) |
| amplifier                        | sound booster                                           |
| multiplexer                      | switch, selector                                        |
| microcontroller                  | board, chip (use only informally)                       |
| driver                           | controller (ambiguous)                                  |
| chest harness                    | strap, belt                                             |
| control module                   | brain, central unit                                     |

### 4.3 Software and Signals

| Use                    | Avoid                          |
| ---------------------- | ------------------------------ |
| firmware               | embedded code                  |
| signal processing      | sound processing               |
| segmentation           | splitting (in headings)        |





---

## 5. Working Conventions

### 5.1 Section Tone

| Folder                | Tone and content                                                                |
| --------------------- | ------------------------------------------------------------------------------- |
| `HeartSim-Report/`    | Academic writing — literature, methodology, discussion. Prose-heavy.            |
| `hardware/`           | Practical build documentation — tables, diagrams, pin lists, BOM.               |
| `software/`           | Code-focused — block diagrams, function references, build and run instructions. |

### 5.2 Scope Statement

- This is an educational prototype, not a medically certified device.
- It is intended for use in supervised training scenarios only.
- Limitations and known issues are discussed in `HeartSim-Report/03_discussion/README.md`.

---

*Last updated: May 2026. Maintained by Joppe Baert and Tijs Beerten.*
