// ============================================================================
// web_interface.h — ESP32 web server + WebSocket telemetry
// ----------------------------------------------------------------------------
// ARCHITECTURE
//   - AsyncWebServer (esp32async/ESPAsyncWebServer library) on TCP port 80.
//     Non-blocking — does not stall the main loop while serving requests.
//   - Static HMI files (index.html, app.js, style.css, chart.min.js) live
//     in the LittleFS partition and are served with a 10-minute browser
//     cache header for fast subsequent page loads.
//   - WebSocket endpoint at /ws delivers a JSON telemetry frame every
//     50 ms (≈20 Hz). Each frame contains current BPM, IBI, systolic
//     duration, active pathology, and 20 raw PPG samples — enough for a
//     smooth analogue-looking waveform on the client.
//   - REST endpoints (all JSON POST) cover every interactive control on
//     the HMI: pathology selection, severity, per-module on/off, audio
//     and haptic master volumes, playback mode + BPM, baseline toggle +
//     intensity, PTT offset, recording start/stop/delete, and diagnostics.
//   - The ESP32 hosts its own WiFi access point ("HeartSim" / password
//     "heartsim2026") — no external router is required for the demo.
//   - A DNS server intercepts every domain lookup on the AP and points it
//     to 192.168.4.1. Combined with the captive-portal redirect handlers,
//     this makes most mobile operating systems open the HMI automatically
//     when the user joins the WiFi.
//   - Over-The-Air firmware updates via ArduinoOTA (mDNS hostname
//     "heartsim.local") so we can re-flash without disconnecting USB.
// ============================================================================
#pragma once
#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoOTA.h>
#include "config.h"
#include "ppg_monitor.h"
#include "cycle_engine.h"
#include "audio_engine.h"
#include "haptic_manager.h"

class WebInterface {
public:
    WebInterface(PPGMonitor* ppg, CycleEngine* cycle,
                 AudioEngine* audio, HapticManager* haptic);

    /**
     * Start WiFi AP, mount LittleFS, registreer routes, spawn telemetry task.
     * Initialiseert ook ArduinoOTA (hostname "heartsim", password "heartsim2026").
     */
    void begin();

    /**
     * Poll ArduinoOTA — roep aan vanuit loop(). Non-blocking.
     * Stopt de audio task bij OTA start, hervat bij fout.
     */
    void handleOTA();

private:
    PPGMonitor*    _ppg;
    CycleEngine*   _cycle;
    AudioEngine*   _audio;
    HapticManager* _haptic;

    AsyncWebServer _server;
    AsyncWebSocket _ws;
    TaskHandle_t   _telemetry_task;
    bool           _ota_was_running = false;

    // First-come operator lock
    uint32_t _operator_id    = 0;      // WS client ID of current operator (0 = none)
    uint32_t _release_at_ms  = 0;      // millis() at which lock auto-releases after disconnect

    bool _isOperator(AsyncWebServerRequest* req) const;
    bool _claimOrReject(AsyncWebServerRequest* req);  // returns false + sends 403 if not operator

    void _setupWiFi();
    void _setupRoutes();
    void _setupWebSocket();
    void _setupOTA();

    // Status-helper — bouwt JSON met huidige system state
    String _buildStatusJson();

    static void _telemetryTaskTrampoline(void* arg);
    void _telemetryTaskLoop();

    // Handler-helpers (callbacks)
    void _handleGetStatus(AsyncWebServerRequest* req);
    void _handlePostPathology(AsyncWebServerRequest* req, JsonVariant& body);
    void _handlePostStart(AsyncWebServerRequest* req);
    void _handlePostStop(AsyncWebServerRequest* req);
    void _handlePostModuleEnable(AsyncWebServerRequest* req, uint8_t module, JsonVariant& body);
    void _handlePostMasterVolume(AsyncWebServerRequest* req, JsonVariant& body);
    void _handlePostMasterHaptic(AsyncWebServerRequest* req, JsonVariant& body);
    void _handlePostPlaybackMode(AsyncWebServerRequest* req, JsonVariant& body);
    void _handlePostPlaybackBPM(AsyncWebServerRequest* req, JsonVariant& body);
    void _handlePostBaselineEnable(AsyncWebServerRequest* req, JsonVariant& body);
    void _handlePostBaselineIntensity(AsyncWebServerRequest* req, JsonVariant& body);
    void _handlePostPTTOffset(AsyncWebServerRequest* req, JsonVariant& body);

    // Recording
    void _handlePostRecordStart(AsyncWebServerRequest* req);
    void _handlePostRecordStop(AsyncWebServerRequest* req);
    void _handleGetRecordings(AsyncWebServerRequest* req);
    void _handleDeleteRecording(AsyncWebServerRequest* req);

    // Diagnostics
    void _handleGetDiag(AsyncWebServerRequest* req);
};
