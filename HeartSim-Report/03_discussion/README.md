# Discussion

<!-- src:DA-P0.1, DA-P0.2, DA-P0.3 — Title block -->

## 1. Introduction

<!-- src:DA-P1.1 -->

The aim of this project was to develop a wearable system that allows medical students to practise cardiac auscultation not on a static mannequin but on a living volunteer. This chapter interprets the observations from the test phase, assesses to what extent our prototype solves this clinical problem, and discusses both the unexpected technical limitations and the user experience as reported by medical experts in training and in practice.

## 2. Evaluation Against the Original Challenge

<!-- src:DA-P2.1 -->

The original medical challenge — providing repeatable exposure to pathological heart sounds without depending on expensive mannequins or incidental patient contacts — is addressed by our prototype on most fronts. The haptic harness with four modules at the classical auscultation points (aortic, pulmonary, tricuspid, mitral), combined with real-time PPG heart-rate measurement and a web interface that lets the instructor choose pathology type and severity level live, delivers a functional and interactive training platform for under 200 euro per set-up. The synchronisation between the measured heart rate of the healthy volunteer and the simulated pathology runs stably, and the severity slider (level 1–5) enables a progression from obvious to subtle — precisely the gradation that we identified as didactically valuable from the literature review [2], [9] (see [HeartSim-Report/01_medical_problem](../01_medical_problem/README.md)).

<!-- src:DA-P2.2 -->

With that, the concept itself — transforming a healthy person into a reconfigurable training model — has been validated as a working principle, even though we must qualify this statement with several caveats regarding the ultimate degree of realism and the hardware robustness, as discussed in the sections below.

## 3. Unexpected Technical Limitation: Failure of the Audio Amplifiers

<!-- src:DA-P3.1 -->

The most significant unexpected limitation during the test phase was the failure of both MAX98357A I²S amplifiers [25]. During an extended test session the speakers stopped producing output. Initially we suspected a software bug and invested considerable time in code debugging. Only after ruling out firmware causes did it become clear that the fault was in the hardware: the amplifiers did not feel warm to the touch — which under normal operation at load would be expected due to dissipation losses — but simply produced no output whatsoever.

<!-- src:DA-P3.2 -->

On the basis of the electrical characteristics described in the methodology (see [HeartSim-Report/02_methodology §4.2](../02_methodology/README.md)), we can technically substantiate the most likely cause of failure. The MAX98357A specifies a peak output current of 1.4 A at full power and was set to its maximum gain (15 dB) via the SD/GAIN pin during various tests. Our 8 ohm speakers have a nominal power rating of 2 W, corresponding to an RMS current of approximately 0.5 A and peak currents up to roughly 300 mA at transients such as an S1 beat. When high-amplitude signals were repeatedly sent — especially when two speakers were connected in parallel or in stereo mode to a single amplifier, as in our set-up — the combined load may have intermittently crept closer to that 1.4 A limit than budgeted. We suspect that a combination of prolonged operation at high gain, the absence of external current limiting, and possibly a transient on the 5 V rail of the MB102 power supply irreversibly damaged the internal output stage of the class-D amplifier.

<!-- src:DA-P3.3, DA-P3.4, DA-P3.5, DA-P3.6 -->

Concrete preventive measures that we would incorporate in a next iteration:

1. A series current-limiting resistor or an N-channel MOSFET with flyback protection between the amplifier and the speaker.
2. Speakers with a higher impedance (e.g. 16 ohm) to halve the peak current, and setting the default GAIN to 9 dB rather than 15 dB.
3. A switchable supply protection on the 5 V rail.

Due to time constraints we were unable to replace the amplifiers or order additional components before the project deadline.

<!-- src:DA-P3.7 -->

The practical consequence for the tests is that we had to carry out the final validation using only the haptic subsystem (the DRV2605L-driven coin motors [24]), without combined audio. This limits the direct comparability with clinical auscultation, since a stethoscope in practice primarily picks up an acoustic signal. At the same time, this is also an instructive observation: the haptic component alone turns out — under the conditions discussed below — to be recognisable enough for medically trained test subjects to distinguish pathology patterns.

## 4. User Experience and Feedback

<!-- src:DA-P4.0 -->

Since we as designers do not have a clinically trained ear for cardiac pathologies — our reference is based on the literature review (see [HeartSim-Report/01_medical_problem](../01_medical_problem/README.md)) and on recordings from PhysioNet [6], Thinklabs [13], and the Littmann CORE library [10] — we relied on three test subjects with clinical experience for the user validation: two medical students and one retired general practitioner. They tested the prototype independently and without prior knowledge of the chosen pathology.

### 4.1 Positive Findings

<!-- src:DA-P4.1.1 -->

**Realism of the concept.** All three test subjects were pleasantly surprised by the overall result and clearly saw clinical potential in the concept. They confirmed that the idea of using a living chest — with natural breathing movements, body warmth, and skin compliance — provides a more realistic experience than the static mannequins they had been accustomed to (see also the comparison with existing training solutions in [HeartSim-Report/01_medical_problem §5](../01_medical_problem/README.md)).

<!-- src:DA-P4.1.2 -->

**Web interface and didactic progression.** The interactive web interface was perceived as very convenient. The fact that the instructor can adjust the pathology and severity level in real time made targeted practice possible: the test subjects could first learn at a higher severity level (grade 4–5) which pattern belongs to which pathology, and then step down incrementally to more difficult levels (grade 1–2). This gradual build-up — which we had adopted from the Levine grading in the literature review [2], [9] — was experienced in practice as didactically strong.

<!-- src:DA-P4.1.3 -->

**Heart-rate synchronisation as reinforcement.** A feature that we added fairly late, inspired by the live-BPM read-out of the Littmann CORE stethoscope [10], proved unexpectedly valuable during the tests. Because the modules transmitted the measured heart rate of the volunteer along with the pathology events, the actual heart rhythm was reinforced via haptics and became more clearly perceptible for the student.

<!-- src:DA-P4.1.4 -->

**Usability through a T-shirt.** A related advantage that we only discovered during the tests: because the amplified heartbeat is transmitted mechanically, the signal remains sufficiently perceptible when the volunteer is wearing a thin T-shirt. The signal is admittedly slightly dampened, but the essential characteristic remains recognisable. This lowers the barrier for demonstrations in large lecture groups and during examinations, where baring the torso is not always practical.

### 4.2 Negative Findings

<!-- src:DA-P4.2.1 -->

**Mechanical noise at high amplitude.** When the coin motors are driven at high amplitude, the mechanical noise of the motor itself (rotation and friction of the eccentric mass) becomes audible through the stethoscope. This self-evidently does not sound like a heart sound and detracts from the realism. For the tests we deliberately kept the amplitude below a threshold value, but this limits the maximum achievable "loudness" and makes it difficult to simulate clearly audible grade-4/5 murmurs [2] without the audio channels.

<!-- src:DA-P4.2.2 -->

**Dimensions of the central control module.** The module containing the Arduino Nano ESP32, the breadboard, the MB102 power supply, and the jumper-wire cabling is in its current form too large and too unwieldy to be worn comfortably on a person. We were aware of this beforehand — a prototype phase is by definition not optimised for form factor — but it remains a clear improvement point for a next iteration (see [HeartSim-Report/02_methodology §3](../02_methodology/README.md) for the current mechanical integration).

<!-- src:DA-P4.2.3 -->

**Dimensions of the haptic-acoustic modules.** The four modules themselves (3D-printed housing with speaker and three coin motors) are relatively large. On the chest of an average adult they pose no problems, but in individuals with a narrower thorax or lower intercostal spaces the free space between the modules becomes tight, preventing the stethoscope from always landing exactly on the intended auscultation point. A more compact redesign of the module housing — without compromising the acoustic properties of the speaker chamber — would need to address this.

### 4.3 Limitations of the User Validation

<!-- src:DA-P4.3.1 -->

We are aware of two fundamental limitations of our user validation. First, the sample size is small (n = 3) and not representative of the broader target group of medical students. Second, we as designers lack a clinically calibrated reference to objectively assess the degree of realism; the positive feedback from our test subjects is encouraging but is not the same as a structured validation by cardiologists or a comparative study against mannequin-based training. A more rigorous validation protocol — for example a blinded recognition test in which multiple students must identify the pathology on the basis of both our prototype and existing PhysioNet recordings [6] — would be necessary to quantitatively substantiate the learning value. Within the scope and the time frame of this project, that was not feasible.

## 5. Summary and Interpretation

<!-- src:DA-P5.1 -->

In summary, our prototype addresses the original challenge successfully at the conceptual level: an affordable, interactive, and person-worn alternative to mannequin-based and audio-only training. The core functionality — synchronous control of four modules, configurable pathology via a web interface, gradable severity — works and is perceived as valuable by medically trained test subjects. The realisation remains, however, a prototype: the hardware fault in the amplifiers [25] removed the auditory component from the final tests, the mechanical noise of the motors limits the haptic dynamic range, and the form factor of both the central module and the haptic modules requires further miniaturisation. These observations form the basis for the recommendations in the next chapter (see [HeartSim-Report/04_future_work](../04_future_work/README.md)).
