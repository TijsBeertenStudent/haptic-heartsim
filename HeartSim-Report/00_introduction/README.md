# Introduction

## 1. Motivation

Cardiac auscultation — the clinical technique of listening to heart sounds with a stethoscope — remains one of the most widely used, cost-effective, and immediately available diagnostic tools in medicine. Despite the rise of advanced imaging modalities such as echocardiography, the stethoscope continues to be the workhorse of bedside examination because it is non-invasive, portable, and deployable in every clinical environment. Correctly identifying pathological sounds, however, demands years of supervised practice and exposure to a diverse patient population [1], [2]. In current medical curricula this requirement is increasingly difficult to meet: studies consistently report a decline in the auscultation proficiency of graduating physicians, driven by time pressure in clinical placements, reduced patient availability for repeated examination, and a growing reliance on imaging [3]. Students learn to auscultate through three principal channels — recorded audio fragments listened to via headphones, high-fidelity simulation mannequins, and real patient contact — yet each of these channels carries fundamental limitations in terms of cost, accessibility, tactile realism, or repeatability.

## 2. Problem Statement

No currently available training solution combines all four properties that a well-designed auscultation trainer should offer: the scalability and repeatability of audio recordings, the spatial and tactile realism of a mannequin, the immediate quantitative feedback of a digital stethoscope, and the natural compliance of a living human chest wall. High-fidelity mannequins such as Harvey and SimMan approach clinical realism but cost upwards of €30 000, making frequent individual practice impossible for the vast majority of programmes [3]. Pure audio libraries are freely accessible but convey no positional information and elicit no haptic response from the learner. Wearable, multi-point, haptic-and-audio cardiac simulators that can be fitted onto a healthy volunteer and configured in real time are absent from the market. HeartSim addresses precisely this gap: it provides an affordable, wearable, and reconfigurable platform that reproduces both the acoustic and vibrotactile signature of normal and pathological heart sounds at the four standard clinical auscultation points, on a living chest wall, under the direct supervision of a trainer.

## 3. Objectives

The project pursues the following goals:

1. **Reproduce normal heart sounds** — generate accurate timing, frequency content, and amplitude for S1 and S2, synchronised to the real-time heart rate of a healthy volunteer measured by a photoplethysmography (PPG) sensor.
2. **Simulate key pathological signals** — reproduce aortic stenosis, mitral regurgitation, mitral stenosis, aortic regurgitation, and S3/S4 gallop rhythms with configurable severity.
3. **Cover all four standard auscultation points** — mount independent haptic modules at the aortic, pulmonary, tricuspid, and mitral areas of the chest, each producing point-specific intensity profiles consistent with clinical reference data.
4. **Provide a real-time trainer interface** — allow a supervisor to select the pathology type and severity level via a web interface without interrupting the auscultation session.
5. **Remain affordable and reproducible** — keep total hardware cost below €250 per set-up using commercially available components, so that multiple simultaneous training stations are financially feasible.
6. **Document the full system** — provide open-source firmware, wiring diagrams, a bill of materials, and this documentation so that the design can be replicated or extended by future student cohorts.

## 4. Scope

**In scope:**
- A single working prototype consisting of four haptic modules mounted on a commercial chest harness.
- Simulation of the normal cardiac cycle (S1, S2) and five pathological presentations: aortic stenosis (AS), mitral regurgitation (MR), mitral stenosis (MS), aortic regurgitation (AR), and gallop rhythms (S3, S4).
- A severity slider with five levels per pathology, mapped to clinically defined intensity grades.
- Synchronisation to the live heart rate of the volunteer via a PPG sensor on the fingertip.
- A web-based trainer interface for real-time control of pathology type and severity.

**Out of scope:**
- Clinical validation or regulatory certification of the device as a medical training tool.
- Simulation of rare or exotic pathological sounds beyond the five conditions listed above.
- Dynamic auscultation manoeuvres (Valsalva, handgrip, positional changes) — these are identified as a priority for future iterations (see [HeartSim-Report/04_future_work/README.md](../04_future_work/README.md)).
- Erb's point as a separate sixth simulation channel; aortic regurgitation signals are routed primarily through the aortic and mitral modules.
- Multi-patient anatomical diversity; the prototype is sized for a single volunteer and does not account for variations in body habitus.

## 5. Document Overview

The repository is organised as follows:

- **[`HeartSim-Report/01_medical_problem/`](../01_medical_problem/README.md)** — clinical background: cardiac anatomy, the heart cycle, heart sounds, auscultation practice, existing training solutions, and the state of the art in cardiac simulation.
- **[`HeartSim-Report/02_methodology/`](../02_methodology/README.md)** — design and implementation: component selection, mechanical and electrical architecture, firmware design, and the audio-to-haptic mapping strategy.
- **[`HeartSim-Report/03_discussion/`](../03_discussion/README.md)** — results and limitations: observations from prototype testing, known hardware and software deficiencies, and a critical evaluation of the simulation quality.
- **[`HeartSim-Report/04_future_work/`](../04_future_work/README.md)** — conclusions and proposed next iterations: dynamic manoeuvres, improved actuators, clinical evaluation, and extended pathology coverage.
- **[`hardware/`](../../hardware/README.md)** — bill of materials, wiring diagrams, schematics, and 3D-printable mechanical parts.
- **[`software/`](../../software/README.md)** — Arduino Nano ESP32 firmware (PlatformIO), procedural heart sound synthesis, and the web-based trainer interface, with build and run instructions.
