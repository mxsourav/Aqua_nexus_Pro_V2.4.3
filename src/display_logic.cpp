// =============================================================
//  display_logic.cpp — AquaNexus Pro Smart Planter
//  TFT Dashboard Rendering Logic
//  Driver : ST7735 via TFT_eSPI (configured by platformio.ini)
//  Display: 1.8" 160×128 in Landscape mode
// =============================================================

#include "display_logic.h"
#include <SPI.h>

// ─────────────────────────────────────────────────────────────
//  Module-Private TFT Instance
// ─────────────────────────────────────────────────────────────
static TFT_eSPI tft = TFT_eSPI();

// Active view (0=Stats, 1=WiFi Info, 2=System)
uint8_t currentView = 0;
static bool view0FrameDrawn = false;
static uint8_t lastWaterPct = 255;
static uint8_t lastMoisturePct = 255;
static uint32_t lastUptimeS = UINT32_MAX;

// ─────────────────────────────────────────────────────────────
//  Helper — draw a rounded filled rectangle progress bar
//  x, y : top-left corner
//  w, h : full bar dimensions
//  pct  : fill percentage (0–100)
//  fg   : bar fill colour
//  bg   : empty bar track colour
// ─────────────────────────────────────────────────────────────
static void drawProgressBar(int16_t x, int16_t y, int16_t w, int16_t h,
                             uint8_t pct, uint16_t fg, uint16_t bg)
{
    // Draw empty track
    tft.fillRoundRect(x, y, w, h, 3, bg);
    // Draw filled portion
    int16_t fillW = (int16_t)((pct / 100.0f) * (float)(w - 2));
    if (fillW > 0) {
        tft.fillRoundRect(x + 1, y + 1, fillW, h - 2, 2, fg);
    }
    // Draw border
    tft.drawRoundRect(x, y, w, h, 3, COLOR_GREY);
}

// ─────────────────────────────────────────────────────────────
//  Helper — pick bar colour based on percentage and inversion
//  invert = true means LOW is bad (moisture), false = LOW is good (tank)
// ─────────────────────────────────────────────────────────────
static uint16_t levelColor(uint8_t pct, bool lowIsGood)
{
    if (lowIsGood) {
        // low value is good (e.g., water distance)
        if (pct > 60) return COLOR_GREEN;
        if (pct > 30) return COLOR_YELLOW;
        return COLOR_RED;
    } else {
        // high value is good (moisture)
        if (pct >= 60) return COLOR_GREEN;
        if (pct >= 30) return COLOR_YELLOW;
        return COLOR_RED;
    }
}

// ─────────────────────────────────────────────────────────────
//  Helper — draw common header bar
// ─────────────────────────────────────────────────────────────
static void drawHeader(const char* title, bool pumpOn, bool tankEmpty)
{
    // Header background
    tft.fillRect(0, 0, TFT_LANDSCAPE_W, 18, COLOR_HEADER_BG);

    // Project name
    tft.setTextColor(COLOR_ACCENT, COLOR_HEADER_BG);
    tft.setTextSize(1);
    tft.setCursor(4, 5);
    tft.print(PROJECT_NAME);

    // Page title (shifted right to avoid overlap with "Nexus Pro")
    tft.setTextColor(COLOR_WHITE, COLOR_HEADER_BG);
    int16_t tx = (TFT_LANDSCAPE_W - (int16_t)(strlen(title) * 6)) / 2;
    if (tx < 62) tx = 62; // Minimum safety margin for "Nexus Pro"
    tft.setCursor(tx, 5);
    tft.print(title);

    // Status indicator top-right (pump or alarm)
    if (tankEmpty) {
        tft.setTextColor(COLOR_RED, COLOR_HEADER_BG);
        tft.setCursor(TFT_LANDSCAPE_W - 36, 5);
        tft.print("ALARM");
    } else if (pumpOn) {
        tft.setTextColor(COLOR_GREEN, COLOR_HEADER_BG);
        tft.setCursor(TFT_LANDSCAPE_W - 30, 5);
        tft.print("PUMP");
    } else {
        tft.setTextColor(COLOR_DARKGREY, COLOR_HEADER_BG);
        tft.setCursor(TFT_LANDSCAPE_W - 18, 5);
        tft.print("OK");
    }

    // Divider line
    tft.drawFastHLine(0, 18, TFT_LANDSCAPE_W, COLOR_ACCENT);
}

// ─────────────────────────────────────────────────────────────
//  Helper — draw footer status bar
// ─────────────────────────────────────────────────────────────
static void drawFooter(uint32_t uptime_s)
{
    tft.drawFastHLine(0, TFT_LANDSCAPE_H - 14, TFT_LANDSCAPE_W, COLOR_DARKGREY);
    tft.fillRect(0, TFT_LANDSCAPE_H - 13, TFT_LANDSCAPE_W, 13, COLOR_BG);

    // Uptime
    uint32_t mins = uptime_s / 60;
    uint32_t hrs  = mins / 60;
    mins = mins % 60;

    char buf[32];
    snprintf(buf, sizeof(buf), "Up: %02luh%02lum", hrs, mins);
    tft.setTextColor(COLOR_DARKGREY, COLOR_BG);
    tft.setTextSize(1);
    tft.setCursor(4, TFT_LANDSCAPE_H - 11);
    tft.print(buf);

    // FW version right-aligned
    snprintf(buf, sizeof(buf), "v%s", FW_VERSION);
    int16_t vx = TFT_LANDSCAPE_W - (int16_t)(strlen(buf) * 6) - 4;
    tft.setCursor(vx, TFT_LANDSCAPE_H - 11);
    tft.print(buf);
}

static void drawBars(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t pct, uint16_t activeColor, uint16_t inactiveColor) {
    int num_bars = 5;
    int gap = 3;
    int bar_w = (w - (num_bars - 1) * gap) / num_bars;
    int active_bars = (pct * num_bars) / 100;
    if (pct > 0 && active_bars == 0) active_bars = 1;

    for (int i = 0; i < num_bars; i++) {
        int bar_h = h * (i + 2) / (num_bars + 1);
        int bar_y = y + (h - bar_h);
        int bar_x = x + i * (bar_w + gap);
        
        uint16_t c = (i < active_bars) ? activeColor : inactiveColor;
        tft.fillRect(bar_x, bar_y, bar_w, bar_h, c);
    }
}

// Draw a hardware-style dial (arc)
static void drawHardwareDial(int16_t cx, int16_t cy, uint8_t val, uint16_t color, const char* label) {
    tft.drawArc(cx, cy, 28, 22, 225, 495, 0x18E3, COLOR_BG, false); // Scale background
    int16_t angle = (int16_t)map(val, 0, 100, 225, 495);
    tft.drawArc(cx, cy, 28, 22, 225, angle, color, COLOR_BG, false); // Active arc
    
    tft.setTextSize(1);
    tft.setTextColor(COLOR_WHITE, COLOR_BG);
    char buf[8];
    snprintf(buf, sizeof(buf), "%u%%", val);
    int16_t vx = cx - (strlen(buf) * 3);
    tft.setCursor(vx, cy - 4);
    tft.print(buf);
    
    tft.setTextColor(COLOR_GREY, COLOR_BG);
    int16_t lx = cx - (strlen(label) * 3);
    tft.setCursor(lx, cy + 32);
    tft.print(label);
}

// Draw a realistic industrial WiFi icon — 4 segments with varying thicknesses
static void drawWifiIcon(int16_t cx, int16_t cy, uint16_t color) {
    uint16_t bg = COLOR_BG;
    // Solid core dot
    tft.fillCircle(cx, cy + 6, 3, color);
    // Segment 1 (Small)
    tft.drawArc(cx, cy + 6, 9, 6, 225, 315, color, bg, false);
    // Segment 2 (Medium)
    tft.drawArc(cx, cy + 6, 15, 12, 220, 320, color, bg, false);
    // Segment 3 (Large/Outer)
    tft.drawArc(cx, cy + 6, 21, 18, 215, 325, color, bg, false);
}

// =============================================================
//  View 0: Main Stats Dashboard (Split Screen UI)
// =============================================================
static void drawView0_Stats(uint8_t moisture, uint8_t waterLevel,
                             bool pumpOn, bool tankEmpty,
                             uint8_t threshold, uint32_t uptime_s)
{
    if (!view0FrameDrawn) {
        tft.fillRect(0, 0, TFT_LANDSCAPE_W, TFT_LANDSCAPE_H, COLOR_BG);
        tft.drawFastVLine(79, 5, 95, 0x1CE7);
        tft.drawFastHLine(5, 105, 150, 0x1CE7);

        tft.setTextColor(COLOR_WATER_BLUE, COLOR_BG);
        tft.setTextSize(1);
        tft.setCursor(22, 10);
        tft.print("WATER");

        tft.setTextColor(COLOR_GREEN, COLOR_BG);
        tft.setCursor(95, 10);
        tft.print("MOISTURE");

        drawWifiIcon(20, 114, COLOR_WATER_BLUE);
        view0FrameDrawn = true;
        lastWaterPct = 255;
        lastMoisturePct = 255;
        lastUptimeS = UINT32_MAX;
    }

    if (waterLevel != lastWaterPct) {
        tft.fillRect(6, 28, 70, 68, COLOR_BG);
        char val[8];
        snprintf(val, sizeof(val), "%d%%", waterLevel);
        tft.setTextColor(COLOR_WATER_BLUE, COLOR_BG);
        tft.setTextSize(3);
        int16_t wx = 40 - (strlen(val) * 9);
        tft.setCursor(wx > 5 ? wx : 5, 30);
        tft.print(val);
        drawBars(10, 70, 60, 25, waterLevel, COLOR_WATER_BLUE, 0x0010);
        lastWaterPct = waterLevel;
    }

    if (moisture != lastMoisturePct) {
        tft.fillRect(84, 28, 70, 68, COLOR_BG);
        char val[8];
        snprintf(val, sizeof(val), "%d%%", moisture);
        tft.setTextColor(COLOR_GREEN, COLOR_BG);
        tft.setTextSize(3);
        int16_t mx = 120 - (strlen(val) * 9);
        tft.setCursor(mx > 85 ? mx : 85, 30);
        tft.print(val);
        drawBars(90, 70, 60, 25, moisture, COLOR_GREEN, 0x0320);
        lastMoisturePct = moisture;
    }

    if (uptime_s != lastUptimeS) {
        tft.fillRect(60, 108, 95, 18, COLOR_BG);
        uint32_t s = uptime_s % 60;
        uint32_t m = (uptime_s / 60) % 60;
        uint32_t h = (uptime_s / 3600);
        char timeStr[16];
        snprintf(timeStr, sizeof(timeStr), "%02lu:%02lu:%02lu", h, m, s);
        tft.setTextColor(COLOR_WATER_BLUE, COLOR_BG);
        tft.setTextSize(2);
        tft.setCursor(65, 110);
        tft.print(timeStr);
        lastUptimeS = uptime_s;
    }
}

// =============================================================
//  View 1: Wi-Fi & IoT Info
// =============================================================
static void drawView1_WiFi(int32_t rssi, const char* ssid,
                            const char* localIP, bool pumpOn,
                            bool tankEmpty, uint32_t uptime_s)
{
    tft.fillRect(0, 19, TFT_LANDSCAPE_W, TFT_LANDSCAPE_H - 33, COLOR_BG);

    tft.setTextSize(1);

    // SSID
    tft.setTextColor(COLOR_GREY, COLOR_BG);
    tft.setCursor(4, 24);
    tft.print("SSID:");
    tft.setTextColor(COLOR_ACCENT, COLOR_BG);
    tft.setCursor(40, 24);
    // Truncate long SSIDs
    char ssidBuf[22];
    strncpy(ssidBuf, ssid, 21);
    ssidBuf[21] = '\0';
    tft.print(ssidBuf);

    // IP
    tft.setTextColor(COLOR_GREY, COLOR_BG);
    tft.setCursor(4, 37);
    tft.print("IP:");
    tft.setTextColor(COLOR_WHITE, COLOR_BG);
    tft.setCursor(40, 37);
    tft.print(localIP);

    // RSSI bar
    tft.setTextColor(COLOR_GREY, COLOR_BG);
    tft.setCursor(4, 50);
    tft.print("Signal:");

    // Convert RSSI (-100 to -30 dBm) to 0–100%
    int32_t clampedRSSI = constrain(rssi, -100, -30);
    uint8_t rssiPct = (uint8_t)map(clampedRSSI, -100, -30, 0, 100);
    uint16_t rssiColor = (rssiPct > 60) ? COLOR_GREEN :
                         (rssiPct > 30) ? COLOR_YELLOW : COLOR_RED;

    char rssiStr[12];
    snprintf(rssiStr, sizeof(rssiStr), "%d dBm", (int)rssi);
    tft.setTextColor(rssiColor, COLOR_BG);
    tft.setCursor(120, 50);
    tft.print(rssiStr);

    drawProgressBar(4, 62, 152, 10, rssiPct, rssiColor, COLOR_DARKGREY);

    // IoT Platform
    tft.setTextColor(COLOR_GREY, COLOR_BG);
    tft.setCursor(4, 77);
    tft.print("Platform: Blynk IoT");

    tft.setCursor(4, 89);
    tft.print("Cloud: ");
    tft.setTextColor(COLOR_GREEN, COLOR_BG);
    tft.print("Connected");

    // View hint
    tft.setTextColor(COLOR_DARKGREY, COLOR_BG);
    tft.setCursor(4, 102);
    tft.print("BTN: next view");

    drawFooter(uptime_s);
}

// =============================================================
//  View 2: Hardware Control Dials (Side-by-Side)
// =============================================================
static void drawView2_System(uint8_t threshold, uint8_t pwmDuty, uint32_t uptime_s)
{
    tft.fillRect(0, 19, TFT_LANDSCAPE_W, TFT_LANDSCAPE_H - 33, COLOR_BG);
    
    // Draw Threshold Dial (Left)
    drawHardwareDial(40, 60, threshold, COLOR_YELLOW, "THRESH");
    
    // Draw PWM Dial (Right)
    uint8_t pwmPct = (uint8_t)map(pwmDuty, 0, 255, 0, 100);
    drawHardwareDial(120, 60, pwmPct, COLOR_ACCENT, "INTENS");

    drawFooter(uptime_s);
}

// =============================================================
//  View: Pumping Water (Overrides normal views when pump is running)
// =============================================================
static void drawView_Pumping(uint8_t moisture, uint32_t uptime_s)
{
    static uint8_t lastPumpingMoisture = 255;
    
    // Draw the whole screen once
    if (currentView != 255) { // 255 is our special "pumping" state tracker
        tft.fillRect(0, 19, TFT_LANDSCAPE_W, TFT_LANDSCAPE_H - 19, COLOR_BG);
        tft.setTextColor(COLOR_WATER_BLUE, COLOR_BG);
        tft.setTextSize(2);
        tft.setCursor(15, 30);
        tft.print("PUMPING...");

        tft.setTextColor(COLOR_GREY, COLOR_BG);
        tft.setTextSize(1);
        tft.setCursor(30, 55);
        tft.print("Live Moisture:");
        
        lastPumpingMoisture = 255;
    }

    if (moisture != lastPumpingMoisture || currentView != 255) {
        char val[8];
        snprintf(val, sizeof(val), "%d%%", moisture);
        tft.fillRect(40, 70, 80, 30, COLOR_BG);
        tft.setTextColor(COLOR_GREEN, COLOR_BG);
        tft.setTextSize(3);
        int16_t mx = 80 - (strlen(val) * 9);
        tft.setCursor(mx > 0 ? mx : 10, 75);
        tft.print(val);
        lastPumpingMoisture = moisture;
    }
    
    currentView = 255; // Set state to pumping
    drawFooter(uptime_s);
}

// =============================================================
//  Public API Implementation
// =============================================================

void display_init()
{
    pinMode(PIN_TFT_CS, OUTPUT);
    digitalWrite(PIN_TFT_CS, HIGH);

    pinMode(PIN_TFT_DC, OUTPUT);
    digitalWrite(PIN_TFT_DC, HIGH);

    pinMode(PIN_TFT_RST, OUTPUT);
    digitalWrite(PIN_TFT_RST, HIGH);

    pinMode(PIN_TFT_BL, OUTPUT);
    digitalWrite(PIN_TFT_BL, LOW);

    // Force the panel through a clean hardware reset before TFT_eSPI starts.
    digitalWrite(PIN_TFT_RST, LOW);
    delay(20);
    digitalWrite(PIN_TFT_RST, HIGH);
    delay(150);

    // Lock VSPI to the exact wiring used on the ESP32 DevKit.
    SPI.begin(PIN_TFT_SCK, -1, PIN_TFT_MOSI, PIN_TFT_CS);

    tft.init();
    tft.setRotation(1);                    // Landscape
    tft.fillScreen(TFT_BLACK);
    tft.invertDisplay(false);
    digitalWrite(PIN_TFT_BL, HIGH);
    tft.fillScreen(COLOR_BG);

    Serial.printf("[TFT] init ok | rotation=%d | size=%dx%d | CS=%d DC=%d RST=%d BL=%d MOSI=%d SCK=%d\n",
                  1, tft.width(), tft.height(),
                  PIN_TFT_CS, PIN_TFT_DC, PIN_TFT_RST, PIN_TFT_BL,
                  PIN_TFT_MOSI, PIN_TFT_SCK);

    display_splash();
}

void display_splash()
{
    tft.fillScreen(TFT_BLACK);
    const char* app = "Aqua N Pro";
    const char* line2 = "Smart Planter";
    char verBuf[24];
    snprintf(verBuf, sizeof(verBuf), "FW v%s", FW_VERSION);

    tft.setTextColor(COLOR_ACCENT, TFT_BLACK);
    tft.setTextSize(2);
    int16_t x1 = (TFT_LANDSCAPE_W - (int16_t)(strlen(app) * 12)) / 2;
    tft.setCursor(x1 > 0 ? x1 : 0, 36);
    tft.print(app);

    tft.setTextColor(COLOR_WHITE, TFT_BLACK);
    tft.setTextSize(1);
    int16_t x2 = (TFT_LANDSCAPE_W - (int16_t)(strlen(line2) * 6)) / 2;
    tft.setCursor(x2 > 0 ? x2 : 0, 64);
    tft.print(line2);

    tft.setTextColor(COLOR_GREY, TFT_BLACK);
    int16_t x3 = (TFT_LANDSCAPE_W - (int16_t)(strlen(verBuf) * 6)) / 2;
    tft.setCursor(x3 > 0 ? x3 : 0, 80);
    tft.print(verBuf);

    delay(1000);
}

void display_nextView()
{
    display_setView((currentView + 1) % VIEW_COUNT);
}

void display_setView(uint8_t view, bool force)
{
    if (currentView != view || force) {
        currentView = view;
        tft.fillScreen(COLOR_BG);
        view0FrameDrawn = false;
    }
}

void display_overlay(const char* msg, uint16_t color)
{
    // Semi-transparent overlay panel (filled dark box)
    uint16_t bx = 10, by = 50;
    uint16_t bw = TFT_LANDSCAPE_W - 20;
    uint16_t bh = 28;

    tft.fillRoundRect(bx, by, bw, bh, 4, COLOR_BG);
    tft.drawRoundRect(bx, by, bw, bh, 4, color);

    tft.setTextSize(1);
    tft.setTextColor(color, COLOR_BG);
    int16_t mx = (TFT_LANDSCAPE_W - (int16_t)(strlen(msg) * 6)) / 2;
    tft.setCursor(mx, by + 10);
    tft.print(msg);
}

void display_update(
    uint8_t     moisture,
    uint8_t     waterLevel,
    bool        pumpOn,
    bool        tankEmpty,
    int32_t     rssi,
    const char* ssid,
    const char* localIP,
    uint8_t     threshold,
    uint32_t    uptime_s)
{
    // Controller offset is handled in platformio.ini

    // Draw the common header bar
    const char* title = "Dashboard";
    if (pumpOn) title = "Watering";
    else if (currentView == 1) title = "Wi-Fi Info";
    else if (currentView == 2) title = "System";
    
    if (currentView != 0 || pumpOn || tankEmpty) {
        drawHeader(title, pumpOn, tankEmpty);
    }

    if (pumpOn) {
        drawView_Pumping(moisture, uptime_s);
    } else {
        // If we just finished pumping, force clear the screen to resume normal view
        if (currentView == 255) {
            currentView = 0; // Return to main view
            tft.fillScreen(COLOR_BG);
            view0FrameDrawn = false;
        }

        // Dispatch to the active view renderer
        switch (currentView) {
            case 0:
                drawView0_Stats(moisture, waterLevel, pumpOn, tankEmpty,
                                threshold, uptime_s);
                break;
            case 1:
                drawView1_WiFi(rssi, ssid, localIP, pumpOn, tankEmpty, uptime_s);
                break;
            case 2:
                drawView2_System(threshold, 0, uptime_s);
                break;
            default:
                break;
        }

        if (tankEmpty && currentView == 0) {
            display_overlay("TANK EMPTY!", COLOR_RED);
        }
    }
}

// Backward-compatible overload used by main.cpp in this branch.
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
    uint32_t    uptime_s)
{
    (void)pwmValue;
    display_update(moisture, waterLevel, pumpOn, tankEmpty, rssi, ssid, localIP, threshold, uptime_s);
}
