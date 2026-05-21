# CONTEXT.md

**Project:** HeartSim — Haptic Simulation of Heart Sounds for Cardiac Auscultation Training
**Authors:** Joppe Baert, Tijs Beerten
**Institution:** KU Leuven — Faculty of Engineering Technology — Group T
**Course:** B-KUL-T4lMD2 — Haptic Interfaces Experience
**Supervisor:** Prof. dr. ir. Carlos Rodriguez-Guerrero
**Teaching Assistants:** Marlon Rodriguez, Ewald Ury
**Academic Year:** 2025–2026

---

## 1. Purpose of This Document

This file is the master reference for everyone working on the HeartSim repository. It defines:

- the project scope and objectives,
- the structure and contents of every section of the repository,
- the editorial, citation, and formatting conventions applied across all files.

When in doubt about terminology, a heading style, or where to place new content, consult this file first.

---

## 2. Project Summary

HeartSim is a wearable haptic device that simulates the physical vibrations and acoustic signals of human heart sounds, enabling medical students to practise cardiac auscultation without requiring access to real patients. The prototype consists of four modular vibration-and-speaker units mounted on a chest harness at the four standard clinical auscultation points (aortic, pulmonary, tricuspid, mitral). A microcontroller drives the haptic motors via DRV2605L drivers multiplexed over I²C, while audio is generated through I²S amplifiers. The system reproduces both normal heart sounds (S1, S2) and pathological signals (murmurs, gallops, stenosis, regurgitation) synchronised to a measured or simulated heart rate.

---

## 3. Repository Structure

```
Haptic_HeartSim/
├── CONTEXT.md                      This file (project entry point)
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
│   ├── Bill Of Materials/           Bill of Materials (.xlsx + README)
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

## 4. Section Contents

This table describes what each documentation file is expected to contain.

| File                                  | Contents                                                                                       |
| ------------------------------------- | ---------------------------------------------------------------------------------------------- |
| `HeartSim-Report/00_introduction/README.md`      | Project motivation, problem statement, objectives, scope                                       |
| `HeartSim-Report/01_medical_problem/README.md`   | Cardiac anatomy, heart cycle, heart sounds, auscultation practice, existing training methods   |
| `HeartSim-Report/02_methodology/README.md`       | Conceptual framework, component selection, mechanical and electrical design, software design   |
| `HeartSim-Report/03_discussion/README.md`        | Test results, performance observations, limitations of the prototype                           |
| `HeartSim-Report/04_future_work/README.md`       | Conclusions and proposed extensions for future iterations                                      |
| `HeartSim-Report/05_references/README.md`        | Full IEEE-style reference list, used consistently across all other documents                   |
| `hardware/README.md`                  | System overview: modules, central control unit, electrical architecture                        |
| `hardware/Bill Of Materials/README.md` | Bill of Materials notes — purchase sources, quantities, costs                                  |
| `software/README.md`                  | Software architecture, repository layout, build and run instructions                           |
| `CONTEXT.md`                          | Repository entry point: project summary, structure, conventions                                 |

---

## 5. Terminology Standards

To keep writing consistent across documents, the terms in the left column are the **preferred** form. The right column lists variants to avoid.

### 5.1 Clinical

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

### 5.2 Hardware

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

### 5.3 Software and Signals

| Use                    | Avoid                          |
| ---------------------- | ------------------------------ |
| firmware               | embedded code                  |
| signal processing      | sound processing               |
| segmentation           | splitting (in headings)        |

---

## 6. Editorial Conventions

### 6.1 Language

- All repository content is written in **British English** (en-GB).
- Decimal separator: **point** (e.g. `3.7 V`), not comma.
- Use the Oxford comma in lists of three or more items.

### 6.2 Numbering and Headings

- Use hierarchical numbering: `1`, `1.1`, `1.1.1`. Do not exceed four levels.
- Markdown headings: `#` for the title, `##` for major sections (numbered), `###` for subsections.
- Every README opens with an H1 title that matches the directory it lives in.

### 6.3 Figures and Tables

- Numbered sequentially within each document: `Figure 1`, `Figure 2`, `Table 1`.
- Caption format: `*Figure 1 — Short description of what is shown.*`
- Figures stored in the `images/` folder adjacent to the README.
- Reference figures in prose as "see Figure 1" (not "fig. 1" or "below").

### 6.4 Citations

- IEEE numeric style: `[1]`, `[2]`, multiple `[1], [3], [5]`, or ranges `[1]–[4]`.
- The full reference list lives in `HeartSim-Report/05_references/README.md`.
- Every other README that cites a source uses the same numbering as that master list.
- When adding a new reference, append it to the master list and use the next available number — never renumber existing entries.

### 6.5 Units

- SI units throughout, with a thin space between value and unit (`3.0 V`, `70 mA`, `200 Hz`, `10 mm`).
- Exception: percentages — no space (`50%`).
- Frequencies in Hz / kHz; currents in mA / A; voltages in V; power in mW / W.

### 6.6 Code and File References

- Code blocks tagged with language identifier: ` ```cpp `, ` ```python `, ` ```bash `.
- File paths in inline code: `hardware/Bill Of Materials/bill_of_materials.xlsx`.
- Variable names in code style: `i_module`, `u_speaker`.

### 6.7 Cross-references

- Between README files, use relative Markdown links:
  `[see Methodology](../02_methodology/README.md)`.
- Within a single document, link to a heading anchor:
  `[see §2.3](#23-control-of-the-motors)`.

---

## 7. Working Conventions

### 7.1 Section Tone

| Folder                | Tone and content                                                                |
| --------------------- | ------------------------------------------------------------------------------- |
| `HeartSim-Report/`    | Academic writing — literature, methodology, discussion. Prose-heavy.            |
| `hardware/`           | Practical build documentation — tables, diagrams, pin lists, BOM.               |
| `software/`           | Code-focused — block diagrams, function references, build and run instructions. |
| Top-level `README.md` | Repository entry point — concise overview, navigation, headline figure.         |

### 7.2 Scope Statement

- This is an educational prototype, not a medically certified device.
- It is intended for use in supervised training scenarios only.
- Limitations and known issues are discussed in `HeartSim-Report/03_discussion/README.md`.

---

*Last updated: May 2026. Maintained by Joppe Baert and Tijs Beerten.*
