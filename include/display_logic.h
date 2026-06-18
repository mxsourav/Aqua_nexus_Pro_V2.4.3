// =============================================================
//  display_logic.h — Project Flour (Flaura) Smart Planter
//  TFT Display Management Interface Declaration
//  Driver: ST7735  |  Library: TFT_eSPI
//  Resolution: 160×128 (Landscape)
// =============================================================
#pragma once

#include <Arduino.h>
#include <TFT_eSPI.h>
#include "config.h"

// ─────────────────────────────────────────────────────────────
//  Display State Machine — current view index
// ─────────────────────────────────────────────────────────────
extern uint8_t currentView;  // 0, 1, 2

// ─────────────────────────────────────────────────────────────
//  Public API
// ─────────────────────────────────────────────────────────────

/**
 * @brief  Initialise TFT hardware, set rotation, clear screen, draw splash.
 *         Must be called once in setup() before any draw calls.
 */
void display_init();

/**
 * @brief  Cycle to the next view (called from button ISR debounce handler).
 *         Wraps around after VIEW_COUNT views.
 */
void display_nextView();

/**
 * @brief  Set a specific view and trigger screen clear/redraw.
 */
void display_setView(uint8_t view, bool force = false);

/**
 * @brief  Render the currently active view with fresh sensor data.
 *         Call this at DISPLAY_REFRESH_INTERVAL_MS cadence from loop().
 *
 * @param  moisture     Soil moisture percentage 0–100
 * @param  waterLevel   Water level percentage 0–100
 * @param  pumpOn       true if pump is currently running
 * @param  tankEmpty    true if HC-SR04 reports tank empty
 * @param  rssi         Wi-Fi RSSI in dBm
 * @param  ssid         Connected SSID string
 * @param  localIP      Device IP address string
 * @param  threshold    Current auto-water threshold %
 * @param  uptime_s     System uptime in seconds
 */
void display_update(
    uint8_t     moisture,
    uint8_t     waterLevel,
    bool        pumpOn,
    bool        tankEmpty,
    int32_t     rssi,
    const char* ssid,
    const char* localIP,
    uint8_t     threshold,
    uint8_t     pwmValue,
    uint32_t    uptime_s
);

/**
 * @brief  Draw a full-screen splash / boot screen with project name.
 *         Automatically dismissed after SPLASH_DURATION_MS.
 */
void display_splash();

/**
 * @brief  Draw a centered message overlay (e.g. "PUMP ON", "TANK EMPTY!").
 * @param  msg    Text to display
 * @param  color  RGB565 text colour
 */
void display_overlay(const char* msg, uint16_t color);
