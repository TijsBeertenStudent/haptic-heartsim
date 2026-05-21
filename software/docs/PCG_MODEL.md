# PCG Model — Mathematical Underpinning

This document describes the mathematical model used for the procedural synthesis of heart sounds in `src/heart_sound_synth.cpp`.

## 1. Cardiac cycle timing

The heart has a rhythmic cycle with an **inter-beat interval (IBI)** in milliseconds. At a heart rate of 75 BPM the IBI = 60000/75 = **800 ms**.

Within one cycle there are up to four sounds:

| Sound | Time within cycle | Description |
|-------|-------------------|-------------|
| S1 | `t = 0` (= R-peak detection) | Closure of the mitral and tricuspid valves (start of systole) |
| S2 | `t = 0.35 × IBI` | Closure of the aortic and pulmonary valves (end of systole) |
| S3 | `t = 0.35 × IBI + 0.15 × IBI` | Early diastole (ventricular filling) |
| S4 | `t = IBI − 80 ms` | Late diastole (atrial contraction) |

The factor 0.35 for systolic duration (`SYSTOLE_FRACTION` in `config.h`) comes from the Bazett formula and is empirically very stable for normal heart rates (50–100 BPM).

## 2. Modelling S1, S2, S3, S4 as damped sinusoids

Each heart **sound** originates from valve vibrations that gradually die out. We model this as a **damped sinusoid**:

$$s(t) = A \cdot \sin(2\pi f t) \cdot e^{-\alpha t}$$

where:

- $A$: amplitude (0..1)
- $f$: fundamental frequency (Hz)
- $\alpha$: damping constant (1/s)
- $t$: time since the start of the sound

Values from the literature ([Phonocardiogram review, Fiveable; ResearchGate fig. 2806530](https://www.researchgate.net/figure/llustrates-the-PCG-signal-including-heart-sounds-S1-S2-S3-S4-1-2-Measured-average_fig2_262806530)):

| Sound | $f$ (Hz) | $\alpha$ (1/s) | Duration (ms) | Amplitude factor |
|-------|----------|----------------|---------------|------------------|
| S1    | 50       | 25             | 70–150        | 0.85             |
| S2    | 70       | 30             | 60–120        | 0.65             |
| S3    | 35       | 20             | 40–100        | 0.50             |
| S4    | 25       | 18             | 40–80         | 0.45             |

S2 is normally **softer** than S1 and has a **higher frequency** due to the stiffer semilunar valves. S3/S4 are very low-frequency and therefore primarily **felt** rather than heard — precisely why the vibration motors are especially valuable at those points.

## 3. Murmurs as filtered white noise

Murmurs arise from **turbulent flow** through a narrowed or leaking valve. Mathematically this closely resembles band-pass-filtered white noise:

$$m(t) = n_{\text{white}}(t) * h_{\text{BPF}}(f_c, Q) \cdot \text{env}(t)$$

where:

- $n_{\text{white}}$: white noise (we use a 32-bit Galois LFSR for speed)
- $h_{\text{BPF}}$: band-pass IIR biquad with centre frequency $f_c$ and quality factor $Q$
- $\text{env}(t)$: time-varying envelope — determines the **shape** of the murmur

### 3.1 Biquad band-pass

We use the Robert Bristow-Johnson "Audio EQ Cookbook" coefficients for a 2nd-order band-pass:

```
ω₀ = 2π · fc / fs
α  = sin(ω₀) / (2Q)

b₀ =  α        b₁ = 0    b₂ = -α
a₀ =  1 + α    a₁ = -2cos(ω₀)    a₂ = 1 - α
```

We normalise by $a_0$ and run Direct Form II Transposed:

```
w[n] = x[n] − a₁·w[n-1] − a₂·w[n-2]
y[n] = b₀·w[n] + b₁·w[n-1] + b₂·w[n-2]
```

### 3.2 Envelope shapes per pathology

The **envelope** is what gives a murmur its clinical signature. Four basic shapes:

#### Diamond (crescendo-decrescendo)
$$\text{env}_{\text{dia}}(\phi) = \begin{cases} 2\phi & \phi < 0.5 \\ 2(1-\phi) & \phi \geq 0.5 \end{cases}$$
Clinical: **Aortic Stenosis** — the sound gradually becomes louder, peaks at mid-systole, and fades again.

#### Rectangular (holosystolic)
$$\text{env}_{\text{rect}}(\phi) = c \quad (\text{constant})$$
Clinical: **Mitral Regurgitation** — constant "blowing" throughout the entire systole.

#### Decrescendo
$$\text{env}_{\text{dec}}(\phi) = e^{-3\phi}$$
Clinical: **Aortic Regurgitation** — loud immediately after S2, fades exponentially.

#### Decrescendo-crescendo (rumble)
$$\text{env}_{\text{rmb}}(\phi) = 0.3 + 0.4 \cdot |2\phi - 1|$$
Clinical: **Mitral Stenosis** — rumbling, louder towards the end (presystolic accentuation).

### 3.3 Parameters per pathology

| Pathology | Timing | $f_c$ (Hz) | $Q$ | Envelope |
|---|---|---|---|---|
| Aortic Stenosis | between S1 & S2 | 300 | 2.5 | Diamond |
| Mitral Regurgitation | between S1 & S2 | 350 | 2.0 | Rectangular |
| Mitral Stenosis | after S2 (+80 ms snap + rumble) | 120 | 4.0 | Decrescendo-crescendo |
| Aortic Regurgitation | directly after S2 (40% of diastole) | 350 | 2.5 | Decrescendo |

## 4. Severity scaling

The severity slider (1..5) multiplies the envelope amplitude:

$$\text{gain}(s) = 0.4 + 0.15 \cdot s$$

So severity=1 → gain=0.55, severity=5 → gain=1.15. This affects **only** the murmur, not S1/S2 themselves — which is clinically correct.

## 5. Spatial mapping (auscultation points)

Different pathologies are loudest at **different locations** on the chest. In `audio_engine.cpp` a 4-element weight table `PATHOLOGY_WEIGHTS` specifies for each pathology how much each auscultation point receives:

```
                          Aortic  Pulm.  Tricusp.  Mitral
NORMAL                    200     200    200       200
AORTIC_STENOSIS           255     200    130       100   ← loudest upper right
MITRAL_REGURGITATION      100     120    180       255   ← loudest lower left
MITRAL_STENOSIS            80      80    140       255   ← apex only
AORTIC_REGURGITATION      230     220    255       180   ← Erb's point
S3_GALLOP                 120     140    200       255   ← left ventricle
S4_GALLOP                 130     150    210       255   ← atrium → apex
```

These values should be verified with a medical supervisor — they are realistic best estimates based on standard auscultation textbooks.

## 6. Soft-clipping to prevent distortion

If severity=5 and master volume=255 and multiple components add up, the signal can exceed 1.0. We prevent hard clipping with a `tanh` soft-clipper:

```
if sample > 0.95:  sample = 0.95 + 0.05 · tanh(sample − 0.95)
```

This gives a natural "chest compression" feel without the sharp artefacts of digital clipping.

## 7. Validation

Recommended tests to verify the model:

1. **Spectral**: Sample a murmur for 10 seconds to a WAV, perform an FFT in Python/MATLAB, compare with PhysioNet Challenge 2016 reference spectra for the same pathology.
2. **Auditory**: Have a cardiologist or senior medical student blindly score 10 simulations — which pathology do they hear?
3. **Timing**: Measure the latency between the PPG R-peak and S1 onset with an oscilloscope. Target: <50 ms.

## References

- Phonocardiogram (PCG) signal processing, Advanced Signal Processing (Fiveable, 2024).
- Heartbeat Cardiac Sounds Analysis, ClinicSearch.
- Jabloun, M. et al. (2013). *A generating model of realistic synthetic heart sounds for performance assessment of phonocardiogram processing algorithms.* Biomedical Signal Processing and Control.
- Phonocardiogram robust synthetic signal generation (2020). ScienceDirect.
- Bristow-Johnson, R. *Cookbook formulae for audio EQ biquad filter coefficients.*
- PhysioNet/CinC Challenge 2016. https://physionet.org/content/challenge-2016/1.0.0/
- 3M Littmann Heart & Lung Sounds Library.
