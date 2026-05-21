/* ============================================================================
   HeartSim — App logic
   ----------------------------------------------------------------------------
   Verantwoordelijkheden:
     - WebSocket connectie naar /ws met auto-reconnect
     - PPG waveform scrolling chart via Chart.js
     - Pathology / severity / module / volume controls met optimistic UI
     - GET /api/status bij first load om de UI te hydrateren
   ============================================================================ */

// ============================================================
// Configuratie
// ============================================================
const PPG_BUFFER_SIZE = 250;            // 5 seconden bij 20 Hz push (12 samples per push)
const WS_RECONNECT_MS = 2000;

// ============================================================
// State
// ============================================================
const PULMONARY_PATHS = new Set([
    "CRACKLES_FINE", "CRACKLES_COARSE",
    "WHEEZE_INSPIRATORY", "WHEEZE_EXPIRATORY"
]);

// ============================================================
// i18n
// ============================================================
const TRANSLATIONS = {
    nl: {
        "h1-title":          "Hart Auscultatie Simulator",
        "h2-pathology":      "Pathologie",
        "h2-severity":       "Ernst",
        "h2-modules":        "Auscultatiepunten",
        "h2-master":         "Master",
        "h2-recording":      "Opname",
        "h2-actions":        "Simulatie",
        "tab-cardiac":       "Cardiaal",
        "tab-pulmonary":     "Pulmonair",
        "path-NORMAL":              "Normaal",
        "path-AORTIC_STENOSIS":     "Aortastenose",
        "path-MITRAL_REGURGITATION":"Mitralisinsufficiëntie",
        "path-MITRAL_STENOSIS":     "Mitraalstenosе",
        "path-AORTIC_REGURGITATION":"Aorta-insufficiëntie",
        "path-S3_GALLOP":           "S3 Galop",
        "path-S4_GALLOP":           "S4 Galop",
        "path-CRACKLES_FINE":       "Fijne Crepitaties",
        "path-CRACKLES_COARSE":     "Grove Crepitaties",
        "path-WHEEZE_INSPIRATORY":  "Insp. Piepen",
        "path-WHEEZE_EXPIRATORY":   "Exp. Piepen",
        "lbl-severity-lo":   "mild",
        "lbl-severity-hi":   "ernstig",
        "lbl-audio":         "Audio",
        "lbl-haptic":        "Haptiek",
        "lbl-ptt":           "PTT offset",
        "lbl-ptt-lo":        "0 ms (geen correctie)",
        "lbl-ptt-hi":        "400 ms",
        "lbl-playback":      "Playback mode",
        "lbl-playback-sub":  "geen PPG nodig — backup voor demo",
        "btn-start":         "▶ Start simulatie",
        "btn-stop":          "Stop",
        "btn-rec-start":     "⏺ Opname",
        "btn-rec-stop":      "⏹ Stop",
        "btn-release":       "Vrijgeven",
    },
    en: {
        "h1-title":          "Heart Auscultation Simulator",
        "h2-pathology":      "Pathology",
        "h2-severity":       "Severity",
        "h2-modules":        "Auscultation points",
        "h2-master":         "Master",
        "h2-recording":      "Recording",
        "h2-actions":        "Simulation",
        "tab-cardiac":       "Cardiac",
        "tab-pulmonary":     "Pulmonary",
        "path-NORMAL":               "Normal",
        "path-AORTIC_STENOSIS":      "Aortic Stenosis",
        "path-MITRAL_REGURGITATION": "Mitral Regurgitation",
        "path-MITRAL_STENOSIS":      "Mitral Stenosis",
        "path-AORTIC_REGURGITATION": "Aortic Regurgitation",
        "path-S3_GALLOP":            "S3 Gallop",
        "path-S4_GALLOP":            "S4 Gallop",
        "path-CRACKLES_FINE":        "Fine Crackles",
        "path-CRACKLES_COARSE":      "Coarse Crackles",
        "path-WHEEZE_INSPIRATORY":   "Inspiratory Wheeze",
        "path-WHEEZE_EXPIRATORY":    "Expiratory Wheeze",
        "lbl-severity-lo":   "mild",
        "lbl-severity-hi":   "severe",
        "lbl-audio":         "Audio",
        "lbl-haptic":        "Haptic",
        "lbl-ptt":           "PTT offset",
        "lbl-ptt-lo":        "0 ms (no correction)",
        "lbl-ptt-hi":        "400 ms",
        "lbl-playback":      "Playback mode",
        "lbl-playback-sub":  "no PPG needed — demo backup",
        "btn-start":         "▶ Start simulation",
        "btn-stop":          "Stop",
        "btn-rec-start":     "⏺ Record",
        "btn-rec-stop":      "⏹ Stop",
        "btn-release":       "Release",
    },
    fr: {
        "h1-title":          "Simulateur d'Auscultation Cardiaque",
        "h2-pathology":      "Pathologie",
        "h2-severity":       "Sévérité",
        "h2-modules":        "Points d'auscultation",
        "h2-master":         "Maître",
        "h2-recording":      "Enregistrement",
        "h2-actions":        "Simulation",
        "tab-cardiac":       "Cardiaque",
        "tab-pulmonary":     "Pulmonaire",
        "path-NORMAL":               "Normal",
        "path-AORTIC_STENOSIS":      "Sténose aortique",
        "path-MITRAL_REGURGITATION": "Insuffisance mitrale",
        "path-MITRAL_STENOSIS":      "Sténose mitrale",
        "path-AORTIC_REGURGITATION": "Insuffisance aortique",
        "path-S3_GALLOP":            "Galop B3",
        "path-S4_GALLOP":            "Galop B4",
        "path-CRACKLES_FINE":        "Crépitants fins",
        "path-CRACKLES_COARSE":      "Crépitants grossiers",
        "path-WHEEZE_INSPIRATORY":   "Sibilance insp.",
        "path-WHEEZE_EXPIRATORY":    "Sibilance exp.",
        "lbl-severity-lo":   "léger",
        "lbl-severity-hi":   "sévère",
        "lbl-audio":         "Audio",
        "lbl-haptic":        "Haptique",
        "lbl-ptt":           "Décalage PTT",
        "lbl-ptt-lo":        "0 ms (sans correction)",
        "lbl-ptt-hi":        "400 ms",
        "lbl-playback":      "Mode lecture",
        "lbl-playback-sub":  "sans PPG — solution de secours",
        "btn-start":         "▶ Démarrer",
        "btn-stop":          "Arrêter",
        "btn-rec-start":     "⏺ Enregistrer",
        "btn-rec-stop":      "⏹ Arrêter",
        "btn-release":       "Libérer",
    }
};

const SUPPORTED_LANGS = Object.keys(TRANSLATIONS);

function detectLang() {
    // 1. URL hash  (#lang=fr)
    const m = location.hash.match(/lang=([a-z]{2})/);
    if (m && SUPPORTED_LANGS.includes(m[1])) return m[1];
    // 2. Browser language
    const nav = (navigator.language || "en").slice(0, 2).toLowerCase();
    return SUPPORTED_LANGS.includes(nav) ? nav : "en";
}

let currentLang = detectLang();

function t(key) {
    return (TRANSLATIONS[currentLang] || TRANSLATIONS.en)[key] || key;
}

function applyTranslations() {
    document.querySelectorAll("[data-i18n]").forEach(el => {
        const key = el.dataset.i18n;
        const val = t(key);
        if (el.tagName === "INPUT" || el.tagName === "BUTTON") {
            if (el.type !== "range" && el.type !== "checkbox") el.textContent = val;
        } else {
            el.textContent = val;
        }
    });
    // Pathology button labels
    document.querySelectorAll(".path-btn[data-id]").forEach(btn => {
        const key = "path-" + btn.dataset.id;
        if (TRANSLATIONS[currentLang]?.[key]) btn.textContent = t(key);
    });
    // Tab buttons
    document.querySelectorAll(".tab-btn[data-tab]").forEach(btn => {
        const key = "tab-" + btn.dataset.tab;
        if (TRANSLATIONS[currentLang]?.[key]) btn.textContent = t(key);
    });
    // Update html lang attribute
    document.documentElement.lang = currentLang;
}

function setLang(lang) {
    if (!SUPPORTED_LANGS.includes(lang)) return;
    currentLang = lang;
    // Persist in hash without triggering a reload
    history.replaceState(null, "", "#lang=" + lang);
    applyTranslations();
    // Highlight active button
    document.querySelectorAll(".lang-btn").forEach(b =>
        b.classList.toggle("active", b.dataset.lang === lang));
}

let myWsId     = null;   // assigned by server on WS connect
let operatorId = null;   // current operator's WS id (0 = nobody)

function isOperator() { return true; }  // Lock uitgeschakeld — altijd operator

const state = {
    bpm: 0,
    ibi: 0,
    systole_ms: 280,
    pathology: "NORMAL",
    severity: 3,
    running: false,
    playback: false,
    playback_bpm: 70,
    baseline_enabled: true,
    baseline_intensity: 8,
    modules: [true, true, true, true],
    audio: 180,
    haptic: 200,
    ptt_offset: 200,
    clients: 0,
    // UI-only state
    mode: "live",                // "live" of "demo"
    show_markers: true,
    show_pathology_overlay: false
};

const ppgBuffer = new Array(PPG_BUFFER_SIZE).fill(0);
let ppgChart = null;
let ws = null;

// ============================================================
// Chart setup — met S1/S2 markers plugin
// ============================================================
// PPG buffer = 240 samples = ~1 sec aan data (telemetry stuurt 12 samples
// per 50ms = 240 samples/sec). Met bekende BPM kunnen we voorspellen waar
// S1 en S2 in de buffer vallen.
const CHART_SAMPLE_RATE = 240;  // samples per seconde dat we naar de chart sturen

const heartMarkersPlugin = {
    id: "heartMarkers",
    afterDatasetsDraw(chart) {
        if (!state.show_markers && !state.show_pathology_overlay) return;
        const { ctx, chartArea: { left, right, top, bottom }, scales: { x, y } } = chart;
        if (!state.running || state.bpm <= 0) return;

        const ibi = state.ibi || 857;
        const sys = state.systole_ms || (ibi * 0.35);
        const samples_per_cycle = (ibi / 1000) * CHART_SAMPLE_RATE;
        const total = ppgBuffer.length;

        // De laatste sample is "nu". Plot van rechts naar links waar S1 events
        // gevallen zijn in het verleden, op intervallen van samples_per_cycle.
        ctx.save();

        // S1 markers (groene verticale lijn)
        if (state.show_markers) {
            ctx.strokeStyle = "#22d3a3";
            ctx.lineWidth = 1.5;
            ctx.setLineDash([2, 3]);
            for (let s = total - 1; s >= 0; s -= samples_per_cycle) {
                const xpos = left + (s / (total - 1)) * (right - left);
                ctx.beginPath();
                ctx.moveTo(xpos, top);
                ctx.lineTo(xpos, bottom);
                ctx.stroke();
                // Label "S1"
                ctx.fillStyle = "#22d3a3";
                ctx.font = "10px ui-monospace, monospace";
                ctx.fillText("S1", xpos + 2, top + 12);
            }

            // S2 markers (blauwe verticale lijn, sys ms na elke S1)
            const s2_offset = (sys / 1000) * CHART_SAMPLE_RATE;
            ctx.strokeStyle = "#60a5fa";
            for (let s = total - 1; s >= 0; s -= samples_per_cycle) {
                const s2 = s - (samples_per_cycle - s2_offset);
                if (s2 < 0) continue;
                const xpos = left + (s2 / (total - 1)) * (right - left);
                ctx.beginPath();
                ctx.moveTo(xpos, top);
                ctx.lineTo(xpos, bottom);
                ctx.stroke();
                ctx.fillStyle = "#60a5fa";
                ctx.fillText("S2", xpos + 2, top + 12);
            }
        }

        // Pathology-event overlay (oranje) — markeert pathologie-specifieke events
        if (state.show_pathology_overlay) {
            const evtInfo = pathologyEventOffset(state.pathology, ibi, sys);
            if (evtInfo) {
                const lbl = document.getElementById("legend-event-label");
                if (lbl) lbl.textContent = evtInfo.label;
                const off_samples = (evtInfo.offset_ms / 1000) * CHART_SAMPLE_RATE;
                ctx.strokeStyle = "#f59e0b";
                ctx.lineWidth = 2;
                ctx.setLineDash([4, 3]);
                for (let s = total - 1; s >= 0; s -= samples_per_cycle) {
                    const evt = s - (samples_per_cycle - off_samples);
                    if (evt < 0) continue;
                    const xpos = left + (evt / (total - 1)) * (right - left);
                    ctx.beginPath();
                    ctx.moveTo(xpos, top);
                    ctx.lineTo(xpos, bottom);
                    ctx.stroke();
                    ctx.fillStyle = "#f59e0b";
                    ctx.fillText(evtInfo.label, xpos + 2, bottom - 4);
                }
            }
        }
        ctx.restore();
    }
};

function pathologyEventOffset(p, ibi, sys) {
    switch (p) {
        case "S3_GALLOP":
            return { label: "S3", offset_ms: sys + 0.15 * ibi };
        case "S4_GALLOP":
            return { label: "S4", offset_ms: ibi > 80 ? ibi - 80 : 0 };
        case "MITRAL_STENOSIS":
            return { label: "OS", offset_ms: sys + 80 };   // opening snap
        case "AORTIC_STENOSIS":
        case "MITRAL_REGURGITATION":
            return { label: "Murmur", offset_ms: sys / 2 };
        case "AORTIC_REGURGITATION":
            return { label: "AR", offset_ms: sys + 0.2 * ibi };
        default:
            return null;
    }
}

function initChart() {
    const ctx = document.getElementById("ppg-chart").getContext("2d");
    ppgChart = new Chart(ctx, {
        type: "line",
        data: {
            labels: ppgBuffer.map((_, i) => i),
            datasets: [{
                data: ppgBuffer,
                borderColor: "#22d3a3",
                borderWidth: 1.8,
                pointRadius: 0,
                tension: 0.15,
                fill: false
            }]
        },
        options: {
            responsive: true,
            maintainAspectRatio: false,
            animation: false,
            scales: {
                x: { display: false },
                y: {
                    display: true,
                    grid:   { color: "rgba(255,255,255,0.05)" },
                    ticks:  { color: "#6b7280", font: { size: 10 } },
                    grace: "12%"
                }
            },
            plugins: { legend: { display: false }, tooltip: { enabled: false } }
        },
        plugins: [heartMarkersPlugin]
    });
}

function pushSamples(samples) {
    // Voeg N samples toe aan ringbuffer + werk chart bij
    for (let i = 0; i < samples.length; i++) {
        ppgBuffer.shift();
        ppgBuffer.push(samples[i]);
    }
    ppgChart.data.datasets[0].data = ppgBuffer;
    ppgChart.update("none");
}

// ============================================================
// WebSocket
// ============================================================
function openWebSocket() {
    const banner = document.getElementById("reconnect-banner");
    ws = new WebSocket(`ws://${location.host}/ws`);

    ws.onopen = () => {
        console.log("[WS] connected");
        banner.classList.add("hidden");
    };

    ws.onmessage = (evt) => {
        try {
            const msg = JSON.parse(evt.data);
            if (msg.type === "telemetry") {
                state.bpm = msg.bpm;
                state.ibi = msg.ibi;
                if ("systole_ms" in msg) state.systole_ms = msg.systole_ms;
                if ("pathology" in msg) state.pathology = msg.pathology;
                updateMetrics();
                pushSamples(msg.samples || []);
            } else {
                // Status snapshot bij eerste connect
                hydrateState(msg);
            }
        } catch (e) {
            console.error("[WS] parse error", e);
        }
    };

    ws.onclose = () => {
        console.warn("[WS] disconnected, reconnect in 2s");
        banner.classList.remove("hidden");
        setTimeout(openWebSocket, WS_RECONNECT_MS);
    };

    ws.onerror = () => ws.close();
}

// ============================================================
// State hydration + UI sync
// ============================================================
function hydrateState(snap) {
    if ("bpm" in snap)                 state.bpm = snap.bpm;
    if ("ibi_ms" in snap)              state.ibi = snap.ibi_ms;
    if ("systole_ms" in snap)          state.systole_ms = snap.systole_ms;
    if ("pathology" in snap)           state.pathology = snap.pathology;
    if ("severity" in snap)            state.severity = snap.severity;
    if ("running" in snap)             state.running = snap.running;
    if ("playback" in snap) {
        state.playback = snap.playback;
        state.mode = snap.playback ? "demo" : "live";
    }
    if ("playback_bpm" in snap)        state.playback_bpm = snap.playback_bpm;
    if ("baseline_enabled" in snap)    state.baseline_enabled = snap.baseline_enabled;
    if ("baseline_intensity" in snap)  state.baseline_intensity = snap.baseline_intensity;
    if ("modules" in snap)             state.modules = snap.modules;
    if ("audio" in snap)               state.audio = snap.audio;
    if ("haptic" in snap)              state.haptic = snap.haptic;
    if ("ptt_offset" in snap)          state.ptt_offset = snap.ptt_offset;
    if ("clients" in snap)             state.clients = snap.clients;
    if ("my_id" in snap)               myWsId = snap.my_id;
    if ("operator_id" in snap)         operatorId = snap.operator_id;
    renderAllUI();
    applyReadOnly();
}

function renderAllUI() {
    updateMetrics();
    updatePathologyButtons();
    updateSeveritySlider();
    updateModuleCards();
    updateMasterSliders();
    updateModeTabs();
    updatePlaybackBPM();
    updateBaselineControls();
    updatePTTSlider();
    updateLiveDot();
    document.getElementById("client-count").textContent = `${state.clients} client${state.clients === 1 ? "" : "s"}`;
}

function updateMetrics() {
    document.getElementById("bpm-value").textContent = state.bpm > 0 ? state.bpm : "--";
    document.getElementById("ibi-value").textContent = state.ibi > 0 ? state.ibi : "---";
}
function updatePathologyButtons() {
    document.querySelectorAll(".path-btn").forEach(btn => {
        btn.classList.toggle("active", btn.dataset.id === state.pathology);
    });
    // Auto-switch tab to match the active pathology
    const isPulm = PULMONARY_PATHS.has(state.pathology);
    document.getElementById("tab-cardiac").classList.toggle("hidden", isPulm);
    document.getElementById("tab-pulmonary").classList.toggle("hidden", !isPulm);
    document.querySelectorAll(".tab-btn").forEach(btn => {
        btn.classList.toggle("active",
            (btn.dataset.tab === "pulmonary") === isPulm);
    });
}
function updateSeveritySlider() {
    document.getElementById("severity-slider").value = state.severity;
    document.getElementById("severity-value").textContent = state.severity;
}
function updateModuleCards() {
    document.querySelectorAll(".module-card").forEach((card, i) => {
        const on = state.modules[i];
        card.classList.toggle("active", on);
        card.querySelector(".module-state").textContent = on ? "ON" : "OFF";
    });
}
function updateMasterSliders() {
    document.getElementById("audio-slider").value = state.audio;
    document.getElementById("haptic-slider").value = state.haptic;
    document.getElementById("audio-pct").textContent  = Math.round(state.audio  / 255 * 100);
    document.getElementById("haptic-pct").textContent = Math.round(state.haptic / 255 * 100);
}
function updateModeTabs() {
    document.querySelectorAll(".mode-tab").forEach(tab => {
        tab.classList.toggle("active", tab.dataset.mode === state.mode);
    });
    const demoPanel = document.getElementById("demo-panel");
    if (demoPanel) demoPanel.classList.toggle("hidden", state.mode !== "demo");
    const tag = document.getElementById("bpm-mode-tag");
    if (tag) {
        tag.textContent = state.mode === "demo" ? "Demo" : "Live";
        tag.classList.toggle("demo", state.mode === "demo");
    }
}
function updatePlaybackBPM() {
    const sl = document.getElementById("playback-bpm-slider");
    const v  = document.getElementById("playback-bpm-value");
    if (sl) sl.value = state.playback_bpm;
    if (v)  v.textContent = state.playback_bpm;
}
function updateBaselineControls() {
    const t = document.getElementById("baseline-toggle");
    const s = document.getElementById("baseline-intensity-slider");
    const v = document.getElementById("baseline-intensity-value");
    if (t) t.checked = state.baseline_enabled;
    if (s) s.value = state.baseline_intensity;
    if (v) v.textContent = state.baseline_intensity;
}
function updatePTTSlider() {
    document.getElementById("ptt-slider").value = state.ptt_offset;
    document.getElementById("ptt-value").textContent = state.ptt_offset;
}
function updateLiveDot() {
    document.getElementById("live-dot").classList.toggle("live", state.running);
}

// ============================================================
// REST POST helper met flash-feedback
// ============================================================
async function post(path, body, sourceEl) {
    try {
        const headers = { "Content-Type": "application/json" };
        if (myWsId !== null) headers["X-Operator-Id"] = String(myWsId);
        const resp = await fetch(path, {
            method: "POST",
            headers,
            body: JSON.stringify(body || {})
        });
        if (resp.status === 403) {
            console.warn("[POST] 403 read-only", path);
            return;
        }
        if (resp.ok && sourceEl) {
            sourceEl.classList.add("flash-ok");
            setTimeout(() => sourceEl.classList.remove("flash-ok"), 600);
        }
    } catch (e) {
        console.error("[POST]", path, e);
    }
}

function applyReadOnly() {
    const ro = !isOperator();
    document.querySelectorAll(
        ".path-btn, .module-card, .mode-tab, #severity-slider, #audio-slider, " +
        "#haptic-slider, #ptt-slider, #start-btn, #stop-btn, " +
        "#rec-start-btn, #rec-stop-btn, " +
        "#playback-bpm-slider, #baseline-toggle, #baseline-intensity-slider"
    ).forEach(el => { if (el) el.disabled = ro; });
    const badge = document.getElementById("operator-badge");
    if (badge) badge.textContent = ro ? "Read-only" : "Operator";
    badge && badge.classList.toggle("ro", ro);
    const relBtn = document.getElementById("release-btn");
    if (relBtn) relBtn.style.display = isOperator() ? "" : "none";
}

// ============================================================
// Event handlers
// ============================================================
function bindEvents() {
    // Tab bar (Cardiac / Pulmonary)
    document.querySelectorAll(".tab-btn").forEach(btn => {
        btn.addEventListener("click", () => {
            const isPulm = btn.dataset.tab === "pulmonary";
            document.getElementById("tab-cardiac").classList.toggle("hidden", isPulm);
            document.getElementById("tab-pulmonary").classList.toggle("hidden", !isPulm);
            document.querySelectorAll(".tab-btn").forEach(b =>
                b.classList.toggle("active", b === btn));
        });
    });

    // Pathology buttons (both tabs share the same selector)
    document.querySelectorAll(".path-btn").forEach(btn => {
        btn.addEventListener("click", () => {
            state.pathology = btn.dataset.id;
            updatePathologyButtons();
            post("/api/pathology", { id: state.pathology, severity: state.severity }, btn);
        });
    });

    // Severity slider
    const sevSlider = document.getElementById("severity-slider");
    sevSlider.addEventListener("input", () => {
        state.severity = parseInt(sevSlider.value, 10);
        document.getElementById("severity-value").textContent = state.severity;
    });
    sevSlider.addEventListener("change", () => {
        post("/api/pathology", { id: state.pathology, severity: state.severity }, sevSlider);
    });

    // Module cards
    document.querySelectorAll(".module-card").forEach((card, i) => {
        card.addEventListener("click", () => {
            state.modules[i] = !state.modules[i];
            updateModuleCards();
            post(`/api/module/${i}/enable`, { enabled: state.modules[i] }, card);
        });
    });

    // Master audio/haptic sliders
    const audioSlider  = document.getElementById("audio-slider");
    const hapticSlider = document.getElementById("haptic-slider");

    audioSlider.addEventListener("input", () => {
        state.audio = parseInt(audioSlider.value, 10);
        document.getElementById("audio-pct").textContent = Math.round(state.audio / 255 * 100);
    });
    audioSlider.addEventListener("change", () => {
        post("/api/volume/master", { value: state.audio }, audioSlider);
    });
    hapticSlider.addEventListener("input", () => {
        state.haptic = parseInt(hapticSlider.value, 10);
        document.getElementById("haptic-pct").textContent = Math.round(state.haptic / 255 * 100);
    });
    hapticSlider.addEventListener("change", () => {
        post("/api/haptic/master", { value: state.haptic }, hapticSlider);
    });

    // Start / Stop
    document.getElementById("start-btn").addEventListener("click", (e) => {
        state.running = true;
        updateLiveDot();
        post("/api/start", {}, e.target);
    });
    document.getElementById("stop-btn").addEventListener("click", (e) => {
        state.running = false;
        updateLiveDot();
        post("/api/stop", {}, e.target);
    });

    // Release operator lock
    document.getElementById("release-btn").addEventListener("click", () => {
        operatorId = null;
        myWsId     = null;
        applyReadOnly();
    });

    // Settings toggle
    document.getElementById("settings-btn").addEventListener("click", () => {
        document.getElementById("settings-panel").classList.toggle("hidden");
    });
    // Mode tabs: Live (gemeten) vs Demo (handmatige BPM)
    document.querySelectorAll(".mode-tab").forEach(tab => {
        tab.addEventListener("click", () => {
            const newMode = tab.dataset.mode;
            if (state.mode === newMode) return;
            state.mode = newMode;
            state.playback = (newMode === "demo");
            updateModeTabs();
            post("/api/playback_mode", { enabled: state.playback }, tab);
        });
    });

    // Demo BPM slider
    const pbBpm = document.getElementById("playback-bpm-slider");
    if (pbBpm) {
        pbBpm.addEventListener("input", () => {
            state.playback_bpm = parseInt(pbBpm.value, 10);
            document.getElementById("playback-bpm-value").textContent = state.playback_bpm;
        });
        pbBpm.addEventListener("change", () => {
            post("/api/playback_bpm", { value: state.playback_bpm }, pbBpm);
        });
    }

    // Baseline-haptic toggle + intensity
    const baseToggle = document.getElementById("baseline-toggle");
    if (baseToggle) {
        baseToggle.addEventListener("change", (e) => {
            state.baseline_enabled = e.target.checked;
            post("/api/baseline/enable", { enabled: state.baseline_enabled }, e.target);
        });
    }
    const baseSlider = document.getElementById("baseline-intensity-slider");
    if (baseSlider) {
        baseSlider.addEventListener("input", () => {
            state.baseline_intensity = parseInt(baseSlider.value, 10);
            document.getElementById("baseline-intensity-value").textContent = state.baseline_intensity;
        });
        baseSlider.addEventListener("change", () => {
            post("/api/baseline/intensity", { value: state.baseline_intensity }, baseSlider);
        });
    }

    // Marker toggles (UI-only, geen REST)
    const mt = document.getElementById("markers-toggle");
    if (mt) {
        mt.addEventListener("change", (e) => {
            state.show_markers = e.target.checked;
            if (ppgChart) ppgChart.update("none");
        });
    }
    const po = document.getElementById("pathology-overlay-toggle");
    if (po) {
        po.addEventListener("change", (e) => {
            state.show_pathology_overlay = e.target.checked;
            const legend = document.getElementById("legend-event");
            if (legend) legend.style.display = e.target.checked ? "" : "none";
            if (ppgChart) ppgChart.update("none");
        });
    }

    // PTT offset slider
    const pttSlider = document.getElementById("ptt-slider");
    pttSlider.addEventListener("input", () => {
        state.ptt_offset = parseInt(pttSlider.value, 10);
        document.getElementById("ptt-value").textContent = state.ptt_offset;
    });
    pttSlider.addEventListener("change", () => {
        post("/api/ptt_offset", { value: state.ptt_offset }, pttSlider);
    });
}

// ============================================================
// Diagnostics overlay  (triple-tap BPM number to open)
// ============================================================
let _diagTaps = 0, _diagTimer = null, _diagInterval = null;

function openDiag() {
    document.getElementById("diag-overlay").classList.remove("hidden");
    refreshDiag();
    _diagInterval = setInterval(refreshDiag, 1000);
}
function closeDiag() {
    document.getElementById("diag-overlay").classList.add("hidden");
    clearInterval(_diagInterval);
    _diagInterval = null;
}
async function refreshDiag() {
    try {
        const r = await fetch("/api/diag");
        if (!r.ok) return;
        const d = await r.json();
        const lines = [
            `uptime          ${d.uptime_s} s`,
            `free_heap       ${d.free_heap} B`,
            `min_free_heap   ${d.min_free_heap} B`,
            ``,
            `stack_loop      ${d.stack_loop ?? "?"} B free`,
            `stack_telemetry ${d.stack_telemetry ?? "?"} B free`,
            ``,
            `wifi_clients    ${d.wifi_clients}`,
            `ws_clients      ${d.ws_clients}`,
            ``,
            `pathology       ${d.pathology}`,
            `running         ${d.running}`,
            `bpm             ${d.bpm}`,
            `avg_ibi_ms      ${d.avg_ibi_ms}`,
            `ptt_offset      ${d.ptt_offset} ms`,
            `recording       ${d.recording}`,
        ];
        document.getElementById("diag-pre").textContent = lines.join("\n");
    } catch (e) {
        document.getElementById("diag-pre").textContent = "Error: " + e.message;
    }
}
function bindDiag() {
    // Triple-tap the BPM display
    document.getElementById("bpm-value").addEventListener("click", () => {
        _diagTaps++;
        clearTimeout(_diagTimer);
        _diagTimer = setTimeout(() => { _diagTaps = 0; }, 600);
        if (_diagTaps >= 3) { _diagTaps = 0; openDiag(); }
    });
    document.getElementById("diag-close").addEventListener("click", closeDiag);
    // Close on backdrop click
    document.getElementById("diag-overlay").addEventListener("click", e => {
        if (e.target === document.getElementById("diag-overlay")) closeDiag();
    });
}

// ============================================================
// Recording
// ============================================================
function formatBytes(b) {
    return b < 1024 ? `${b} B` : `${(b / 1024).toFixed(1)} KB`;
}

async function loadRecordings() {
    try {
        const resp = await fetch("/api/recordings");
        if (!resp.ok) return;
        const data = await resp.json();
        renderRecordings(data.recordings || []);
    } catch (e) { /* silent — not critical */ }
}

function renderRecordings(list) {
    const ul = document.getElementById("rec-list");
    ul.innerHTML = "";
    list.forEach(r => {
        const li = document.createElement("li");
        li.className = "rec-item";
        li.dataset.name = r.name;
        li.innerHTML = `<a href="${r.name}" download>${r.name.replace("/recordings/", "")}</a>` +
                       `<span class="rec-size">${formatBytes(r.size)}</span>` +
                       `<button class="rec-del-btn" title="Delete">🗑</button>`;
        li.querySelector(".rec-del-btn").addEventListener("click", async () => {
            const fd = new FormData();
            fd.append("name", r.name);
            await fetch("/api/recordings/delete", { method: "POST", body: fd });
            li.remove();
        });
        ul.appendChild(li);
    });
}

function bindRecording() {
    const startBtn  = document.getElementById("rec-start-btn");
    const stopBtn   = document.getElementById("rec-stop-btn");
    const statusEl  = document.getElementById("rec-status");

    startBtn.addEventListener("click", async () => {
        const resp = await fetch("/api/record/start", { method: "POST" });
        if (!resp.ok) return;
        startBtn.disabled = true;
        stopBtn.disabled  = false;
        statusEl.textContent = "Recording…";
        statusEl.classList.add("recording");
    });

    stopBtn.addEventListener("click", async () => {
        stopBtn.disabled = true;
        const resp = await fetch("/api/record/stop", { method: "POST" });
        startBtn.disabled = false;
        statusEl.classList.remove("recording");
        if (resp.ok) {
            const data = await resp.json();
            statusEl.textContent = `Saved: ${data.filename?.replace("/recordings/", "") ?? ""}`;
            loadRecordings();
        } else {
            statusEl.textContent = "Error — nothing captured";
        }
    });
}

// ============================================================
// Boot
// ============================================================
async function boot() {
    initChart();
    applyTranslations();
    bindEvents();
    bindRecording();
    bindDiag();
    // Language switcher buttons
    document.querySelectorAll(".lang-btn").forEach(btn =>
        btn.addEventListener("click", () => setLang(btn.dataset.lang)));
    setLang(currentLang); // apply active state

    try {
        const resp = await fetch("/api/status");
        if (resp.ok) hydrateState(await resp.json());
    } catch (e) {
        console.warn("[boot] /api/status mislukte, ga toch verder", e);
    }
    openWebSocket();
    loadRecordings();
}

boot();
