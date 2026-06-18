// =============================================================
//  websocket_server.h — Project Flour (Flaura)
//  WebSocket Server for AquaNexus Pro Mobile App Communication
//  Transport: ESPAsyncWebServer WebSocket on port 81
//  Protocol:  JSON bi-directional packets
// =============================================================
#pragma once

#include <Arduino.h>

// ─────────────────────────────────────────────────────────────
//  WebSocket Configuration
// ─────────────────────────────────────────────────────────────
#define WS_PORT             81      // WebSocket server port
#define WS_MAX_CLIENTS      4       // Max simultaneous mobile connections
#define WS_BROADCAST_INTERVAL_MS  1000  // Push sensor data every 1s

// ─────────────────────────────────────────────────────────────
//  Command IDs (incoming from mobile app)
// ─────────────────────────────────────────────────────────────
#define CMD_PUMP_START      "pump_start"
#define CMD_PUMP_STOP       "pump_stop"
#define CMD_EMERGENCY_STOP  "emergency_stop"
#define CMD_SET_MODE        "set_mode"
#define CMD_SET_THRESHOLD   "set_threshold"
#define CMD_SET_PWM         "set_pwm"
#define CMD_FORCE_PUMP_START "force_pump_start"
#define CMD_SET_SCHEDULE_HRS "set_schedule_hrs"

// ─────────────────────────────────────────────────────────────
//  Callback function types — main.cpp registers these
// ─────────────────────────────────────────────────────────────
typedef void (*CmdPumpStartFn)();
typedef void (*CmdPumpStopFn)();
typedef void (*CmdEmergencyStopFn)();
typedef void (*CmdSetModeFn)(const char* mode);      // "manual" | "auto"
typedef void (*CmdSetThresholdFn)(uint8_t value);     // 0–100
typedef void (*CmdSetPwmFn)(uint8_t value);           // 0–255
typedef void (*CmdForcePumpStartFn)();
typedef void (*CmdSetScheduleHrsFn)(uint32_t hours);

// ─────────────────────────────────────────────────────────────
//  Public API
// ─────────────────────────────────────────────────────────────

/**
 * @brief  Initialise the AsyncWebServer + WebSocket on WS_PORT.
 *         Call once in setup() after Wi-Fi is connected.
 */
void ws_init();

/**
 * @brief  Register command callbacks from main firmware.
 *         Must be called before ws_init() or immediately after.
 */
void ws_registerCallbacks(
    CmdPumpStartFn      onPumpStart,
    CmdPumpStopFn       onPumpStop,
    CmdEmergencyStopFn  onEmergencyStop,
    CmdSetModeFn        onSetMode,
    CmdSetThresholdFn   onSetThreshold,
    CmdSetPwmFn         onSetPwm,
    CmdForcePumpStartFn onForcePumpStart,
    CmdSetScheduleHrsFn onSetScheduleHrs
);

/**
 * @brief  Broadcast current sensor/state data to ALL connected clients.
 *         Call this periodically from the main loop (WS_BROADCAST_INTERVAL_MS).
 *
 * @param moisture    Soil moisture 0–100 %
 * @param waterLevel  Water level 0–100 %
 * @param rssi        Wi-Fi RSSI (dBm)
 * @param pumpOn      Pump running state
 * @param tankEmpty   Tank empty state
 * @param mode        "manual" or "auto"
 * @param threshold   Auto-water threshold (0–100)
 * @param pwmDuty     Current PWM duty (0–255)
 */
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
    uint32_t    schedIntervalHrs
);

/**
 * @brief  Send a one-shot alert to all clients (e.g., tank empty alert).
 * @param alertType   Alert identifier string
 * @param message     Human-readable message
 */
void ws_sendAlert(const char* alertType, const char* message);

/**
 * @brief  Clean up stale WebSocket connections — call from loop().
 */
void ws_cleanup();

/**
 * @brief  Get the count of currently connected WebSocket clients.
 */
uint8_t ws_clientCount();
