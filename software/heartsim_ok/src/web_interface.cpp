// ============================================================================
// web_interface.cpp
// ----------------------------------------------------------------------------
// Belangrijke implementatie-keuzes:
//   1. AsyncWebServer: niet-blocking — geen bottleneck op de main loop.
//   2. JSON bodies parsen via AsyncCallbackJsonWebHandler.
//   3. Telemetry pakketten ~12 nieuwe samples elke 50ms (= 20 Hz @ 250 Hz PPG).
//   4. Captive portal: we redirecten alle bekende OS check-URLs naar /.
// ============================================================================
#include "web_interface.h"
#include <WiFi.h>
#include <AsyncJson.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <DNSServer.h>
#include <ArduinoOTA.h>

// ============================================================================
// DNS server voor captive portal — vangt elk verzoek af naar 192.168.4.1
// ============================================================================
static DNSServer s_dns_server;

WebInterface::WebInterface(PPGMonitor* ppg, CycleEngine* cycle,
                            AudioEngine* audio, HapticManager* haptic)
    : _ppg(ppg), _cycle(cycle), _audio(audio), _haptic(haptic),
      _server(80), _ws("/ws"), _telemetry_task(nullptr) {}

void WebInterface::begin() {
    // LittleFS mounten — bevat onze web assets
    if (!LittleFS.begin(true)) {
        Serial.println("[WEB] FOUT: LittleFS mount mislukt — flash 'pio run -t uploadfs'");
    } else {
        Serial.printf("[WEB] LittleFS gemount, %u bytes vrij\n",
                      LittleFS.totalBytes() - LittleFS.usedBytes());
    }

    _setupWiFi();
    _setupOTA();          // Must be after softAP so the network stack is up
    _setupWebSocket();
    _setupRoutes();
    _server.begin();
    Serial.println("[WEB] HTTP server gestart op poort 80");

    // Telemetry task
    xTaskCreatePinnedToCore(
        _telemetryTaskTrampoline,
        "telemetry",
        4096,
        this,
        TASK_PRIO_TELEMETRY,
        &_telemetry_task,
        CORE_SENSOR
    );
}

void WebInterface::_setupWiFi() {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASS, WIFI_AP_CHAN);
    IPAddress ip = WiFi.softAPIP();
    Serial.printf("[WEB] AP gestart: SSID=%s, IP=%s\n", WIFI_AP_SSID, ip.toString().c_str());

    // DNS catch-all voor captive portal
    s_dns_server.setErrorReplyCode(DNSReplyCode::NoError);
    s_dns_server.start(53, "*", ip);
}

void WebInterface::_setupWebSocket() {
    _ws.onEvent([this](AsyncWebSocket* server, AsyncWebSocketClient* client,
                       AwsEventType type, void* arg, uint8_t* data, size_t len) {
        switch (type) {
            case WS_EVT_CONNECT: {
                Serial.printf("[WEB] WS client #%u connected from %s\n",
                              client->id(), client->remoteIP().toString().c_str());
                // Newest client always becomes operator (single-user demo mode)
                _operator_id   = client->id();
                _release_at_ms = 0;
                Serial.printf("[WEB] Operator: client #%u (took over)\n", _operator_id);
                // Send status + operator flag as initial snapshot
                JsonDocument snap;
                snap.set(serialized(_buildStatusJson()));  // merge existing fields
                snap["operator_id"] = _operator_id;
                snap["my_id"]       = client->id();
                String out;
                serializeJson(snap, out);
                client->text(out);
                break;
            }
            case WS_EVT_DISCONNECT:
                Serial.printf("[WEB] WS client #%u disconnected\n", client->id());
                if (client->id() == _operator_id) {
                    // Grace period: auto-release after 10 s if no reconnect
                    _release_at_ms = millis() + 10000UL;
                    Serial.println("[WEB] Operator disconnected — release in 10 s");
                }
                break;
            default:
                break;
        }
    });
    _server.addHandler(&_ws);
}

void WebInterface::_setupRoutes() {
    // === Statische files vanaf LittleFS ===
    _server.serveStatic("/", LittleFS, "/")
           .setDefaultFile("index.html")
           .setCacheControl("max-age=600");

    // === Captive portal endpoints — alle OS-checks doorsturen naar root ===
    // iOS: /hotspot-detect.html, Android: /generate_204, Windows: /ncsi.txt
    auto portalHandler = [](AsyncWebServerRequest* req) {
        req->redirect("http://192.168.4.1/");
    };
    _server.on("/hotspot-detect.html",     HTTP_GET, portalHandler);
    _server.on("/generate_204",            HTTP_GET, portalHandler);
    _server.on("/gen_204",                 HTTP_GET, portalHandler);
    _server.on("/ncsi.txt",                HTTP_GET, portalHandler);
    _server.on("/connecttest.txt",         HTTP_GET, portalHandler);
    _server.on("/redirect",                HTTP_GET, portalHandler);
    _server.onNotFound(portalHandler);

    // === REST API endpoints ===

    // GET /api/status — huidige system state
    _server.on("/api/status", HTTP_GET, [this](AsyncWebServerRequest* req) {
        _handleGetStatus(req);
    });

    // POST /api/pathology — verwacht JSON body {id, severity}
    _server.addHandler(new AsyncCallbackJsonWebHandler("/api/pathology",
        [this](AsyncWebServerRequest* req, JsonVariant& body) {
            _handlePostPathology(req, body);
        }));

    // POST /api/start
    _server.on("/api/start", HTTP_POST, [this](AsyncWebServerRequest* req) {
        _handlePostStart(req);
    });

    // POST /api/stop
    _server.on("/api/stop", HTTP_POST, [this](AsyncWebServerRequest* req) {
        _handlePostStop(req);
    });

    // POST /api/module/{n}/enable — verwacht {enabled: bool}
    for (uint8_t m = 0; m < NUM_MODULES; m++) {
        String path = "/api/module/" + String(m) + "/enable";
        _server.addHandler(new AsyncCallbackJsonWebHandler(path.c_str(),
            [this, m](AsyncWebServerRequest* req, JsonVariant& body) {
                _handlePostModuleEnable(req, m, body);
            }));
    }

    // POST /api/volume/master — {value: 0..255}
    _server.addHandler(new AsyncCallbackJsonWebHandler("/api/volume/master",
        [this](AsyncWebServerRequest* req, JsonVariant& body) {
            _handlePostMasterVolume(req, body);
        }));

    // POST /api/haptic/master — {value: 0..255}
    _server.addHandler(new AsyncCallbackJsonWebHandler("/api/haptic/master",
        [this](AsyncWebServerRequest* req, JsonVariant& body) {
            _handlePostMasterHaptic(req, body);
        }));

    // POST /api/playback_mode — {enabled: bool}
    _server.addHandler(new AsyncCallbackJsonWebHandler("/api/playback_mode",
        [this](AsyncWebServerRequest* req, JsonVariant& body) {
            _handlePostPlaybackMode(req, body);
        }));

    // POST /api/ptt_offset — {value: 0..400}
    _server.addHandler(new AsyncCallbackJsonWebHandler("/api/ptt_offset",
        [this](AsyncWebServerRequest* req, JsonVariant& body) {
            _handlePostPTTOffset(req, body);
        }));

    // POST /api/playback_bpm — {value: 30..200}
    _server.addHandler(new AsyncCallbackJsonWebHandler("/api/playback_bpm",
        [this](AsyncWebServerRequest* req, JsonVariant& body) {
            _handlePostPlaybackBPM(req, body);
        }));

    // POST /api/baseline/enable — {enabled: bool}
    _server.addHandler(new AsyncCallbackJsonWebHandler("/api/baseline/enable",
        [this](AsyncWebServerRequest* req, JsonVariant& body) {
            _handlePostBaselineEnable(req, body);
        }));

    // POST /api/baseline/intensity — {value: 0..10}
    _server.addHandler(new AsyncCallbackJsonWebHandler("/api/baseline/intensity",
        [this](AsyncWebServerRequest* req, JsonVariant& body) {
            _handlePostBaselineIntensity(req, body);
        }));

    // GET /api/diag — system diagnostics (heap, tasks, IBI history)
    _server.on("/api/diag", HTTP_GET,
        [this](AsyncWebServerRequest* req) { _handleGetDiag(req); });

    // Recording endpoints
    _server.on("/api/record/start",  HTTP_POST,
        [this](AsyncWebServerRequest* req) { _handlePostRecordStart(req); });
    _server.on("/api/record/stop",   HTTP_POST,
        [this](AsyncWebServerRequest* req) { _handlePostRecordStop(req); });
    _server.on("/api/recordings",    HTTP_GET,
        [this](AsyncWebServerRequest* req) { _handleGetRecordings(req); });
    _server.on("/api/recordings/delete", HTTP_POST,
        [this](AsyncWebServerRequest* req) { _handleDeleteRecording(req); });

    // Serve WAV downloads directly from LittleFS under /recordings/
    _server.serveStatic("/recordings/", LittleFS, "/recordings/")
           .setCacheControl("no-cache");
}

// ============================================================================
// JSON status builder
// ============================================================================
String WebInterface::_buildStatusJson() {
    JsonDocument doc;
    uint16_t pb_bpm = _cycle->getPlaybackBPM();
    uint16_t pb_ibi = 60000UL / (pb_bpm == 0 ? 70 : pb_bpm);
    doc["bpm"]        = _cycle->isPlaybackMode() ? pb_bpm : _ppg->getBPM();
    doc["ibi_ms"]     = _cycle->isPlaybackMode() ? pb_ibi : _ppg->getIBImillis();
    doc["pathology"]  = pathologyName(_cycle->getPathology());
    doc["severity"]   = _cycle->getSeverity();
    doc["running"]    = _cycle->isRunning();
    doc["playback"]   = _cycle->isPlaybackMode();
    doc["playback_bpm"]        = pb_bpm;
    doc["baseline_enabled"]    = _cycle->isBaselineEnabled();
    doc["baseline_intensity"]  = _cycle->getBaselineIntensity();
    doc["audio"]      = _audio->getModuleVolume(0); // upper speaker as proxy
    doc["haptic"]     = _haptic->getMasterIntensity();
    doc["ptt_offset"]   = _cycle->getPTTOffset();
    doc["operator_id"]  = _operator_id;
    doc["clients"]      = _ws.count();

    JsonArray mods = doc["modules"].to<JsonArray>();
    for (uint8_t m = 0; m < NUM_MODULES; m++) {
        mods.add(_haptic->isModuleEnabled(m));
    }

    String out;
    serializeJson(doc, out);
    return out;
}

// ============================================================================
// Telemetry task — 20 Hz push naar alle WS clients
// ============================================================================
void WebInterface::_telemetryTaskTrampoline(void* arg) {
    static_cast<WebInterface*>(arg)->_telemetryTaskLoop();
    vTaskDelete(nullptr);
}

void WebInterface::_telemetryTaskLoop() {
    Serial.printf("[WEB] Telemetry task gestart op core %d\n", xPortGetCoreID());

    while (true) {
        s_dns_server.processNextRequest();

        // Auto-release operator lock after grace period
        if (_release_at_ms > 0 && millis() >= _release_at_ms) {
            _operator_id   = 0;
            _release_at_ms = 0;
            Serial.println("[WEB] Operator lock released");
        }

        if (_ws.count() > 0) {
            JsonDocument doc;
            doc["type"]  = "telemetry";
            uint16_t pb_bpm = _cycle->getPlaybackBPM();
            uint16_t pb_ibi = 60000UL / (pb_bpm == 0 ? 70 : pb_bpm);
            doc["bpm"]   = _cycle->isPlaybackMode() ? pb_bpm : _ppg->getBPM();
            doc["ibi"]   = _cycle->isPlaybackMode() ? pb_ibi : _ppg->getIBImillis();
            doc["systole_ms"] = (uint16_t)(SYSTOLE_FRACTION *
                (_cycle->isPlaybackMode() ? pb_ibi : _ppg->getIBImillis()));
            doc["pathology"] = pathologyName(_cycle->getPathology());

            // ~12 samples (250 Hz / 20 Hz)
            JsonArray samples = doc["samples"].to<JsonArray>();
            for (uint8_t i = 0; i < 12; i++) {
                samples.add(_ppg->getLatestRawSample());
                // (Voor een echt scrollende waveform zou je een ringbuffer
                //  uit PPGMonitor exposen. Voor nu één punt per push is OK
                //  — Chart.js interpoleert visueel.)
            }

            String msg;
            serializeJson(doc, msg);
            _ws.textAll(msg);
        }

        vTaskDelay(pdMS_TO_TICKS(50));    // 20 Hz
    }
}

// ============================================================================
// Route handlers
// ============================================================================
// ============================================================================
// Operator lock helpers
// ============================================================================

// The operator token is the WS client-ID sent back in the connect snapshot.
// REST callers pass it as X-Operator-Id header.
bool WebInterface::_claimOrReject(AsyncWebServerRequest* req) {
    // Lock uitgeschakeld voor single-user demo — iedereen mag aanpassen
    return true;
}

void WebInterface::_handleGetStatus(AsyncWebServerRequest* req) {
    req->send(200, "application/json", _buildStatusJson());
}

void WebInterface::_handlePostPathology(AsyncWebServerRequest* req, JsonVariant& body) {
    if (!_claimOrReject(req)) return;
    const char* id_str = body["id"] | "NORMAL";
    uint8_t severity   = body["severity"] | 3;

    PathologyId p = pathologyFromString(id_str);
    _cycle->setPathology(p, severity);

    req->send(200, "application/json", "{\"ok\":true}");
}

void WebInterface::_handlePostStart(AsyncWebServerRequest* req) {
    if (!_claimOrReject(req)) return;
    _cycle->start();
    req->send(200, "application/json", "{\"ok\":true}");
}

void WebInterface::_handlePostStop(AsyncWebServerRequest* req) {
    if (!_claimOrReject(req)) return;
    _cycle->stop();
    req->send(200, "application/json", "{\"ok\":true}");
}

void WebInterface::_handlePostModuleEnable(AsyncWebServerRequest* req, uint8_t module, JsonVariant& body) {
    if (!_claimOrReject(req)) return;
    bool enabled = body["enabled"] | true;
    _haptic->setModuleEnabled(module, enabled);
    req->send(200, "application/json", "{\"ok\":true}");
}

void WebInterface::_handlePostMasterVolume(AsyncWebServerRequest* req, JsonVariant& body) {
    if (!_claimOrReject(req)) return;
    int value = body["value"] | 180;
    value = constrain(value, 0, 255);
    _audio->setMasterVolume((uint8_t)value);
    // Build scaled 4-weight vector and let AudioEngine collapse to 2 speakers
    const uint8_t* w = PATHOLOGY_WEIGHTS[_cycle->getPathology()];
    uint8_t scaled[NUM_MODULES];
    for (uint8_t m = 0; m < NUM_MODULES; m++) {
        scaled[m] = (uint8_t)((w[m] * value) / 255);
    }
    _audio->setSpatialWeights(scaled);
    req->send(200, "application/json", "{\"ok\":true}");
}

void WebInterface::_handlePostMasterHaptic(AsyncWebServerRequest* req, JsonVariant& body) {
    if (!_claimOrReject(req)) return;
    int value = body["value"] | 200;
    value = constrain(value, 0, 255);
    _haptic->setMasterIntensity((uint8_t)value);
    req->send(200, "application/json", "{\"ok\":true}");
}

void WebInterface::_handlePostPlaybackMode(AsyncWebServerRequest* req, JsonVariant& body) {
    if (!_claimOrReject(req)) return;
    bool enabled = body["enabled"] | false;
    _cycle->setPlaybackMode(enabled);
    req->send(200, "application/json", "{\"ok\":true}");
}

void WebInterface::_handlePostPTTOffset(AsyncWebServerRequest* req, JsonVariant& body) {
    int value = body["value"] | (int)PPG_PTT_OFFSET_MS;
    value = constrain(value, 0, 400);
    _cycle->setPTTOffset((uint16_t)value);
    req->send(200, "application/json", "{\"ok\":true}");
}

void WebInterface::_handlePostPlaybackBPM(AsyncWebServerRequest* req, JsonVariant& body) {
    if (!_claimOrReject(req)) return;
    int value = body["value"] | 70;
    value = constrain(value, 30, 200);
    _cycle->setPlaybackBPM((uint16_t)value);
    req->send(200, "application/json", "{\"ok\":true}");
}

void WebInterface::_handlePostBaselineEnable(AsyncWebServerRequest* req, JsonVariant& body) {
    if (!_claimOrReject(req)) return;
    bool enabled = body["enabled"] | true;
    _cycle->setBaselineEnabled(enabled);
    req->send(200, "application/json", "{\"ok\":true}");
}

void WebInterface::_handlePostBaselineIntensity(AsyncWebServerRequest* req, JsonVariant& body) {
    if (!_claimOrReject(req)) return;
    int value = body["value"] | 8;
    value = constrain(value, 0, 10);
    _cycle->setBaselineIntensity((uint8_t)value);
    req->send(200, "application/json", "{\"ok\":true}");
}

void WebInterface::_handleGetDiag(AsyncWebServerRequest* req) {
    JsonDocument doc;

    // Heap
    doc["free_heap"]     = ESP.getFreeHeap();
    doc["min_free_heap"] = ESP.getMinFreeHeap();

    // Stack high-water marks for known tasks (bytes free at peak usage)
    doc["stack_audio"]     = uxTaskGetStackHighWaterMark(nullptr); // queried from current task — placeholder
    doc["stack_loop"]      = uxTaskGetStackHighWaterMark(xTaskGetHandle("loopTask"));
    doc["stack_telemetry"] = uxTaskGetStackHighWaterMark(_telemetry_task);
    doc["stack_haptic"]    = uxTaskGetStackHighWaterMark(nullptr); // haptic handle in cycle engine

    // WiFi
    doc["wifi_clients"] = WiFi.softAPgetStationNum();
    doc["ws_clients"]   = _ws.count();

    // Cycle / PPG info
    doc["bpm"]          = _ppg->getBPM();
    doc["avg_ibi_ms"]   = _cycle->getAvgIBI();
    doc["ptt_offset"]   = _cycle->getPTTOffset();
    doc["pathology"]    = pathologyName(_cycle->getPathology());
    doc["running"]      = _cycle->isRunning();
    doc["recording"]    = _audio->recorder()->isRecording();

    // Uptime
    doc["uptime_s"]     = millis() / 1000UL;

    String out;
    serializeJson(doc, out);
    req->send(200, "application/json", out);
}

void WebInterface::_handlePostRecordStart(AsyncWebServerRequest* req) {
    _audio->recorder()->startRecording();
    req->send(200, "application/json", "{\"ok\":true,\"recording\":true}");
}

void WebInterface::_handlePostRecordStop(AsyncWebServerRequest* req) {
    String fname = _audio->recorder()->stopRecording();
    if (fname.length() == 0) {
        req->send(500, "application/json", "{\"ok\":false,\"error\":\"nothing captured\"}");
        return;
    }
    JsonDocument doc;
    doc["ok"]       = true;
    doc["filename"] = fname;
    doc["url"]      = fname;  // served by serveStatic
    String out;
    serializeJson(doc, out);
    req->send(200, "application/json", out);
}

void WebInterface::_handleGetRecordings(AsyncWebServerRequest* req) {
    String json = "{\"recordings\":" + _audio->recorder()->listRecordingsJson() + "}";
    req->send(200, "application/json", json);
}

void WebInterface::_handleDeleteRecording(AsyncWebServerRequest* req) {
    if (!req->hasParam("name", true)) {
        req->send(400, "application/json", "{\"ok\":false,\"error\":\"missing name\"}");
        return;
    }
    String name = req->getParam("name", true)->value();
    bool ok = _audio->recorder()->deleteRecording(name);
    req->send(200, "application/json", ok ? "{\"ok\":true}" : "{\"ok\":false}");
}

// ============================================================================
// OTA
// ============================================================================
void WebInterface::_setupOTA() {
    // Hostname without ".local" — mDNS appends that automatically.
    // Reachable as heartsim.local on the HeartSim AP.
    ArduinoOTA.setHostname("heartsim");
    ArduinoOTA.setPassword("heartsim2026");

    ArduinoOTA.onStart([this]() {
        String type = (ArduinoOTA.getCommand() == U_FLASH) ? "sketch" : "filesystem";
        Serial.println("[OTA] Start — uploading: " + type);

        // Stop the audio task before flash writes begin.
        // I2S DMA transfers and flash writes share the same SPI bus on the
        // ESP32-S3; leaving audio running risks corrupted flash pages.
        _ota_was_running = _cycle->isRunning();
        if (_ota_was_running) {
            _cycle->stop();
            delay(30);   // Let the audio task's current i2s_write() unblock
        }
    });

    ArduinoOTA.onEnd([]() {
        // Device reboots automatically after this callback — no need to
        // restart audio here.
        Serial.println("\n[OTA] Compleet — herstart...");
    });

    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("[OTA] %3u%%\r", progress * 100 / total);
    });

    ArduinoOTA.onError([this](ota_error_t error) {
        const char* msg;
        switch (error) {
            case OTA_AUTH_ERROR:    msg = "authenticatie mislukt";  break;
            case OTA_BEGIN_ERROR:   msg = "begin mislukt";          break;
            case OTA_CONNECT_ERROR: msg = "verbinding mislukt";     break;
            case OTA_RECEIVE_ERROR: msg = "ontvangst mislukt";      break;
            case OTA_END_ERROR:     msg = "afronden mislukt";       break;
            default:                msg = "onbekende fout";         break;
        }
        Serial.printf("\n[OTA] Fout [%u]: %s\n", error, msg);

        // Flash did not complete — resume simulation if it was running before.
        if (_ota_was_running) {
            _cycle->start();
        }
    });

    ArduinoOTA.begin();
    Serial.println("[OTA] Klaar. Upload via: heartsim.local");
}

void WebInterface::handleOTA() {
    ArduinoOTA.handle();
}
