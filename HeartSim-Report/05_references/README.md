# References

This file is the master reference document for the HeartSim repository. It serves two purposes:

1. **Source Usage Overview** — the table below shows every source used across the four core documents, what each source contributes, and in which section of the repository it is cited.
2. **IEEE Reference List** — the numbered entries that follow the table are the authoritative citations used throughout the repository. Every other README uses these same numbers; when a new source is needed, it is appended here and the next available number is used — existing entries are never renumbered.

The four document sections referenced in the table are:

| Label | Document |
|-------|----------|
| `01_medical_problem` | Literature review — cardiac auscultation and haptic simulation |
| `02_methodology` | Design methodology, component selection, and firmware architecture |
| `03_discussion` | Test results, observed limitations, and user feedback |
| `04_future_work` | Conclusions and proposed next iterations |



---

## Source Table

| No. | Short Name | Abbreviated Citation | Purpose in This Project | Used In |
|-----|-----------|---------------------|------------------------|---------|
| [1] | Pollock & Makaryus | J. D. Pollock and A. N. Makaryus, "Physiology, Cardiac Cycle," *StatPearls*, 2022 | Reference for cardiac anatomy, the four phases of ventricular activity, ventricular pressure values, and the mechanical origin of S1/S2 | 01_medical_problem |
| [2] | Thomas et al. | S. L. Thomas et al., "Physiology, Cardiovascular Murmurs," *StatPearls*, 2023 | Clinical classification of murmurs by timing, shape, and location; Levine grading; dynamic auscultation manoeuvres (Valsalva, handgrip) | 01_medical_problem, 03_discussion, 04_future_work |
| [3] | Rodriguez-Guerrero et al. | C. Rodriguez-Guerrero et al., "B-KUL-T4lMD2 — Course Project," KU Leuven, 2026 | Project scope, minimum requirements, deliverables, and evaluation criteria set by the course supervisors | 01_medical_problem |
| [4] | ESC Mitral Valve | European Society of Cardiology, "Mitral valve disease," *Council for Cardiology Practice E-Journal*, vol. 16 | Clinical features of mitral regurgitation and mitral valve prolapse, radiation patterns, and manoeuvre-dependent intensity changes | 01_medical_problem |
| [5] | Springer et al. | D. B. Springer et al., "Logistic Regression-HSMM," *IEEE Trans. Biomed. Eng.*, vol. 63, no. 4, 2016 | Frequency content of S1/S2 peaking near 50 Hz; motivation for the 40 mm speaker choice; timing of S1 and S2 relative to the cardiac cycle | 01_medical_problem |
| [6] | PhysioNet / CinC 2016 | C. Liu et al., "Open access heart sound database," *Physiol. Meas.*, vol. 37, no. 12, 2016 | Primary labelled WAV dataset for the project; benchmark used in PCG segmentation studies; FFT of real recordings used to set bandpass filter parameters | 01_medical_problem, 02_methodology, 03_discussion, 04_future_work |
| [7] | NLM — Heart Failure | National Library of Medicine, "Heart Failure," *NCBI Bookshelf*, n.d. | Pathophysiology of S3 and S4 gallops; S3 as a marker of congestive heart failure or elevated ventricular filling pressure | 01_medical_problem |
| [8] | Pelech | A. N. Pelech, "The physiology of cardiac auscultation," *Pediatr. Clin. North Am.*, vol. 51, no. 6, 2004 | Physiological basis of cardiac auscultation; criteria for distinguishing innocent from pathological murmurs | 01_medical_problem |
| [9] | Oxford Handbook | J. Thomas and T. Monaghan, *Oxford Handbook of Clinical Examination and Practical Skills*, 2nd ed., OUP, 2014 | Standard four-point auscultation scheme (Figure 3); murmur timing and shape overview (Figure 2); Levine grading scale (Figure 5); aortic stenosis and aortic regurgitation features | 01_medical_problem, 03_discussion, 04_future_work |
| [10] | Littmann CORE | 3M Littmann, "CORE Digital Stethoscope," 3M Health Care, n.d. | Inspiration for live BPM feedback and graduated difficulty levels; heart sound library used for auditively validating the synthesis of aortic stenosis and mitral regurgitation | 01_medical_problem, 02_methodology, 03_discussion, 04_future_work |
| [11] | Univ. of Michigan | University of Michigan, "Heart Sound & Murmur Library," Open.Michigan, n.d. | Widely cited educational library; **audio players were non-functional at the time of the project** (404 errors); textual descriptions used for S3/S4 gallop duration and intensity calibration | 01_medical_problem, 02_methodology |
| [12] | Univ. of Washington | University of Washington SoM, "Heart Sounds Demonstrations," n.d. | Functional short audio samples, clearly labelled per pathology; used as a working alternative to [11] | 01_medical_problem |
| [13] | Thinklabs | Thinklabs Medical LLC, "Heart Sounds Library," n.d. | Supplementary audio source; YouTube channel with 100+ pathology videos showing simultaneous audio playback and spectrogram for visual validation of synthetic output | 01_medical_problem, 03_discussion, 04_future_work |
| [14] | Schmidt et al. 2010 | S. E. Schmidt et al., "Segmentation by DHMM," *Physiol. Meas.*, vol. 31, no. 4, 2010 | Duration-dependent hidden Markov model; systole duration as ~0.3–0.4 × IBI — theoretical basis of the cycle engine; frequency ranges per pathology | 01_medical_problem |
| [15] | GitHub — Srikrishnan | Srikrishnan, "Biomedical Signal Analysis," GitHub, 2022 | Open-source HSMM PCG segmentation implementation; used as reference when designing the cycle-engine firmware | 01_medical_problem |
| [16] | Osmosis — YouTube | Osmosis, "Cardiac cycle," YouTube, n.d. | Visual explanation of the Wiggers diagram and the cardiac cycle; used for initial knowledge building at the start of the project | 01_medical_problem |
| [17] | Cardiac cycle — YouTube | "Cardiac cycle explanation," YouTube, n.d. | Supplementary visual explanation of the cardiac cycle and valve physiology | 01_medical_problem |
| [18] | Stanford 25 — YouTube | Stanford 25, "Heart sounds and murmurs explained," YouTube, n.d. | Clinical audio–visual demonstration of differences between normal and pathological heart sounds | 01_medical_problem |
| [19] | JST-XH crimp guide | "Step-by-step guide: How to crimp JST XH connectors," web tutorial, n.d. | Assembly procedure for JST-XH multi-pin connectors used in module wiring to reduce cable count and improve mechanical retention | 02_methodology |
| [20] | DuPont crimp guide | "Dupont Crimp Tool Tutorial," *Instructables*, n.d. | Crimping procedure for DuPont-style connector housings used throughout the prototype wiring harness | 02_methodology |
| [21] | Jabloun et al. 2013 | M. Jabloun et al., "Generating model of synthetic heart sounds," *IEEE EMBC*, 2013 | Mathematical damped-sinusoid model for S1/S2; parameter values for damping coefficient α and dominant frequency *f* implemented in the audio synthesis engine | 02_methodology |
| [22] | Schmidt et al. 2016 | S. E. Schmidt et al., "Heart sound classification using deep neural networks," *CinC* 2016 | Clinical dataset reference for murmur frequency spectra; used to validate that the synthetic output falls within the 50–500 Hz band observed in real recordings | 02_methodology |
| [23] | Audio EQ Cookbook | R. Bristow-Johnson, "Audio EQ Cookbook," rev. 2021 | Biquad filter coefficient formulae (Direct Form II Transposed); used to derive centre frequency and Q-factor for the murmur bandpass filters | 02_methodology |
| [24] | DRV2605L Datasheet | Texas Instruments, "DRV2605L Datasheet," SLOS854B, 2014 | Real-Time Playback register settings; H-bridge peak current limit (250 mA); I²C address (0x5A) for motor driver configuration | 02_methodology |
| [25] | MAX98357A Datasheet | Maxim Integrated / Analog Devices, "MAX98357A Datasheet," 19-7380, n.d. | SD/GAIN pin voltage table from which the PWM-to-volume mapping is derived; peak output current specification used to assess amplifier loading and diagnose the hardware failure discussed in 03_discussion | 02_methodology |
| [26] | ESP32-S3 TRM | Espressif Systems, "ESP32-S3 Technical Reference Manual," ver. 1.4, 2024 | ADC characteristics; I²S DMA operation; FreeRTOS dual-core task affinity (core 0 for PPG and cycle planner, core 1 for audio streaming) | 02_methodology |

---

## IEEE Reference List

[1] J. D. Pollock and A. N. Makaryus, "Physiology, Cardiac Cycle," in *StatPearls*. Treasure Island, FL, USA: StatPearls Publishing, 2022. [Online]. Available: https://www.ncbi.nlm.nih.gov/books/NBK459327/

*Cited in 01_medical_problem (cardiac anatomy, chamber and valve function, four phases of ventricular activity, mechanical origin of S1 and S2, ventricular pressure values); figure caption for Figure 1 (Wiggers diagram).*

---

[2] S. L. Thomas, J. Heaton, and A. N. Makaryus, "Physiology, Cardiovascular Murmurs," in *StatPearls*. Treasure Island, FL, USA: StatPearls Publishing, 2023. [Online]. Available: https://www.ncbi.nlm.nih.gov/books/NBK525958/

*Cited in 01_medical_problem (murmur classification by timing, location, intensity, shape; gallop rhythms; Levine grading; dynamic auscultation manoeuvres); 03_discussion (Levine gradation as didactic framework); 04_future_work (Levine scale and gradation rationale).*

---

[3] C. Rodriguez-Guerrero, M. Rodriguez, and E. Ury, "B-KUL-T4lMD2 Haptic Interfaces Experience — Course Project," course materials, Dept. of Mechanical Engineering, KU Leuven, Leuven, Belgium, 2026.

*Cited in 01_medical_problem (project scope, minimum requirements, deliverables, and evaluation criteria).*

---

[4] European Society of Cardiology, "Mitral valve disease: Clinical features focusing on auscultatory findings, including auscultation of mitral valve prolapse," *Council for Cardiology Practice E-Journal*, vol. 16, n.d. [Online]. Available: https://www.escardio.org/communities/councils/cardiology-practice/scientific-documents-and-publications/ejournal/volume-16/Mitral-valve-disease-clinical-features-focusing-on-auscultatory-findings-including-auscultation-of-mitral-valve-prolapse/ [URL not verified]

*Cited in 01_medical_problem (mitral regurgitation clinical features, radiation patterns, dynamic manoeuvres; regurgitation as a consequence of valve damage).*

---

[5] D. B. Springer, L. Tarassenko, and G. D. Clifford, "Logistic Regression-HSMM-Based Heart Sound Segmentation," *IEEE Trans. Biomed. Eng.*, vol. 63, no. 4, pp. 822–832, Apr. 2016, doi: 10.1109/TBME.2015.2475278. [Online]. Available: https://ieeexplore.ieee.org/document/7234876

*Cited in 01_medical_problem (frequency content of S1/S2 peaking near 50 Hz; motivation for a 40 mm speaker; synchronisation of heart sounds to the cardiac cycle; frequency ranges per pathology in Table 1).*

---

[6] C. Liu et al., "An open access database for the evaluation of heart sound algorithms," *Physiol. Meas.*, vol. 37, no. 12, pp. 2181–2213, Dec. 2016, doi: 10.1088/0967-3334/37/12/2181. [Online]. Available: https://physionet.org/content/challenge-2016/1.0.0/

*Cited in 01_medical_problem (primary labelled WAV dataset for the project; benchmark used by PCG segmentation studies); 02_methodology (murmur spectra validation; FFT of real recordings used to set bandpass filter parameters); 03_discussion (audio reference database for qualitative validation); 04_future_work (proposed database for blind recognition tests).*

---

[7] National Library of Medicine, "Heart Failure," *NCBI Bookshelf*, Bethesda, MD, USA, n.d. [Online]. Available: https://www.ncbi.nlm.nih.gov/books/NBK345/ [URL not verified]

*Cited in 01_medical_problem (pathophysiology of S3 and S4 gallops; S3 as a sign of congestive heart failure or high ventricular filling).*

---

[8] A. N. Pelech, "The physiology of cardiac auscultation," *Pediatr. Clin. North Am.*, vol. 51, no. 6, pp. 1515–1535, Dec. 2004, doi: 10.1016/j.pcl.2004.08.004. [Online]. Available: https://www.sciencedirect.com/science/article/pii/S003139550400121X

*Cited in 01_medical_problem (physiology of cardiac auscultation; innocent versus pathological murmurs in paediatric patients).*

---

[9] J. Thomas and T. Monaghan, *Oxford Handbook of Clinical Examination and Practical Skills*, 2nd ed. Oxford, UK: Oxford University Press, 2014. ISBN 978-0-19-967490-6.

*Cited in 01_medical_problem (four standard auscultation areas, Figure 3; murmur timing and shape overview, Figure 2; Levine grading scale, Figure 5; aortic stenosis and aortic regurgitation clinical features); 03_discussion (Levine scale as framework for severity slider); 04_future_work (Levine scale rationale for gradation design).*

---

[10] 3M Littmann, "CORE Digital Stethoscope," 3M Health Care, St. Paul, MN, USA, n.d. [Online]. Available: https://www.littmann.com/en-us/home/core-digital-stethoscope/

*Cited in 01_medical_problem (inspiration for immediate BPM feedback and graduated difficulty; in-app heart sound library); 02_methodology (auditively validated reference for correctly synthesised aortic stenosis and mitral regurgitation sounds); 03_discussion (audio reference database used for qualitative validation); 04_future_work (gradation approach and proposed recording feature inspired by the CORE app).*

---

[11] University of Michigan, "Heart Sound & Murmur Library," Open.Michigan Educational Resources, Ann Arbor, MI, USA, n.d. [Online]. Available: https://open.umich.edu/find/open-educational-resources/medical/heart-sound-murmur-library [URL not verified]

*Cited in 01_medical_problem (frequently cited educational library; noted as non-functional at time of project — audio players return 404 errors); 02_methodology (reference for S3/S4 gallop duration and intensity calibration, textual descriptions still accessible).*

---

[12] University of Washington School of Medicine, "Heart Sounds Demonstrations," Department of Physical Diagnosis, Seattle, WA, USA, n.d. [Online]. Available: https://depts.washington.edu/physdx/heart/demo.html

*Cited in 01_medical_problem (functional audio collection of labelled heart sound demonstrations; used as a working alternative to the non-functional Michigan library).*

---

[13] Thinklabs Medical LLC, "Heart Sounds Library," Thinklabs One Digital Stethoscope, Centennial, CO, USA, n.d. [Online]. Available: https://www.thinklabs.com/heart-sounds

*Cited in 01_medical_problem (supplementary audio source; YouTube channel with 100+ pathology videos showing live audio and spectrogram simultaneously); 03_discussion (audio reference database for qualitative validation); 04_future_work (proposed database for blind recognition tests).*

---

[14] S. E. Schmidt, C. Holst-Hansen, C. Graff, E. Toft, and J. J. Struijk, "Segmentation of heart sound recordings by a duration-dependent hidden Markov model," *Physiol. Meas.*, vol. 31, no. 4, pp. 513–529, Apr. 2010, doi: 10.1088/0967-3334/31/4/004. [Online]. Available: https://iopscience.iop.org/article/10.1088/0967-3334/31/4/004

*Cited in 01_medical_problem (DHMM segmentation model; 98.8 % sensitivity on 73 patients; systole duration as ~0.3–0.4 × IBI — theoretical basis for the cycle engine; frequency ranges per pathology in Table 1).*

---

[15] Srikrishnan, "Biomedical Signal Analysis," GitHub code repository, 2022. [Online]. Available: https://github.com/srikrishnan1972/Biomedical-Signal-Analysis [URL not verified]

*Cited in 01_medical_problem (example implementation of HSMM-based PCG segmentation and feature extraction; used as reference for the cycle-engine firmware).*

---

[16] Osmosis, "Cardiac cycle," YouTube video, n.d. [Online]. Available: https://www.youtube.com/watch?v=dX9SAgzLwXk [URL not verified]

*Cited in 01_medical_problem (visual explanation of the Wiggers diagram and cardiac cycle; used for initial knowledge building at the start of the project).*

---

[17] "Cardiac cycle explanation," YouTube video, n.d. [Online]. Available: https://www.youtube.com/watch?v=1b4V09HzhBw [URL not verified]

*Cited in 01_medical_problem (supplementary explanation of the cardiac cycle and valve physiology).*

---

[18] Stanford 25, "Heart sounds and murmurs explained," YouTube video, n.d. [Online]. Available: https://www.youtube.com/watch?v=pcgGnDU4a3M [URL not verified]

*Cited in 01_medical_problem (clinical demonstration of auditory differences between normal and pathological heart sounds).*

---

[19] "Step-by-step guide: How to crimp JST XH connectors," web tutorial, n.d. [Online]. Available: https://ldzy.tw/crimp-jst-xh-connector-step-to-step-guide/ [URL not verified]

*Cited in 02_methodology (connector assembly procedure for JST-XH multi-pin connectors used in module wiring).*

---

[20] "Dupont Crimp Tool Tutorial," *Instructables*, n.d. [Online]. Available: https://www.instructables.com/Dupont-Crimp-Tool-Tutorial/

*Cited in 02_methodology (procedure for crimping DuPont-style connector housings used in the prototype wiring).*

---

[21] M. Jabloun, P. Ravier, and O. Buttelli, "A generating model of realistic synthetic heart sounds for performance assessment of phonocardiogram processing algorithms," in *Proc. 35th Annu. Int. Conf. IEEE Eng. Med. Biol. Soc. (EMBC)*, Osaka, Japan, Jul. 2013, pp. 5308–5311, doi: 10.1109/EMBC.2013.6610754. [URL not verified]

*Cited in 02_methodology (mathematical model of S1/S2 as damped sinusoids; parameter values for damping coefficient α and dominant frequency f used in the audio synthesis engine).*

---

[22] S. E. Schmidt, C. Holst-Hansen, C. Hansen et al., "Heart sound classification using deep neural networks," in *Proc. Computing in Cardiology (CinC) Challenge 2016*, Vancouver, BC, Canada, 2016. [citation details unverified]

*Cited in 02_methodology (clinical dataset reference for murmur frequency spectra; validates that synthetic output falls within the 50–500 Hz band observed in real recordings).*

---

[23] R. Bristow-Johnson, "Cookbook formulae for audio equaliser biquad filter coefficients," web document, rev. 2021. [Online]. Available: https://webaudio.github.io/Audio-EQ-Cookbook/audio-eq-cookbook.html [URL not verified]

*Cited in 02_methodology (biquad filter coefficient formulae used for the bandpass filters applied to murmur synthesis; centre frequency and Q-factor derivation).*

---

[24] Texas Instruments, "DRV2605L 2-V to 5.2-V Haptic Driver for LRA and ERM with Effect Library and Smart-Loop Architecture," datasheet SLOS854B, Dallas, TX, USA: Texas Instruments, 2014. [Online]. Available: https://www.ti.com/product/DRV2605L

*Cited in 02_methodology (register settings for Real-Time Playback mode; H-bridge peak current limit of 250 mA; I²C address and electrical specifications for motor driver configuration).*

---

[25] Maxim Integrated (Analog Devices), "MAX98357A/MAX98357B — Filterless Class D I²S Mono Audio Power Amplifier," datasheet 19-7380, Sunnyvale, CA, USA: Maxim Integrated, n.d. [Online]. Available: https://www.analog.com/en/products/max98357a.html

*Cited in 02_methodology (SD/GAIN-pin voltage table from which the PWM-to-volume mapping is derived; peak output current specification used to assess amplifier loading and diagnose the hardware failure described in 03_discussion).*

---

[26] Espressif Systems, "ESP32-S3 Technical Reference Manual," ver. 1.4, Shanghai, China: Espressif Systems, 2024. [Online]. Available: https://www.espressif.com/sites/default/files/documentation/esp32-s3_technical_reference_manual_en.pdf

*Cited in 02_methodology (ADC characteristics; I²S DMA operation; FreeRTOS dual-core task affinity used to assign PPG sampling to core 0 and audio streaming to core 1).*

---


