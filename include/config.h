//  config.h — Nexus Pro Smart Planter
//  Central configuration: credentials, pin map, thresholds
//  MCU: ESP32 38-Pin DevKit V1
// =============================================================
#pragma once

#include <Arduino.h>

// ─────────────────────────────────────────────────────────────
//  Section 1: Wi-Fi Credentials & Secrets Inclusion
// ─────────────────────────────────────────────────────────────
#include "secrets.h"
#define DEVICE_HOSTNAME     "flura"

// ─────────────────────────────────────────────────────────────
//  Section 2: Blynk IoT Configuration
//  Create a template at https://blynk.cloud → New Template
//  Virtual Pins used by this firmware:
//    V0  → Moisture %          (Value Display widget)
//    V1  → Water Level %       (Value Display widget)
//    V2  → Pump Status         (LED widget)
//    V3  → Manual Pump Button  (Button widget, Push mode)
//    V4  → Moisture Threshold  (Slider widget, range 0-100)
//    V5  → Pump Duty Cycle %   (Slider widget, range 0-100)
//    V6  → Event Log           (Terminal / SuperChart)
// ─────────────────────────────────────────────────────────────
#define BLYNK_TEMPLATE_ID   "TMPL3uh2Rl7fq"
#define BLYNK_TEMPLATE_NAME "Project Flour"

// ─────────────────────────────────────────────────────────────
//  Section 3: Hardware Pin Definitions
// ─────────────────────────────────────────────────────────────

// --- TFT ST7735 Display (SPI) ---
// Configured via PlatformIO build_flags in platformio.ini
// SCL (CLK) = GPIO18, SDA (MOSI) = GPIO23
// RES = GPIO4, DC = GPIO22, CS = GPIO5, BL = GPIO21
#define PIN_TFT_CS          5
#define PIN_TFT_RST         4
#define PIN_TFT_DC          22
#define PIN_TFT_MOSI        23
#define PIN_TFT_SCK         18
#define PIN_TFT_BL          21

// --- HC-SR04 Ultrasonic Water Level Sensor ---
#define PIN_TRIG            33    // Trigger output
#define PIN_ECHO            35    // Echo input

// --- Capacitive Soil Moisture Sensor v1.2 (ADC) ---
#define PIN_MOISTURE        34    // ADC1 CH6 — must use ADC1 with Wi-Fi

// --- IRLZ44N MOSFET (Pump Control, PWM) ---
#define PIN_PUMP_PWM        25    // LEDC PWM output to MOSFET gate

// --- Active Buzzer (5V, Active-high) ---
#define PIN_BUZZER          26

// --- Mode / Cycle Push Button (Internal Pull-up) ---
#define PIN_BUTTON          27
#define ENABLE_VIEW_BUTTON  1

// --- Float Switch (Alternative tank-empty sensor) ---
// Configured as INPUT_PULLUP; LOW = water present, HIGH = empty
// Pin 13 is connected but HC-SR04 is primary per user spec.
#define PIN_FLOAT_SW        13
#define ENABLE_FLOAT_SWITCH 0

// ─────────────────────────────────────────────────────────────
//  Section 4: Sensor Calibration
// ─────────────────────────────────────────────────────────────

// Capacitive moisture sensor raw ADC range
// Measure in air (dry) → AIR_VALUE, submerged (wet) → WATER_VALUE
#define MOISTURE_AIR_VALUE    2950    // ADC reading in open air (0%)
#define MOISTURE_WATER_VALUE   850    // ADC reading fully submerged (100%)
#define MOISTURE_RAW_MIN_VALID  50    // below this likely short / invalid
#define MOISTURE_RAW_MAX_VALID 4000   // above this likely floating / invalid

// HC-SR04 tank geometry
// Distance in cm from sensor to water surface when tank is FULL
#define TANK_FULL_DISTANCE_CM    0.5f

// Distance threshold: tank is considered EMPTY above this
#define TANK_EMPTY_THRESHOLD_CM  6.0f

// Maximum measurable distance (timeout guard)
#define ULTRASONIC_MAX_CM        200.0f

// ─────────────────────────────────────────────────────────────
//  Section 5: Irrigation & Control Parameters
// ─────────────────────────────────────────────────────────────

// Default moisture threshold below which auto-watering triggers (%)
#define DEFAULT_MOISTURE_THRESHOLD  35

// Default pump duty cycle for PWM soft-start (0–255 = 0–100%)
// Soft ramp: starts at PUMP_RAMP_START, ramps to PUMP_TARGET in RAMP_STEPS
#define PUMP_RAMP_START     80      // ~31% duty — gentle start
#define PUMP_TARGET_DUTY    200     // ~78% duty — nominal run speed
#define PUMP_RAMP_STEPS     10      // number of increment steps
#define PUMP_RAMP_DELAY_MS  50      // ms between ramp steps

// Maximum watering duration per session (ms) — safety cap
#define PUMP_MAX_DURATION_MS    30000   // 30 seconds

// Minimum off-time between pump cycles (ms) — prevents hammering
#define PUMP_MIN_INTERVAL_MS    120000  // 2 minutes

// ─────────────────────────────────────────────────────────────
//  Section 6: Polling & Update Intervals
// ─────────────────────────────────────────────────────────────
#define SENSOR_POLL_INTERVAL_MS     2000    // Read sensors every 2 s
#define BLYNK_UPDATE_INTERVAL_MS    5000    // Push to Blynk every 5 s
#define DISPLAY_REFRESH_INTERVAL_MS 1000    // Redraw TFT every 1 s
#define ULTRASONIC_POLL_INTERVAL_MS 3000    // HC-SR04 read every 3000 ms

// ─────────────────────────────────────────────────────────────
//  Section 7: PWM (LEDC) Configuration
// ─────────────────────────────────────────────────────────────
#define LEDC_CHANNEL        0       // LEDC channel 0
#define LEDC_TIMER_BITS     8       // 8-bit resolution (0–255)
#define LEDC_BASE_FREQ      1000    // 1 kHz — quiet pump operation

// ─────────────────────────────────────────────────────────────
//  Section 8: Buzzer Patterns
// ─────────────────────────────────────────────────────────────
#define BUZZ_SHORT_MS       200
#define BUZZ_LONG_MS        800
#define BUZZ_TANK_REPEAT    3       // Beeps for tank-empty alarm
#define BUZZ_ALARM_COOLDOWN_MS  30000
#define ENABLE_UI_BEEPS         1

// ─────────────────────────────────────────────────────────────
//  Section 9: TFT Display Layout (128 W × 160 H — Landscape)
//  In landscape mode width = 160, height = 128
// ─────────────────────────────────────────────────────────────
#define TFT_LANDSCAPE_W     160
#define TFT_LANDSCAPE_H     128

// Colour Palette (RGB565)
#define COLOR_BG            0x0000   // Pure black
#define COLOR_HEADER_BG     0x0820   // Dark teal-green
#define COLOR_ACCENT        0x07FF   // Cyan
#define COLOR_WATER_BLUE    0x5D9B   // Brighter Sky Blue
#define COLOR_GREEN         0x07E0   // Bright green
#define COLOR_YELLOW        0xFFE0   // Bright yellow
#define COLOR_ORANGE        0xFD20   // Orange
#define COLOR_RED           0xF800   // Red
#define COLOR_WHITE         0xFFFF
#define COLOR_GREY          0xFFFF
#define COLOR_DARKGREY      0xFFFF
#define COLOR_STATUS_GOOD   0x07E0   // Green for OK
#define COLOR_STATUS_WARN   0xFFE0   // Yellow for warning
#define COLOR_STATUS_ERR    0xF800   // Red for error

// Number of display views cycled by the mode button
#define VIEW_COUNT          3
//  View 0: Main Stats Dashboard (Moisture + Water Level bars + Status)
//  View 1: Wi-Fi & IoT Info
//  View 2: System Info (uptime, firmware version, thresholds)

// ─────────────────────────────────────────────────────────────
//  Section 10: Firmware Version
// ─────────────────────────────────────────────────────────────
#define FW_VERSION          "2.4.3"
#define PROJECT_NAME        "Nexus Pro"
