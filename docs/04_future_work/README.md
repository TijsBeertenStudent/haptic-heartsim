# Conclusion and Future Work

<!-- src:DB-P0.1, DB-P0.2, DB-P0.3 — Title block -->

## 1. Conclusion

### 1.1 Summary of Key Findings

<!-- src:DB-P1.1.1 -->

The HeartSim project started from a recognised clinical training need: medical students have limited repeatable exposure to pathological heart sounds, while the existing solutions (audio-only training, high-fidelity mannequins, or incidental patient contacts) each lack an important dimension (see [docs/01_medical_problem §5](../01_medical_problem/README.md)). We built a working prototype that transforms a healthy volunteer into a reconfigurable training model: four modules at the classical auscultation points (aortic, pulmonary, tricuspid, mitral) generate — under the control of an Arduino Nano ESP32 — a combination of haptic vibrations and, when the amplifiers are functioning correctly, audio, in real time synchronised with the measured heart rate of the wearer via a PPG sensor. A web interface lets the instructor wirelessly choose which pathology is simulated and at which severity level (Levine 1–5) [2], [9].

### 1.2 Most Significant Aspects of the Prototype

<!-- src:DB-P1.2.1 -->

**Live synchronisation with a real heartbeat.** The coupling between the measured BPM of the volunteer and the timing of the pathology events is what differentiates this system from audio-only training. During the user tests it became apparent that this moreover reinforces the perceptibility of the natural heart rhythm through the stethoscope, making recognisable auscultation possible even through a thin T-shirt — something we had not anticipated (see also [docs/03_discussion §4.1](../03_discussion/README.md)).

<!-- src:DB-P1.2.2 -->

**Gradable severity and direct control.** By linking the severity slider from level 1 to 5 to the SD/GAIN pin of the amplifiers [25] and to the motor drive, the instructor can determine the difficulty level live. This directly addresses the gradation approach that we identified from the literature (Levine scale [2], Littmann CORE [10]) as didactically valuable.

<!-- src:DB-P1.2.3 -->

**Modular and affordable architecture.** With a total cost well under 200 euro per set-up, the concept offers a fundamental cost advantage over commercial simulation mannequins, without sacrificing the living chest — with natural breathing, body warmth, and skin compliance (see the objective stated in [docs/00_introduction §3](../00_introduction/README.md)).

<!-- src:DB-P1.2.4 -->

**Validation by medically trained users.** Two medical students and one retired general practitioner tested the prototype and gave predominantly positive feedback, both about the underlying concept and about the interactive web interface. Although the sample is limited, this provides a first indication that the system has clinical potential (see [docs/03_discussion §4](../03_discussion/README.md) for the full feedback analysis).

<!-- src:DB-P1.2.5 -->

**Lessons from hardware failure.** The failure of the MAX98357A amplifiers [25] during the test phase taught us that the electrical dimensioning of class-D audio in a wearable context requires more protection than we had foreseen in the original design (see [docs/03_discussion §3](../03_discussion/README.md)). This is in itself a relevant learning outcome that we document for anyone building further on this work.

## 2. Future Work

<!-- src:DB-P2.0 -->

The prototype is a working basis but not yet a finished product. We see a number of clear improvement directions, ordered from direct technical refinement to broader clinical and didactic extension.

### 2.1 Hardware Robustness and Protection

<!-- src:DB-P2.1.1 -->

The most urgent improvement follows directly from the amplifier failure discussed in the Discussion chapter (see [docs/03_discussion §3](../03_discussion/README.md)). For a next iteration, a series current limiter or an N-channel MOSFET buffer between the amplifier and the speaker should cap the peak current, together with a choice of speakers with a higher impedance (for example 16 ohm instead of 8 ohm) to halve the load. In addition, a switchable protection on the 5 V rail of the power supply is advisable to absorb transient spikes. Once these protection measures are in place, the combined haptic-and-audio test must still be carried out to validate the full system behaviour.

### 2.2 Form Factor and Wearability

<!-- src:DB-P2.2.1 -->

The current control module (Arduino Nano ESP32 on a breadboard with MB102 power supply and jumper-wire cabling) is too bulky and too fragile to be worn comfortably on a person (see [docs/03_discussion §4.2](../03_discussion/README.md)). A logical next step is to design a custom PCB that integrates the microcontroller, the amplifiers, the multiplexer, and the power regulation on a single compact board, with soldered connectors to the modules instead of jumper wires. The four haptic-acoustic modules themselves can also be redesigned in a more compact housing — preferably with an optimised speaker chamber that continues to support the frequency range from 100 Hz to several kHz — so that they leave sufficient room for the stethoscope even on narrower thorax anatomies.

### 2.3 Reduction of Mechanical Motor Noise

<!-- src:DB-P2.3.1 -->

At high amplitudes the mechanical noise of the coin motors becomes audible through the stethoscope, which degrades the realism (see [docs/03_discussion §4.2](../03_discussion/README.md)). Possible solutions are (i) switching to LRA motors (Linear Resonant Actuators), which are acoustically quieter and moreover have a much sharper frequency response, or (ii) a mechanical decoupling between the motor housing and the skin-contact surface via a damping layer, so that structure-borne noise is transmitted less directly to the stethoscope. Once these modifications are implemented, the haptic component can be used at full power without reservation, making the simulation of clearly audible grade-4/5 murmurs [2] more realistic.

### 2.4 Broader Validation and Anatomical Coverage

<!-- src:DB-P2.4.1 -->

Our user validation was limited to three test subjects, all male. Future work must explicitly evaluate the system on female subjects, because mammary tissue and intercostal distances affect the placement and signal transmission of the modules — an aspect that we could not address within this project. Similarly, the system should be tested on subjects with different body builds (BMI range, thorax width), since this directly impacts the contact between the module and the skin and thus the perceived signal strength.

<!-- src:DB-P2.4.2 -->

In addition, a structured validation with clinical experts is necessary to objectively assess the degree of realism. Our ear is not medically trained, so all our assessments of auditory realism are based on comparison with the PhysioNet [6], Thinklabs [13], and Littmann CORE [10] databases. A collaboration with a cardiologist or a university simulation centre could, through a blinded recognition test, quantify how much better students learn to distinguish pathologies after training on HeartSim, potentially compared against the existing mannequin-based or audio-only methods.

### 2.5 Extension to Pulmonary Auscultation

<!-- src:DB-P2.5.1 -->

A logical next step — which we had already flagged in the original project proposal — is to extend the concept to the respiratory system. Pulmonary pathologies such as crackles, wheezes, and diminished breath sounds lend themselves technically well to the same haptic-acoustic principle, with the main differences being the frequency range and the placement of the modules on the lung fields. We made a start on exploring this possibility within this project but were unable to complete that branch of work. For a follow-up project this is a logical extension that positions the system as a broader auscultation platform rather than solely a cardiac trainer.

### 2.6 Didactic Extension of the Web Interface

<!-- src:DB-P2.6.1 -->

The current web interface is functional but minimalist: it allows an instructor to choose which pathology is simulated. A next development phase could expand it into a didactic platform: a structured course mode that guides students through a progression of pathologies, an exam mode in which the instructor can set an unknown pathology and the student must provide a diagnosis, and a recording function that — inspired by the Littmann CORE app [10] — allows the student to compare her own auscultation afterwards against the actually played pathology. Dynamic manoeuvres (Valsalva, handgrip, positional changes) [2], which we identified in the literature review as clinically relevant but placed outside the scope of this prototype (see [docs/00_introduction §4](../00_introduction/README.md)), could also be made controllable via the same interface, potentially coupled to a breathing sensor.

## 3. Lessons Learned

<!-- src:DB-P3.1 -->

**Dimension electrical protection for the worst case, not the operating point.** Our MAX98357A amplifiers [25] operated well below their maximum for the typical pathologies, but transients and prolonged high-gain operation nevertheless damaged them irreversibly. For anyone building further on this work: always provide current limiting and transient protection on the output stage, even if the average load appears low.

<!-- src:DB-P3.2 -->

**Test each subsystem individually before integration.** Our modular bring-up strategy (first I²C multiplexer, then motor driver, then audio, then PPG, then integration — see [docs/02_methodology §6](../02_methodology/README.md)) saved a great deal of debug time. However, identifying the amplifier failure was delayed because we initially looked at software; a quick hardware-only sanity check (a multimeter on the amplifier output) would have led us to the true cause faster.

<!-- src:DB-P3.3 -->

**Unexpected features can be as valuable as the planned ones.** Heart-rate synchronisation as a reinforcement of the natural rhythm — and by extension the usability through a T-shirt — were not things we had planned in advance. Both emerged from iteratively building and testing with the actual target group. For follow-up projects it is worthwhile to schedule early user tests, even with an incomplete prototype.

<!-- src:DB-P3.4 -->

**Validation without clinical experts is a fundamental limitation.** We were unable to assess ourselves how realistic our simulation is. Anyone developing this concept further should seek contact with a cardiologist or university simulation centre early in the process in order to objectively calibrate the auditory and haptic signals.

<!-- src:DB-P3.5 -->

**A working concept is not a finished product.** The prototype demonstrates that the concept works; the next phase (PCB integration, housing, protection circuits, broader validation) is at least as large in scope as what we have realised so far. Anyone building on this work would do well to plan for this from the outset rather than treating it as a finishing touch.

## 4. Closing Remarks

<!-- src:DB-P4.1 -->

HeartSim demonstrates that an affordable, wearable, and interactive alternative to classical auscultation training is technically feasible within a student project of a few months. The prototype remains a prototype — with the attendant limitations in robustness, form factor, and validation — but the core functionality works, and the first feedback from medically trained users is encouraging. We hope that this work provides a useful basis for anyone who wishes to develop the concept further into a clinically validatable training platform.
