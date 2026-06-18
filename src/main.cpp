// =============================================================
//  main.cpp — Nexus Pro Smart Planter v2.4.3
//  IoT Smart Planter — Production Firmware
//
//  MCU     : ESP32 38-Pin DevKit V1
//  Platform: PlatformIO + Arduino framework
//  IoT     : Blynk + WebSocket Server (Nexus Pro Mobile App)
//  Display : ST7735 1.8" TFT via TFT_eSPI
//
//  Architecture:
//    • Non-blocking millis()-based cooperative scheduler
//    • Interrupt-driven button with software debounce
//    • Digital ON/OFF pump control (IRLZ44N MOSFET)
//    • Dual-mode watering: moisture-threshold + schedule timer
//    • NTP time sync + Blynk V7/V8 persistent schedule storage
//    • WebSocket JSON server on port 81 for mobile app
//    • Blynk virtual pin bi-directional sync
// =============================================================

// ─────────────────────────────────────────────────────────────
//  Includes
// ─────────────────────────────────────────────────────────────
#define BLYNK_TEMPLATE_ID   "TMPL3uh2Rl7fq"
#define BLYNK_TEMPLATE_NAME "Project Flour"
#define BLYNK_AUTH_TOKEN    "Op5HKfSQ0iNTpLoe8IEGMJoc-lsfAeVS"

#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <BlynkSimpleEsp32.h>
#include <HCSR04.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

#include "config.h"
#include "display_logic.h"
#include "websocket_server.h"

// ─────────────────────────────────────────────────────────────
//  Global Sensor State
// ─────────────────────────────────────────────────────────────
struct SensorData {
    uint8_t  moisturePct   = 0;
    uint8_t  waterLevelPct = 0;
    float    distanceCm    = 0.0f;
    bool     tankEmpty     = false;
    bool     moistureValid = false;
    bool     ultrasonicValid = false;
    int32_t  rssi          = -100;
    uint32_t uptimeSeconds = 0;
} sensors;

// ─────────────────────────────────────────────────────────────
//  Pump State Machine
// ─────────────────────────────────────────────────────────────
struct PumpState {
    bool     running        = false;
    bool     manualOverride = false;
    bool     forceOverride  = false;
    uint8_t  currentDuty    = 0;
    uint32_t startedAt      = 0;
    uint32_t lastStoppedAt  = 0;
    uint8_t  targetDuty     = PUMP_TARGET_DUTY;
} pump;

// ─────────────────────────────────────────────────────────────
//  Operating Mode ("manual" or "auto")
// ─────────────────────────────────────────────────────────────
static char g_operatingMode[16] = "auto";
volatile bool g_bypassUltrasonic = false;

// ─────────────────────────────────────────────────────────────
//  User-configurable thresholds
// ─────────────────────────────────────────────────────────────
volatile uint8_t g_moistureThreshold = DEFAULT_MOISTURE_THRESHOLD;
static uint8_t g_pumpPwmRaw        = 255;
static uint8_t g_pumpDutyCyclePct  = 100;

// ─────────────────────────────────────────────────────────────
//  Button ISR state
// ─────────────────────────────────────────────────────────────
volatile bool     g_buttonPressed = false;
volatile uint32_t g_lastButtonMs  = 0;
#define DEBOUNCE_MS 200

// --- Button-triggered temporary pump run (10 seconds) ---
volatile bool     g_buttonPumpActive = false;
volatile uint32_t g_buttonPumpStopMs = 0;

// ─────────────────────────────────────────────────────────────
//  Scheduler timestamps
// ─────────────────────────────────────────────────────────────
static uint32_t t_sensorPoll     = 0;
static uint32_t t_ultrasonicPoll = 0;
static uint32_t t_blynkUpdate    = 0;
static uint32_t t_displayRefresh = 0;
static uint32_t t_pumpWatchdog   = 0;
static uint32_t t_wsBroadcast    = 0;
static uint32_t t_wsCleanup      = 0;
static uint32_t t_detailsUntilMs = 0;

// ─────────────────────────────────────────────────────────────
//  Hardware Objects
// ─────────────────────────────────────────────────────────────
UltraSonicDistanceSensor hcsr04(PIN_TRIG, PIN_ECHO);

// ─────────────────────────────────────────────────────────────
//  NTP Time Client
// ─────────────────────────────────────────────────────────────
WiFiUDP   ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 19800, 3600000); // IST UTC+5:30
static bool ntpSynced = false;

// ─────────────────────────────────────────────────────────────
//  Schedule Mode State
//   V7 → next watering epoch (Unix seconds, stored on Blynk server)
//   V8 → schedule interval in hours (user sets this)
// ─────────────────────────────────────────────────────────────
static uint32_t g_scheduleIntervalHrs = 0;    // 0 = disabled
static uint32_t g_nextWaterEpoch      = 0;    // Unix timestamp of next water
static bool     g_scheduleEnabled     = false;
static uint32_t t_scheduleCheck       = 0;

// ═════════════════════════════════════════════════════════════
//  BUZZER — Non-blocking state machine
// ═════════════════════════════════════════════════════════════
static bool     buzzer_active = false;
static uint8_t  buzzer_repeat = 0;
static uint32_t buzzer_t0     = 0;
static bool     buzzer_phase  = false;
static uint32_t buzzer_lastAlarmMs = 0;

void buzzer_alarm(uint8_t repeats) {
    uint32_t now = millis();
    if (buzzer_active) return;
    if ((now - buzzer_lastAlarmMs) < BUZZ_ALARM_COOLDOWN_MS) return;
    buzzer_active = true;
    buzzer_repeat = repeats * 2;
    buzzer_phase  = true;
    buzzer_t0     = now;
    buzzer_lastAlarmMs = now;
    digitalWrite(PIN_BUZZER, HIGH);
}

void buzzer_service() {
    if (!buzzer_active) return;
    if ((millis() - buzzer_t0) >= BUZZ_SHORT_MS) {
        buzzer_phase = !buzzer_phase;
        digitalWrite(PIN_BUZZER, buzzer_phase ? HIGH : LOW);
        buzzer_t0 = millis();
        buzzer_repeat--;
        if (buzzer_repeat == 0) {
            digitalWrite(PIN_BUZZER, LOW);
            buzzer_active = false;
        }
    }
}

static void ui_beep_once(uint16_t onMs = 45) {
#if ENABLE_UI_BEEPS
    digitalWrite(PIN_BUZZER, HIGH);
    delay(onMs);
    digitalWrite(PIN_BUZZER, LOW);
#else
    (void)onMs;
#endif
}

static void ui_beep_twice(uint16_t onMs = 45) {
#if ENABLE_UI_BEEPS
    digitalWrite(PIN_BUZZER, HIGH);
    delay(onMs);
    digitalWrite(PIN_BUZZER, LOW);
    delay(50);
    digitalWrite(PIN_BUZZER, HIGH);
    delay(onMs);
    digitalWrite(PIN_BUZZER, LOW);
#else
    (void)onMs;
#endif
}

// ═════════════════════════════════════════════════════════════
//  PUMP CONTROL — LEDC PWM soft-start
// ═════════════════════════════════════════════════════════════

void pump_start(bool force = false) {
    if (pump.running) return;
    if (sensors.tankEmpty && !force) {
        Serial.println("[PUMP] Blocked — tank empty");
        return;
    }

    pump.forceOverride = force;

    Serial.println("[PUMP] Starting (Full Power)");
    pump.running   = true;
    pump.startedAt = millis();
    ui_beep_once(45);

    digitalWrite(PIN_PUMP_PWM, HIGH);
    pump.currentDuty = 255;
    pump.targetDuty = 255;
}

void pump_stop() {
    if (!pump.running) return;
    digitalWrite(PIN_PUMP_PWM, LOW);
    pump.currentDuty    = 0;
    pump.running        = false;
    pump.manualOverride = false;
    pump.forceOverride  = false;
    pump.lastStoppedAt  = millis();
    g_buttonPumpActive  = false;
    g_buttonPumpStopMs  = 0;
    Serial.println("[PUMP] Stopped");
    ui_beep_twice(45);
}

void tryBuzzTankEmpty() {
    static uint32_t lastTankAlarm = 0;
    if (millis() - lastTankAlarm > BUZZ_ALARM_COOLDOWN_MS || lastTankAlarm == 0) {
        buzzer_alarm(BUZZ_TANK_REPEAT);
        lastTankAlarm = millis();
    }
}

void pump_emergencyStop(const char* reason) {
    pump_stop();
    Serial.printf("[SAFETY] Emergency stop: %s\n", reason);
    tryBuzzTankEmpty();

    // Notify Blynk
    Blynk.logEvent("tank_empty",
        String("Tank Empty! Pump halted. ") + reason);
    Blynk.virtualWrite(V2, 0);

    // Notify all WebSocket clients
    ws_sendAlert("tank_empty", reason);
}

// ═════════════════════════════════════════════════════════════
//  BUTTON STATE
// ═════════════════════════════════════════════════════════════
// Handled directly via polling in loop() for better EMI immunity

// ═════════════════════════════════════════════════════════════
//  SENSOR READING
// ═════════════════════════════════════════════════════════════

void readMoistureSensor() {
    const uint8_t sampleCount = 9;
    int32_t samples[sampleCount];
    for (uint8_t i = 0; i < sampleCount; i++) {
        samples[i] = analogRead(PIN_MOISTURE);
        delay(2);
    }
    for (uint8_t i = 0; i < sampleCount - 1; i++) {
        for (uint8_t j = i + 1; j < sampleCount; j++) {
            if (samples[j] < samples[i]) {
                int32_t t = samples[i];
                samples[i] = samples[j];
                samples[j] = t;
            }
        }
    }
    int32_t raw = samples[sampleCount / 2];
    if (raw <= MOISTURE_RAW_MIN_VALID || raw >= MOISTURE_RAW_MAX_VALID) {
        sensors.moistureValid = false;
        sensors.moisturePct = 0;
        Serial.printf("[MOISTURE] Invalid raw=%d (sensor disconnected or wiring fault)\n", (int)raw);
        return;
    }

    int32_t pct = map(raw, MOISTURE_AIR_VALUE, MOISTURE_WATER_VALUE, 0, 100);
    sensors.moistureValid = true;
    sensors.moisturePct = (uint8_t)constrain(pct, 0, 100);
    Serial.printf("[MOISTURE] Raw: %d -> %d%%\n", (int)raw, sensors.moisturePct);
}

void readUltrasonicSensor() {
    if (g_bypassUltrasonic) {
        sensors.ultrasonicValid = true;
        sensors.distanceCm = TANK_FULL_DISTANCE_CM;
        sensors.waterLevelPct = 100;
        if (sensors.tankEmpty) {
            sensors.tankEmpty = false;
            display_setView(currentView, true); // Force redraw to clear overlay
        }
        return;
    }

    // Measure distance in cm
    float dist = hcsr04.measureDistanceCm();

    if (dist < 0.0f || dist > ULTRASONIC_MAX_CM) {
        sensors.ultrasonicValid = false;
        Serial.printf("[ULTRASONIC] Error: invalid distance %f cm\n", dist);
        return;
    }

    sensors.ultrasonicValid = true;
    sensors.distanceCm = dist;

    // Calculate water percentage:
    // Full = TANK_FULL_DISTANCE_CM (0.5 cm)
    // Empty = 6.5 cm (so total range is 6.0 cm)
    float maxRange = 6.5f - TANK_FULL_DISTANCE_CM; // 6.5 - 0.5 = 6.0 cm
    float pct = (6.5f - dist) * 100.0f / maxRange;
    sensors.waterLevelPct = (uint8_t)constrain(pct, 0, 100);

    // Tank is empty if distance is above TANK_EMPTY_THRESHOLD_CM (6.0 cm)
    bool emptyNow = (dist >= TANK_EMPTY_THRESHOLD_CM);

    if (emptyNow && !sensors.tankEmpty) {
        sensors.tankEmpty = true;
        Serial.printf("[ULTRASONIC] Tank EMPTY! Dist: %.1f cm, Level: %d%%\n", dist, sensors.waterLevelPct);
    } else if (!emptyNow && sensors.tankEmpty) {
        sensors.tankEmpty = false;
        Serial.printf("[ULTRASONIC] Tank OK. Dist: %.1f cm, Level: %d%%\n", dist, sensors.waterLevelPct);
        display_setView(currentView, true); // Clear overlay
    }

    Serial.printf("[ULTRASONIC] Dist: %.1f cm -> %d%%\n", dist, sensors.waterLevelPct);
}

// ═════════════════════════════════════════════════════════════
//  IRRIGATION LOGIC
// ═════════════════════════════════════════════════════════════

void evaluateIrrigationLogic() {
    uint32_t now = millis();

    if (g_buttonPumpActive) {
        if (now >= g_buttonPumpStopMs) {
            Serial.println("[BUTTON-PUMP] 10s duration completed — stopping");
            pump_stop();
            Blynk.virtualWrite(V2, 0);
        }
        return; // Bypass all other logic (safety & auto limits) during this force run
    }

    if (sensors.tankEmpty) {
        if (pump.running) pump_emergencyStop("Tank empty guard");
        return;
    }

    if (pump.running && (now - pump.startedAt) > PUMP_MAX_DURATION_MS) {
        Serial.println("[PUMP] Max duration — stopping");
        pump_stop();
        return;
    }

    // Only auto-irrigate in "auto" mode
    if (strcmp(g_operatingMode, "auto") != 0) return;

    if (!sensors.moistureValid) {
        if (pump.running && !pump.manualOverride) {
            Serial.println("[AUTO] Moisture reading invalid — keeping pump under timeout guard only");
        } else {
            Serial.println("[AUTO] Moisture reading invalid — auto irrigation blocked");
        }
        return;
    }

    bool cooledDown = (pump.lastStoppedAt == 0) || ((now - pump.lastStoppedAt) > PUMP_MIN_INTERVAL_MS);

    if (!pump.running && !pump.manualOverride) {
        if (sensors.moisturePct < g_moistureThreshold && cooledDown) {
            Serial.printf("[AUTO] Moisture %d%% < %d%% — pump start\n",
                          sensors.moisturePct, g_moistureThreshold);
            pump_start();
            Blynk.virtualWrite(V2, 1023);
        }
    }

    if (pump.running) {
        if (sensors.moisturePct >= 60) {
            Serial.printf("[PUMP] 60%% moisture reached — stopping pump (current: %d%%)\n", sensors.moisturePct);
            pump_stop();
            Blynk.virtualWrite(V2, 0);
        }
    }
}

// ═════════════════════════════════════════════════════════════
//  WEBSOCKET COMMAND CALLBACKS (from mobile app)
// ═════════════════════════════════════════════════════════════

void onWs_PumpStart() {
    if (sensors.tankEmpty) {
        ws_sendAlert("blocked", "Pump blocked — tank empty!");
        return;
    }
    pump.manualOverride = true;
    pump_start();
    Blynk.virtualWrite(V2, 1023);
}

void onWs_PumpStop() {
    pump.manualOverride = false;
    pump_stop();
    Blynk.virtualWrite(V2, 0);
}

void onWs_ForcePumpStart() {
    pump.manualOverride = true;
    pump_start(true); // force
    Blynk.virtualWrite(V2, 1023);
}

void onWs_EmergencyStop() {
    pump_emergencyStop("Emergency stop from mobile app");
    display_overlay("E-STOP!", COLOR_RED);
}

void onWs_SetMode(const char* mode) {
    strncpy(g_operatingMode, mode, sizeof(g_operatingMode) - 1);
    g_operatingMode[sizeof(g_operatingMode) - 1] = '\0';
    Serial.printf("[WS] Mode set: %s\n", g_operatingMode);

    // If switching to manual, stop auto-pump
    if (strcmp(mode, "manual") == 0 && pump.running && !pump.manualOverride) {
        pump_stop();
    }
}

void onWs_SetThreshold(uint8_t value) {
    g_moistureThreshold = value;
    Blynk.virtualWrite(V4, value);  // Sync Blynk slider
    Serial.printf("[WS] Threshold: %d%%\n", value);
}

void onWs_SetPwm(uint8_t value) {
    g_pumpPwmRaw = value;
    g_pumpDutyCyclePct = (uint8_t)map(value, 0, 255, 0, 100);
    Serial.printf("[WS] PWM duty: %d/255 (%d%%)\n", value, g_pumpDutyCyclePct);

    // Update live if pump running
    if (pump.running) {
        // Digital only now
    }
}

void onWs_SetScheduleHrs(uint32_t hrs) {
    g_scheduleIntervalHrs = hrs;
    g_scheduleEnabled = (hrs > 0);
    if (g_scheduleEnabled && g_nextWaterEpoch == 0 && ntpSynced) {
        // First time setup: schedule the FIRST watering
        g_nextWaterEpoch = (uint32_t)timeClient.getEpochTime() + (hrs * 3600UL);
    } else if (!g_scheduleEnabled) {
        g_nextWaterEpoch = 0;
    }
    Blynk.virtualWrite(V8, hrs);
    Blynk.virtualWrite(V7, (int32_t)g_nextWaterEpoch);
    Serial.printf("[WS] Schedule set: every %uh\n", hrs);
}

// ═════════════════════════════════════════════════════════════
//  BLYNK VIRTUAL PIN HANDLERS
// ═════════════════════════════════════════════════════════════

BLYNK_WRITE(V3) {
    int val = param.asInt();
    if (val == 1) {
        if (sensors.tankEmpty) {
            Blynk.logEvent("tank_empty", "Manual pump blocked: tank empty!");
            return;
        }
        pump.manualOverride = true;
        pump_start();
        Blynk.virtualWrite(V2, 1023);
    } else {
        pump.manualOverride = false;
        pump_stop();
        Blynk.virtualWrite(V2, 0);
    }
}

// V7 — Next watering Unix epoch (Blynk server saves this across reboots)
BLYNK_WRITE(V7) {
    int32_t epoch = param.asInt();
    if (epoch > 0) {
        g_nextWaterEpoch = (uint32_t)epoch;
        Serial.printf("[BLYNK] Schedule: next epoch restored = %d\n", epoch);
    }
}

// V8 — Schedule interval in hours (0 = disabled)
BLYNK_WRITE(V8) {
    uint32_t hrs = (uint32_t)constrain(param.asInt(), 0, 720); // max 30 days
    g_scheduleIntervalHrs = hrs;
    g_scheduleEnabled = (hrs > 0);
    if (g_scheduleEnabled && g_nextWaterEpoch == 0 && ntpSynced) {
        // First time setup: schedule the FIRST watering
        g_nextWaterEpoch = (uint32_t)timeClient.getEpochTime() + (hrs * 3600UL);
        Blynk.virtualWrite(V7, (int32_t)g_nextWaterEpoch);
        Serial.printf("[BLYNK] Schedule set: every %uh, first watering in %uh\n", hrs, hrs);
    } else if (!g_scheduleEnabled) {
        g_nextWaterEpoch = 0;
        Blynk.virtualWrite(V7, 0);
        Serial.println("[BLYNK] Schedule disabled");
    }
}

BLYNK_WRITE(V4) {
    g_moistureThreshold = (uint8_t)constrain(param.asInt(), 0, 100);
    Serial.printf("[BLYNK] Threshold: %d%%\n", g_moistureThreshold);
}

BLYNK_WRITE(V5) {
    g_pumpDutyCyclePct = (uint8_t)constrain(param.asInt(), 0, 100);
    g_pumpPwmRaw = (uint8_t)map(g_pumpDutyCyclePct, 0, 100, 0, 255);
    if (pump.running) {
        ledcWrite(PIN_PUMP_PWM, g_pumpPwmRaw);
        pump.currentDuty = g_pumpPwmRaw;
    }
}

BLYNK_CONNECTED() {
    Blynk.syncVirtual(V4); // moisture threshold
    Blynk.syncVirtual(V5); // pwm (legacy)
    Blynk.syncVirtual(V7); // restore next watering epoch from server
    Blynk.syncVirtual(V8); // restore schedule interval from server
}

void pushBlynkData() {
    if (!Blynk.connected()) return;
    Blynk.virtualWrite(V0, sensors.moisturePct);
    Blynk.virtualWrite(V1, sensors.waterLevelPct);
    Blynk.virtualWrite(V2, pump.running ? 1023 : 0);
    
    // Build schedule countdown string for V6 log
    char schedBuf[32] = "Schedule: OFF";
    if (g_scheduleEnabled && ntpSynced && g_nextWaterEpoch > 0) {
        uint32_t nowEpoch = (uint32_t)timeClient.getEpochTime();
        if (nowEpoch < g_nextWaterEpoch) {
            uint32_t secs = g_nextWaterEpoch - nowEpoch;
            snprintf(schedBuf, sizeof(schedBuf), "Sched: %02uh%02um",
                     secs / 3600, (secs % 3600) / 60);
        } else {
            strncpy(schedBuf, "Schedule: DUE!", sizeof(schedBuf));
        }
    }
    Blynk.virtualWrite(V6, String("M:") + sensors.moisturePct +
                            "% W:" + sensors.waterLevelPct +
                            "% P:" + (pump.running ? "ON" : "OFF") +
                            " | " + schedBuf);
}

// ═════════════════════════════════════════════════════════════
//  CONNECTIVITY HELPER
// ═════════════════════════════════════════════════════════════

void ensureConnectivity() {
    static uint32_t lastReconnect = 0;
    if (WiFi.status() != WL_CONNECTED) {
        uint32_t now = millis();
        if ((now - lastReconnect) > 30000) {
            Serial.println("[WiFi] Reconnecting...");
            WiFi.reconnect();
            lastReconnect = now;
        }
    }
    sensors.rssi = WiFi.RSSI();
}

// ═════════════════════════════════════════════════════════════
//  SETUP
// ═════════════════════════════════════════════════════════════

void setup() {
    Serial.begin(115200);
    Serial.println("\n==============================");
    Serial.printf("  Nexus Pro  FW v%s\n", FW_VERSION);
    Serial.println("  Nexus Pro Smart Planter");
    Serial.println("==============================");

    // GPIO
    pinMode(PIN_BUZZER,   OUTPUT);
#if ENABLE_VIEW_BUTTON
    pinMode(PIN_BUTTON,   INPUT_PULLUP);
#endif
    pinMode(PIN_FLOAT_SW, INPUT_PULLUP);
    
    digitalWrite(PIN_BUZZER, LOW);
    ui_beep_once(60);
    
    // Onboard Blue LED diagnostics (GPIO 2)
    pinMode(2, OUTPUT);
    for (int i = 0; i < 10; i++) {
        digitalWrite(2, HIGH);
        delay(80);
        digitalWrite(2, LOW);
        delay(80);
    }
    
    // Pump Pin Setup (Digital Only - Full Power)
    pinMode(PIN_PUMP_PWM, OUTPUT);
    digitalWrite(PIN_PUMP_PWM, LOW);




    // ADC
    analogReadResolution(12);
    analogSetAttenuation(ADC_11db);

    // Button is now polled in loop() to avoid false triggers from screen refresh noise

    // TFT Display
    display_init();

    // Wi-Fi
    WiFi.mode(WIFI_STA);
    WiFi.setHostname(DEVICE_HOSTNAME);

    Serial.printf("[WiFi] Connecting to %s ", WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    uint8_t retry = 0;
    while (WiFi.status() != WL_CONNECTED && retry < 30) {
        delay(500);
        Serial.print(".");
        retry++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        // Connected via DHCP! Let's dynamically shift to static IP .164 on this subnet
        IPAddress dhcpIP = WiFi.localIP();
        IPAddress gateway = WiFi.gatewayIP();
        IPAddress subnet = WiFi.subnetMask();
        IPAddress dns = WiFi.dnsIP();
        
        // Define the target static IP with the same network prefix but ending in .164
        IPAddress staticIP = dhcpIP;
        staticIP[3] = 164; // Force host octet to 164
        
        Serial.printf("\n[WiFi] DHCP Assigned IP: %s\n", dhcpIP.toString().c_str());
        Serial.printf("[WiFi] Reconfiguring to Static IP: %s\n", staticIP.toString().c_str());
        
        if (!WiFi.config(staticIP, gateway, subnet, dns, IPAddress(8, 8, 8, 8))) {
            Serial.println("[WiFi] Dynamic Static IP Configuration FAILED! Falling back to DHCP.");
        } else {
            Serial.printf("[WiFi] Static IP Configured successfully at: %s\n", WiFi.localIP().toString().c_str());
        }

        if (MDNS.begin(DEVICE_HOSTNAME)) {
            Serial.printf("[mDNS] Ready: http://%s.local:81/ | ws://%s.local:81/ws\n",
                          DEVICE_HOSTNAME, DEVICE_HOSTNAME);
        } else {
            Serial.println("[mDNS] FAILED — hostname advertisement unavailable");
        }
    } else {
        Serial.println("\n[WiFi] FAILED — offline mode");
        display_overlay("WiFi Failed!", COLOR_YELLOW);
        delay(2000);
    }

    // Blynk
    if (WiFi.status() == WL_CONNECTED) {
        Blynk.config(BLYNK_AUTH_TOKEN);
        Blynk.connect(3000);
    }

    // ── WebSocket Server for Mobile App ───────────────────────
    if (WiFi.status() == WL_CONNECTED) {
        ws_registerCallbacks(
            onWs_PumpStart,
            onWs_PumpStop,
            onWs_EmergencyStop,
            onWs_SetMode,
            onWs_SetThreshold,
            onWs_SetPwm,
            onWs_ForcePumpStart,
            onWs_SetScheduleHrs
        );
        ws_init();
        Serial.printf("[WS] Mobile app endpoint: ws://%s:81/ws\n",
                      WiFi.localIP().toString().c_str());
    }

    // NTP Time Client
    if (WiFi.status() == WL_CONNECTED) {
        timeClient.begin();
        if (timeClient.forceUpdate()) {
            ntpSynced = true;
            Serial.printf("[NTP] Time synced: %s\n", timeClient.getFormattedTime().c_str());
        } else {
            Serial.println("[NTP] Sync failed — schedule will resume after sync");
        }
    }

    // Initial sensor reads
    readMoistureSensor();
    readUltrasonicSensor();

    Serial.printf("[BOOT] Setup complete — FW v%s\n", FW_VERSION);

    uint32_t now = millis();
    t_sensorPoll     = now;
    t_ultrasonicPoll = now;
    t_blynkUpdate    = now;
    t_displayRefresh = now;
    t_wsBroadcast    = now;
    t_wsCleanup      = now;
    t_scheduleCheck  = now;
}

// ═════════════════════════════════════════════════════════════
//  LOOP — Non-blocking cooperative scheduler
// ═════════════════════════════════════════════════════════════

void loop() {
    uint32_t now = millis();

    // 1. Blynk network processing
    if (WiFi.status() == WL_CONNECTED) {
        Blynk.run();
    }

    // 2. Handle mode button press with stable polling debounce and long-press pump trigger
#if ENABLE_VIEW_BUTTON
    static uint32_t lastButtonChange = 0;
    static bool buttonState = HIGH;
    static uint32_t buttonPressedMs = 0;
    static bool longPressTriggered = false;
    
    bool rawState = digitalRead(PIN_BUTTON);
    if (rawState == LOW) {
        if (buttonState == HIGH && (now - lastButtonChange) > 200) {
            lastButtonChange = now;
            buttonState = LOW;
            buttonPressedMs = now;
            longPressTriggered = false;
            
            ui_beep_once(35);
            if (currentView == 2) {
                display_setView(0);
                t_detailsUntilMs = 0;
                Serial.println("[BTN] Main view");
            } else {
                display_setView(2);
                t_detailsUntilMs = now + 4000;
                Serial.println("[BTN] Details view (4s)");
            }
        } else if (buttonState == LOW && !longPressTriggered && buttonPressedMs != 0 && (now - buttonPressedMs) > 3000) {
            longPressTriggered = true;
            Serial.println("[BTN] Long press detected (>3s) -> starting pump for 10s");
            
            // Alert user via buzzer long-beeps
            ui_beep_twice(100);
            
            // Start pump for 10 seconds
            g_buttonPumpActive = true;
            g_buttonPumpStopMs = now + 10000;
            pump.manualOverride = true;
            pump_start(true); // force = true to run whichever state it is (bypassing empty tank check)
            
            if (WiFi.status() == WL_CONNECTED && Blynk.connected()) {
                Blynk.virtualWrite(V2, 1023);
            }
        }
    } else { // rawState == HIGH
        if (buttonState == LOW && (now - lastButtonChange) > 200) {
            lastButtonChange = now;
            buttonState = HIGH;
            buttonPressedMs = 0;
            longPressTriggered = false;
        }
    }
#endif

    if (t_detailsUntilMs != 0 && now > t_detailsUntilMs) {
        display_setView(0);
        t_detailsUntilMs = 0;
    }

    // 3. Ultrasonic sensor (500ms)
    if ((now - t_ultrasonicPoll) >= ULTRASONIC_POLL_INTERVAL_MS) {
        t_ultrasonicPoll = now;
        readUltrasonicSensor();

        // Optional float switch fallback
#if ENABLE_FLOAT_SWITCH
        if (digitalRead(PIN_FLOAT_SW) == HIGH && !sensors.tankEmpty) {
            sensors.ultrasonicValid = false;
            sensors.tankEmpty = true;
            if (pump.running) pump_emergencyStop("Float switch: tank empty");
        }
#endif
    }

    // 4. Moisture & irrigation logic (2000ms)
    if ((now - t_sensorPoll) >= SENSOR_POLL_INTERVAL_MS) {
        t_sensorPoll = now;

        // --- MOTOR EMI NOISE SUPPRESSION ---
        // Briefly cut motor power so the analog rails stabilise
        // before taking moisture reading — eliminates false "100% wet" noise.
        bool motorWasPaused = false;
        if (pump.running) {
            digitalWrite(PIN_PUMP_PWM, LOW);
            delay(20); // Seamless pause
            motorWasPaused = true;
        }

        readMoistureSensor();
        evaluateIrrigationLogic();

        // Extra safety: if moisture is still under threshold and pump is
        // supposed to be running but somehow stopped, force-restart it.
        if (!pump.running && !sensors.tankEmpty &&
            sensors.moistureValid && sensors.moisturePct < g_moistureThreshold &&
            strcmp(g_operatingMode, "auto") == 0 &&
            (pump.lastStoppedAt == 0 || (now - pump.lastStoppedAt) > PUMP_MIN_INTERVAL_MS)) {
            Serial.println("[AUTO] Catch-up: moisture under threshold — force pump on");
            pump_start();
            Blynk.virtualWrite(V2, 1023);
            motorWasPaused = false; // pump_start already set pin HIGH
        }

        // Resume motor if it was paused for sampling and logic kept it running
        if (motorWasPaused && pump.running) {
            digitalWrite(PIN_PUMP_PWM, HIGH);
        }
        // -----------------------------------

        sensors.uptimeSeconds = now / 1000;
    }

    // 5. Blynk data push (5000ms)
    if ((now - t_blynkUpdate) >= BLYNK_UPDATE_INTERVAL_MS) {
        t_blynkUpdate = now;
        ensureConnectivity();
        pushBlynkData();
    }

    // 6. Display refresh (1000ms)
    if ((now - t_displayRefresh) >= DISPLAY_REFRESH_INTERVAL_MS) {
        t_displayRefresh = now;
        
        uint32_t displayTimer = sensors.uptimeSeconds;
        if (strcmp(g_operatingMode, "auto") == 0) {
            if (pump.running) {
                displayTimer = 0;
            } else if ((now - pump.lastStoppedAt) < PUMP_MIN_INTERVAL_MS) {
                displayTimer = (PUMP_MIN_INTERVAL_MS - (now - pump.lastStoppedAt)) / 1000;
            } else {
                displayTimer = 0; // Ready to pump
            }
        }

        display_update(
            sensors.moisturePct, sensors.waterLevelPct,
            pump.running, sensors.tankEmpty,
            sensors.rssi,
            WiFi.SSID().c_str(),
            WiFi.localIP().toString().c_str(),
            g_moistureThreshold,
            pump.targetDuty,
            displayTimer
        );
    }

    // 7. WebSocket broadcast to mobile app (1000ms)
    if ((now - t_wsBroadcast) >= WS_BROADCAST_INTERVAL_MS) {
        t_wsBroadcast = now;
        ws_broadcastSensorData(
            sensors.moisturePct, sensors.waterLevelPct,
            sensors.rssi,
            pump.running, sensors.tankEmpty,
            g_operatingMode,
            g_moistureThreshold,
            pump.currentDuty,
            g_nextWaterEpoch,
            g_scheduleIntervalHrs
        );
    }

    // 8. WebSocket client cleanup (10s)
    if ((now - t_wsCleanup) >= 10000) {
        t_wsCleanup = now;
        ws_cleanup();
    }

    // 9. Schedule check (every 30s)
    if (g_scheduleEnabled && ntpSynced && (now - t_scheduleCheck) >= 30000) {
        t_scheduleCheck = now;
        timeClient.update();
        uint32_t currentEpoch = (uint32_t)timeClient.getEpochTime();
        if (currentEpoch >= g_nextWaterEpoch && !pump.running && !sensors.tankEmpty) {
            Serial.printf("[SCHEDULE] Time reached! Watering now. Next in %uh\n", g_scheduleIntervalHrs);
            pump.manualOverride = false; // use auto-stop via moisture threshold
            pump_start();
            Blynk.virtualWrite(V2, 1023);
            // Schedule next watering
            g_nextWaterEpoch = currentEpoch + (g_scheduleIntervalHrs * 3600UL);
            Blynk.virtualWrite(V7, (int32_t)g_nextWaterEpoch);
        }
    }

    // 9. Buzzer state machine
    buzzer_service();

    // 10. Pump safety watchdog (500ms)
    if ((now - t_pumpWatchdog) >= 500) {
        t_pumpWatchdog = now;
        uint32_t currentMs = millis();
        if (pump.running && currentMs > pump.startedAt && (currentMs - pump.startedAt) > PUMP_MAX_DURATION_MS) {
            pump_stop();
            Blynk.logEvent("pump_timeout", "Pump auto-stopped (timeout).");
            ws_sendAlert("pump_timeout", "Pump stopped: max duration reached");
            Blynk.virtualWrite(V2, 0);
        }
        if (pump.running && sensors.tankEmpty && !pump.forceOverride) {
            pump_emergencyStop("Watchdog: tank empty");
        }
    }

    yield();
}
