// =============================================================
//  websocket_server.cpp — Project Flour (Flaura)
//  WebSocket Server Implementation using ESPAsyncWebServer
//  Handles bi-directional JSON communication with mobile app
// =============================================================

#include "websocket_server.h"
#include "config.h"

#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

// ─────────────────────────────────────────────────────────────
//  Static Instances
// ─────────────────────────────────────────────────────────────
static AsyncWebServer  server(WS_PORT);
static AsyncWebSocket  ws("/ws");       // WebSocket endpoint: ws://<ip>:81/ws
extern volatile bool g_bypassUltrasonic;

// ─────────────────────────────────────────────────────────────
//  Registered Command Callbacks
// ─────────────────────────────────────────────────────────────
static CmdPumpStartFn      cb_pumpStart     = nullptr;
static CmdPumpStopFn       cb_pumpStop      = nullptr;
static CmdEmergencyStopFn  cb_emergencyStop = nullptr;
static CmdSetModeFn        cb_setMode       = nullptr;
static CmdSetThresholdFn   cb_setThreshold  = nullptr;
static CmdSetPwmFn         cb_setPwm        = nullptr;
static CmdForcePumpStartFn cb_forcePumpStart = nullptr;
static CmdSetScheduleHrsFn cb_setScheduleHrs = nullptr;

// ─────────────────────────────────────────────────────────────
//  Process Incoming JSON Command
//  Expected format:
//    {"cmd": "pump_start"}
//    {"cmd": "set_mode",      "value": "manual"}
//    {"cmd": "set_threshold", "value": 45}
//    {"cmd": "set_pwm",       "value": 180}
//    {"cmd": "emergency_stop"}
// ─────────────────────────────────────────────────────────────
static void processCommand(uint8_t clientId, const char* payload, size_t len)
{
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, payload, len);

    if (err) {
        Serial.printf("[WS] JSON parse error from client %u: %s\n",
                      clientId, err.c_str());
        // Send error response back
        char resp[128];
        snprintf(resp, sizeof(resp),
                 "{\"type\":\"error\",\"msg\":\"Invalid JSON: %s\"}", err.c_str());
        ws.text(clientId, resp);
        return;
    }

    const char* cmd = doc["cmd"] | "";
    Serial.printf("[WS] Client %u → cmd: '%s'\n", clientId, cmd);

    // Dispatch command to registered callback
    if (strcmp(cmd, CMD_PUMP_START) == 0) {
        if (cb_pumpStart) cb_pumpStart();
        ws.text(clientId, "{\"type\":\"ack\",\"cmd\":\"pump_start\",\"ok\":true}");

    } else if (strcmp(cmd, CMD_FORCE_PUMP_START) == 0) {
        if (cb_forcePumpStart) cb_forcePumpStart();
        ws.text(clientId, "{\"type\":\"ack\",\"cmd\":\"force_pump_start\",\"ok\":true}");

    } else if (strcmp(cmd, CMD_PUMP_STOP) == 0) {
        if (cb_pumpStop) cb_pumpStop();
        ws.text(clientId, "{\"type\":\"ack\",\"cmd\":\"pump_stop\",\"ok\":true}");

    } else if (strcmp(cmd, CMD_EMERGENCY_STOP) == 0) {
        if (cb_emergencyStop) cb_emergencyStop();
        ws.text(clientId, "{\"type\":\"ack\",\"cmd\":\"emergency_stop\",\"ok\":true}");

    } else if (strcmp(cmd, CMD_SET_MODE) == 0) {
        const char* mode = doc["value"] | "auto";
        if (cb_setMode) cb_setMode(mode);
        char resp[96];
        snprintf(resp, sizeof(resp),
                 "{\"type\":\"ack\",\"cmd\":\"set_mode\",\"value\":\"%s\",\"ok\":true}", mode);
        ws.text(clientId, resp);

    } else if (strcmp(cmd, CMD_SET_THRESHOLD) == 0) {
        uint8_t val = (uint8_t)constrain(doc["value"] | 40, 0, 100);
        if (cb_setThreshold) cb_setThreshold(val);
        char resp[96];
        snprintf(resp, sizeof(resp),
                 "{\"type\":\"ack\",\"cmd\":\"set_threshold\",\"value\":%d,\"ok\":true}", val);
        ws.text(clientId, resp);

    } else if (strcmp(cmd, CMD_SET_PWM) == 0) {
        uint8_t val = (uint8_t)constrain(doc["value"] | 200, 0, 255);
        if (cb_setPwm) cb_setPwm(val);
        char resp[96];
        snprintf(resp, sizeof(resp),
                 "{\"type\":\"ack\",\"cmd\":\"set_pwm\",\"value\":%d,\"ok\":true}", val);
        ws.text(clientId, resp);

    } else if (strcmp(cmd, CMD_SET_SCHEDULE_HRS) == 0) {
        uint32_t val = (uint32_t)(doc["value"] | 0);
        if (cb_setScheduleHrs) cb_setScheduleHrs(val);
        char resp[96];
        snprintf(resp, sizeof(resp),
                 "{\"type\":\"ack\",\"cmd\":\"set_schedule_hrs\",\"value\":%u,\"ok\":true}", val);
        ws.text(clientId, resp);

    } else if (strcmp(cmd, "set_bypass_us") == 0) {
        bool val = doc["value"] | false;
        g_bypassUltrasonic = val;
        char resp[96];
        snprintf(resp, sizeof(resp),
                 "{\"type\":\"ack\",\"cmd\":\"set_bypass_us\",\"value\":%s,\"ok\":true}", val ? "true" : "false");
        ws.text(clientId, resp);

    } else {
        char resp[96];
        snprintf(resp, sizeof(resp),
                 "{\"type\":\"error\",\"msg\":\"Unknown cmd: %s\"}", cmd);
        ws.text(clientId, resp);
    }
}

// ─────────────────────────────────────────────────────────────
//  WebSocket Event Handler
// ─────────────────────────────────────────────────────────────
static void onWsEvent(AsyncWebSocket* server, AsyncWebSocketClient* client,
                       AwsEventType type, void* arg, uint8_t* data, size_t len)
{
    switch (type) {
        case WS_EVT_CONNECT:
            Serial.printf("[WS] Client #%u connected from %s\n",
                          client->id(), client->remoteIP().toString().c_str());
            // Send welcome + current state immediately
            client->text("{\"type\":\"welcome\",\"device\":\"Nexus Pro\",\"fw\":\"" FW_VERSION "\"}");
            break;

        case WS_EVT_DISCONNECT:
            Serial.printf("[WS] Client #%u disconnected\n", client->id());
            break;

        case WS_EVT_DATA: {
            AwsFrameInfo* info = (AwsFrameInfo*)arg;
            if (info->final && info->index == 0 && info->len == len) {
                // Complete single-frame text message
                if (info->opcode == WS_TEXT) {
                    data[len] = 0;  // null-terminate
                    processCommand(client->id(), (const char*)data, len);
                }
            }
            break;
        }

        case WS_EVT_PONG:
            Serial.printf("[WS] Pong from client #%u\n", client->id());
            break;

        case WS_EVT_ERROR:
            Serial.printf("[WS] Error from client #%u: %u — %s\n",
                          client->id(), *((uint16_t*)arg), (char*)data);
            break;
    }
}

// =============================================================
//  Public API Implementation
// =============================================================

void ws_registerCallbacks(
    CmdPumpStartFn      onPumpStart,
    CmdPumpStopFn       onPumpStop,
    CmdEmergencyStopFn  onEmergencyStop,
    CmdSetModeFn        onSetMode,
    CmdSetThresholdFn   onSetThreshold,
    CmdSetPwmFn         onSetPwm,
    CmdForcePumpStartFn onForcePumpStart,
    CmdSetScheduleHrsFn onSetScheduleHrs)
{
    cb_pumpStart     = onPumpStart;
    cb_pumpStop      = onPumpStop;
    cb_emergencyStop = onEmergencyStop;
    cb_setMode       = onSetMode;
    cb_setThreshold  = onSetThreshold;
    cb_setPwm        = onSetPwm;
    cb_forcePumpStart = onForcePumpStart;
    cb_setScheduleHrs = onSetScheduleHrs;
}

void ws_init()
{
    ws.onEvent(onWsEvent);
    server.addHandler(&ws);

    // Serve a tiny health-check page at http://<ip>:81/
    server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->send(200, "text/html",
            "<html><body style='background:#0a0a1a;color:#0ff;font-family:monospace;'>"
            "<h1>Nexus Pro Smart Planter</h1>"
            "<p>WebSocket: ws://" + WiFi.localIP().toString() + ":81/ws</p>"
            "<p>Firmware: v" FW_VERSION "</p>"
            "<p>Clients: " + String(ws.count()) + "</p>"
            "</body></html>");
    });

    server.begin();
    Serial.printf("[WS] Server started on port %d — ws://%s:%d/ws\n",
                  WS_PORT, WiFi.localIP().toString().c_str(), WS_PORT);
}

void ws_broadcastSensorData(
    uint8_t     moisture,
    uint8_t     waterLevel,
    int32_t     rssi,
    bool        pumpOn,
    bool        tankEmpty,
    const char* mode,
    uint8_t     threshold,
    uint8_t     pwmDuty,
    uint32_t    schedNextEpoch,
    uint32_t    schedIntervalHrs)
{
    if (ws.count() == 0) return;  // No clients — skip serialization

    JsonDocument doc;
    doc["type"]       = "sensor_data";
    doc["moisture"]   = moisture;
    doc["water_lvl"]  = waterLevel;
    doc["rssi"]       = rssi;
    doc["pump_on"]    = pumpOn;
    doc["tank_empty"] = tankEmpty;
    doc["mode"]       = mode;
    doc["threshold"]  = threshold;
    doc["pwm"]        = pwmDuty;
    doc["uptime"]     = millis() / 1000;
    doc["schedNextEpoch"]   = schedNextEpoch;
    doc["schedIntervalHrs"] = schedIntervalHrs;
    doc["bypass_us"]        = g_bypassUltrasonic;

    char buffer[256];
    size_t len = serializeJson(doc, buffer, sizeof(buffer));
    ws.textAll(buffer, len);
}

void ws_sendAlert(const char* alertType, const char* message)
{
    if (ws.count() == 0) return;

    JsonDocument doc;
    doc["type"]      = "alert";
    doc["alert"]     = alertType;
    doc["message"]   = message;
    doc["timestamp"] = millis() / 1000;

    char buffer[256];
    size_t len = serializeJson(doc, buffer, sizeof(buffer));
    ws.textAll(buffer, len);
}

void ws_cleanup()
{
    ws.cleanupClients(WS_MAX_CLIENTS);
}

uint8_t ws_clientCount()
{
    return ws.count();
}
