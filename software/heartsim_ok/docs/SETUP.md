# SETUP — Getting HeartSim running with PlatformIO

This guide takes you from **zero** to a **working WiFi HMI on your phone** in approximately 30 minutes. No prior knowledge of PlatformIO required.

---

## 1. Installing required software

### 1.1 Visual Studio Code (free)

1. Download VS Code: https://code.visualstudio.com/
2. Install with default settings.

### 1.2 PlatformIO extension

1. Open VS Code.
2. Click the **Extensions** icon on the left (squares icon, or `Ctrl+Shift+X`).
3. Search for **PlatformIO IDE**.
4. Click **Install**. This takes 3–5 minutes (PlatformIO loads its own Python environment).
5. After installation: **restart VS Code**.

### 1.3 USB driver (Windows only)

The Arduino Nano ESP32 appears as a COM port via USB-CDC. On modern Windows 10/11 this works out of the box. If the port does not appear:
- Plug in the Nano while double-clicking the RESET button (DFU mode).
- Optionally install: https://github.com/espressif/usb-jtag-serial-driver/releases

---

## 2. Opening the project

### 2.1 Obtaining the codebase

Two options:

**Option A — ZIP** (no Git required):
1. Download the project ZIP and extract to e.g. `C:\Users\<you>\Documents\heartsim\`.

**Option B — Git**:
```bash
git clone <repo-url> heartsim
cd heartsim
```

### 2.2 Opening in VS Code

1. Open VS Code.
2. **File → Open Folder…** → select the `heartsim` folder (the folder containing `platformio.ini`).
3. At the bottom of VS Code a PlatformIO toolbar appears with icons (✓ build, → upload, 🔌 monitor, etc.).

PlatformIO automatically downloads the ESP32 platform and libraries the first time you open the project. This takes 2–5 minutes — wait until there is no more activity in the bottom right.

---

## 3. Compiling and uploading the code

### 3.1 Download Chart.js (one-time)

The web interface uses Chart.js for the live PPG graph. Download it once into the `data/` folder so it works offline:

**PowerShell:**
```powershell
Invoke-WebRequest -Uri "https://cdn.jsdelivr.net/npm/chart.js@4.4.6/dist/chart.umd.min.js" -OutFile "data\chart.min.js"
```

**Bash / macOS / Linux:**
```bash
curl -L -o data/chart.min.js https://cdn.jsdelivr.net/npm/chart.js@4.4.6/dist/chart.umd.min.js
```

### 3.2 First-time flash via USB

1. Plug the **Arduino Nano ESP32** into your laptop with USB-C.
2. Open a PlatformIO terminal in VS Code: **Terminal → New Terminal** (ensure the tab says **PlatformIO CLI**, not PowerShell).
3. Build + upload the firmware:
   ```bash
   pio run -t upload
   ```
   *Or click the right-arrow icon in the PlatformIO toolbar at the bottom.*
4. Upload the filesystem (web UI: HTML/CSS/JS):
   ```bash
   pio run -t uploadfs
   ```

> **Tip**: if the upload fails with "no DFU device", press the **RESET button twice in quick succession** on the Nano to force DFU mode and try again.

### 3.3 Serial monitor (useful for debugging)

```bash
pio device monitor
```

On successful start-up you should see:
```
[HAPTIC] OK: TCA9548A found
[HAPTIC] OK: modules 0..3 initialised
[AUDIO]  OK
[WEB]    AP started: SSID=HeartSim, IP=192.168.4.1
[OTA]    Ready. Upload via: heartsim.local
[WEB]    HTTP server started on port 80
```

Press `Ctrl+C` to close the monitor.

---

## 4. Activating the WiFi HMI

### 4.1 Connecting to the device

1. Power the Nano (USB-C or MB102 5 V).
2. Wait approximately 5 seconds.
3. On your **phone or laptop**: open WiFi settings.
4. Find the network **`HeartSim`**.
5. Connect with password **`heartsim2026`**.

> On iOS / Android a **captive portal** pop-up appears automatically that opens the web interface. Tap it.
>
> If that does not happen: open a browser and go to **`http://192.168.4.1`** (type "http://" explicitly — otherwise some browsers attempt a Google search).

### 4.2 What you should see

- **BPM display** top-left: live heart rate (or `--` if no PPG connected).
- **PPG waveform** top-right: live signal.
- **Pathology** buttons: Normal, Aortic Stenosis, Mitral Regurg…
- **Severity slider**: 1..5.
- **Auscultation points**: 4 modules with ON/OFF.
- **Audio / Haptic** sliders.
- **⏺ Record** button.
- **▶ Start simulation** button at the bottom.

### 4.3 First test

1. Press **⚙ Settings** top-right → enable **Playback mode** (then you do not need a PPG sensor — an internal 70 BPM clock is used).
2. Select pathology **Aortic Stenosis**, severity 3.
3. Press **▶ Start simulation**.
4. You should hear from both speakers (upper and lower) a systolic noise signal; the haptic modules vibrate on the S1/S2 rhythm.
5. Toggle a module off by tapping it — that position goes silent.
6. Press **Stop** to stop.

---

## 5. Subsequent updates — OTA (without USB)

If the hardware is in the chest harness and you want to upload new code without connecting the USB cable:

1. Connect your laptop to the WiFi network **`HeartSim`**.
2. Open the PlatformIO terminal in VS Code.
3. Firmware update:
   ```bash
   pio run -t upload --upload-port heartsim.local --upload-flags "--auth=heartsim2026"
   ```
4. Filesystem (HMI) update:
   ```bash
   pio run -t uploadfs --upload-port heartsim.local --upload-flags "--auth=heartsim2026"
   ```

The simulation stops automatically just before the flash write. After the reboot the device restarts and you need to reconnect your phone/laptop to the `HeartSim` WiFi.

> **Making OTA permanent** — add this to `platformio.ini`:
> ```ini
> upload_protocol = espota
> upload_port     = heartsim.local
> upload_flags    = --auth=heartsim2026
> ```
> Then `pio run -t upload` suffices without extra parameters.

---

## 6. Troubleshooting

| Symptom | Solution |
|---|---|
| `pio` command not found in PowerShell | Use the **PlatformIO CLI** terminal in VS Code, or add `C:\Users\<you>\.platformio\penv\Scripts` to your PATH |
| Build fails on `ledcAttach` not declared | You have an older Arduino-ESP32 core. The code in `audio_engine.cpp` uses `ledcSetup` + `ledcAttachPin` (compatible with core 2.x) |
| Build fails on `timerAlarm` not declared | Same — `ppg_monitor.cpp` uses the old API `timerBegin(0,80,true)` + `timerAlarmWrite` + `timerAlarmEnable` |
| Upload fails: "no DFU device" | Press RESET button twice quickly on the Nano to force DFU mode |
| `HeartSim` WiFi not visible | Hard power cycle. Supply < 500 mA is insufficient. Check serial monitor for `[WEB] AP started` |
| `192.168.4.1` does not load | Turn off **mobile data** on your phone (otherwise it routes via 4G). Type `http://` explicitly |
| Captive portal does not appear | Open `http://192.168.4.1` manually. Some iOS versions block the pop-up |
| BPM stays at 0 | Place PPG sensor on fingertip. Wait 10 s for envelope detector to stabilise. Or enable **Playback mode** in ⚙ Settings |
| Audio stutters | Check serial: `[AUDIO] Task started on core 1`. Power via MB102 5 V rail instead of USB |
| TCA9548A not found | Check I²C wiring (SDA=A4, SCL=A5), 4.7 kΩ pull-ups to 3.3 V |
| One DRV2605L not found | Serial monitor shows which channel is missing — resolder or swap the module |
| No vibrations, only sound | Check that motors are connected directly to DRV2605L OUT+/OUT− (no MOSFET needed) |
| OTA: "authentication failed" | Password is exactly `heartsim2026` (no extra spaces) |
| OTA: "no response" | Connect your laptop to **HeartSim** WiFi (not your regular WiFi) |

### Diagnostic overlay in the HMI

Tap **3× quickly** on the BPM number in the web interface → a diagnostics panel appears showing:
- Free heap memory
- Number of WebSocket clients
- Current pathology, BPM, average IBI
- FreeRTOS stack high-water marks per task
- Uptime

This is useful for remote debugging without a USB cable.

---

## 7. Project structure (reference)

```
heartsim_ok/
├── platformio.ini              ← Build config
├── src/                        ← C++ firmware
│   ├── main.cpp
│   ├── config.h                ← Pin defines, constants
│   ├── audio_engine.{cpp,h}    ← I²S + 2× MAX98357 PWM gain
│   ├── haptic_manager.{cpp,h}  ← 4× DRV2605L via TCA9548A
│   ├── heart_sound_synth.{cpp,h}  ← Procedural PCG synthesis
│   ├── cycle_engine.{cpp,h}    ← Schedules S1/S2/murmurs on cardiac cycle
│   ├── ppg_monitor.{cpp,h}     ← ADC + bandpass + peak detection
│   ├── recorder.{cpp,h}        ← WAV recorder to LittleFS
│   ├── web_interface.{cpp,h}   ← AsyncWebServer + WS + REST API
│   └── tca9548a.h              ← I²C mux wrapper
├── data/                       ← LittleFS content (web HMI)
│   ├── index.html
│   ├── style.css
│   ├── app.js
│   └── chart.min.js
└── docs/
    ├── SETUP.md                ← This file
    ├── WIRING.md               ← Hardware connections
    ├── BOM.md                  ← Bill of materials
    ├── PCG_MODEL.md            ← Mathematical model of heart sounds
    └── DEMO_DAY.md             ← Checklist for the presentation
```

---

## 8. Quick-reference card

```
WiFi:           HeartSim / heartsim2026
Web UI:         http://192.168.4.1
USB upload:     pio run -t upload && pio run -t uploadfs
OTA upload:     pio run -t upload --upload-port heartsim.local
OTA password:   heartsim2026
Serial monitor: pio device monitor
Diag panel:     3× tap on BPM number in the HMI
Backup demo:    ⚙ Settings → Playback mode ON  (no PPG needed)
```

For further details on the architecture: see `README.md` and `docs/PCG_MODEL.md`.
