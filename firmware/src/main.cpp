#include <Arduino.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <Preferences.h>
#include "config.h"
#include "colors.h"
#include "api_client.h"
#include "display_manager.h"
#include "touch_handler.h"
#include "tamagotchi.h"
#include "alert.h"
#include "battery.h"
#ifdef DEBUG_MODE
#include "debug_scenarios.h"
#endif

enum class Mode : uint8_t { IDLE, DATA, SETTINGS };

static DisplayManager display;
static TouchHandler touch;
static ApiClient api;
static Tamagotchi mascot;
static Alert alert;
static Battery battery;
static Preferences prefs;
static ClaudeStatus status;

static Mode mode = Mode::IDLE;
static bool wifi_ok = false;
static bool data_dirty = false;
static int battery_pct = 100;
static unsigned long last_poll = 0;
static unsigned long last_touch_time = 0;
static unsigned long last_bat_read = 0;

static char agent_host[64] = AGENT_HOST;
static uint16_t agent_port = AGENT_PORT;
static int alert_threshold = ALERT_THRESHOLD_DEFAULT;

#ifdef DEBUG_MODE
static int debug_idx = 0;
static unsigned long debug_last_advance = 0;

static void debugLoad(int idx) {
    debug_idx = ((idx % DEBUG_SCENARIO_COUNT) + DEBUG_SCENARIO_COUNT) % DEBUG_SCENARIO_COUNT;
    status = debugBuildStatus(debug_idx);
    status.fetch_time = millis();
    data_dirty = true;
    debug_last_advance = millis();
    Serial.printf("[DEBUG] %d/%d %s\n", debug_idx + 1, DEBUG_SCENARIO_COUNT, DEBUG_DATA[debug_idx].label);
}

static void debugLabel() {
    char buf[28];
    snprintf(buf, sizeof(buf), "[DBG] %s  %d/%d",
             DEBUG_DATA[debug_idx].label, debug_idx + 1, DEBUG_SCENARIO_COUNT);
    TFT_eSPI* tft = display.tft();
    tft->setTextDatum(MC_DATUM);
    tft->fillRect(0, 0, SCREEN_W, 14, Colors::DARK_BG);
    tft->setTextColor(Colors::CRAIL_LIGHT, Colors::DARK_BG);
    tft->drawString(buf, SCREEN_W / 2, 7, 1);
}
#endif

static void loadSettings() {
    prefs.begin("claude", true);
    strlcpy(agent_host, prefs.getString("host", AGENT_HOST).c_str(), sizeof(agent_host));
    agent_port = prefs.getUShort("port", AGENT_PORT);
    alert_threshold = prefs.getInt("alert_thr", ALERT_THRESHOLD_DEFAULT);
    prefs.end();
}

static void saveSettings() {
    prefs.begin("claude", false);
    prefs.putString("host", agent_host);
    prefs.putUShort("port", agent_port);
    prefs.putInt("alert_thr", alert_threshold);
    prefs.end();
}

static void poll() {
    wifi_ok = (WiFi.status() == WL_CONNECTED);
    if (!wifi_ok) {
        WiFi.reconnect();
        status.valid = false;
        return;
    }
    api.fetch(status);
    data_dirty = true;
}

static void switchMode(Mode next) {
    if (mode == next) return;
    mode = next;
    switch (next) {
    case Mode::IDLE:
        display.drawIdleBackground();
        break;
    case Mode::DATA:
        display.drawDataScreen(status, battery_pct, wifi_ok);
        data_dirty = false;
        break;
    case Mode::SETTINGS:
        break;
    }
}

static void drawSettings() {
    TFT_eSPI* tft = display.tft();
    tft->fillScreen(Colors::DARK_BG);

    tft->setTextDatum(TL_DATUM);
    tft->setTextColor(Colors::WHITE, Colors::DARK_BG);
    tft->drawString("Settings", 12, 10, 4);

    int y = 55;
    tft->setTextColor(Colors::CLOUDY, Colors::DARK_BG);
    tft->drawString("WiFi:", 12, y, 2);
    tft->setTextColor(Colors::WHITE, Colors::DARK_BG);
    tft->drawString(WiFi.SSID().c_str(), 20, y + 20, 2);
    tft->drawString(WiFi.localIP().toString().c_str(), 20, y + 40, 2);

    y += 80;
    tft->setTextColor(Colors::CLOUDY, Colors::DARK_BG);
    tft->drawString("Agent:", 12, y, 2);
    char ab[64];
    snprintf(ab, sizeof(ab), "%s:%d", agent_host, agent_port);
    tft->setTextColor(Colors::WHITE, Colors::DARK_BG);
    tft->drawString(ab, 20, y + 20, 2);

    y += 55;
    tft->setTextColor(Colors::CLOUDY, Colors::DARK_BG);
    tft->drawString("Alert threshold:", 12, y, 2);
    char tb[8];
    snprintf(tb, sizeof(tb), "%d%%", alert_threshold);
    tft->setTextColor(Colors::WHITE, Colors::DARK_BG);
    tft->drawString(tb, 20, y + 20, 2);

    y += 55;
    tft->setTextColor(Colors::CLOUDY, Colors::DARK_BG);
    tft->drawString("MAC:", 12, y, 2);
    tft->setTextColor(Colors::WHITE, Colors::DARK_BG);
    tft->drawString(WiFi.macAddress().c_str(), 20, y + 18, 1);

    tft->drawString("Firmware: v1.0.0", 12, y + 35, 1);

    tft->setTextColor(Colors::CRAIL, Colors::DARK_BG);
    tft->setTextDatum(MC_DATUM);
    tft->drawString("Tap to return", SCREEN_W / 2, 295, 2);
    tft->drawString("Long press for WiFi setup", SCREEN_W / 2, 310, 1);
}

void setup() {
    Serial.begin(115200);
    display.begin();
    touch.begin(display.tft());

#ifdef DEBUG_MODE
    Serial.println("[DEBUG] mode — WiFi disabled");
    display.tft()->setTextDatum(MC_DATUM);
    display.tft()->setTextColor(Colors::CRAIL_LIGHT, Colors::PAMPAS);
    display.tft()->drawString("[DEBUG MODE]", SCREEN_W / 2, SCREEN_H / 2, 2);
    delay(800);
    wifi_ok = false;
#else
    loadSettings();
    display.tft()->setTextColor(Colors::CLOUDY, Colors::PAMPAS);
    display.tft()->setTextDatum(MC_DATUM);
    display.tft()->drawString("Connecting...", SCREEN_W / 2, SCREEN_H / 2, 2);
    WiFiManager wm;
    wm.setConfigPortalTimeout(180);
    wm.autoConnect("ClaudeTamagotchi");
    wifi_ok = (WiFi.status() == WL_CONNECTED);
    api.begin(agent_host, agent_port);
#endif

    mascot.begin(display.tft());
    alert.begin(PIN_PIEZO);
    alert.setThreshold(alert_threshold);
    battery.begin(PIN_BAT_ADC);
    battery_pct = battery.percent();

#ifdef DEBUG_MODE
    debugLoad(0);
    switchMode(Mode::DATA);
#else
    poll();
    switchMode(Mode::IDLE);
#endif
}

void loop() {
    unsigned long now = millis();
    TouchEvent ev = touch.update();

#ifdef DEBUG_MODE
    if (ev == TouchEvent::TAP) {
        debugLoad(debug_idx + 1);
        switchMode(Mode::DATA);
    } else if (ev == TouchEvent::LONG_PRESS) {
        debugLoad(debug_idx - 1);
        switchMode(Mode::DATA);
    }
    if (now - debug_last_advance >= DEBUG_INTERVAL_MS) {
        debugLoad(debug_idx + 1);
        switchMode(Mode::DATA);
    }
    if (now - last_bat_read >= BATTERY_READ_MS) {
        battery_pct = battery.percent();
        last_bat_read = now;
    }
    alert.check(status.quota_percent);
    if (data_dirty) {
        display.drawDataScreen(status, battery_pct, false);
        debugLabel();
        data_dirty = false;
    }
    display.animateHeaderDots();
#else
    switch (mode) {
    case Mode::IDLE:
        if (ev == TouchEvent::TAP) {
            last_touch_time = now;
            poll();
            switchMode(Mode::DATA);
        } else if (ev == TouchEvent::LONG_PRESS) {
            drawSettings();
            mode = Mode::SETTINGS;
        }
        break;
    case Mode::DATA:
        if (ev == TouchEvent::TAP) {
            last_touch_time = now;
        } else if (ev == TouchEvent::LONG_PRESS) {
            drawSettings();
            mode = Mode::SETTINGS;
        }
        break;
    case Mode::SETTINGS:
        if (ev == TouchEvent::TAP) {
            switchMode(Mode::IDLE);
        } else if (ev == TouchEvent::LONG_PRESS) {
            display.tft()->setTextDatum(MC_DATUM);
            display.tft()->setTextColor(Colors::CRAIL_LIGHT, Colors::DARK_BG);
            display.tft()->drawString("Starting WiFi portal...", SCREEN_W / 2, SCREEN_H / 2, 2);
            WiFiManager wm;
            wm.setConfigPortalTimeout(180);
            wm.startConfigPortal("ClaudeTamagotchi");
            wifi_ok = (WiFi.status() == WL_CONNECTED);
            switchMode(Mode::IDLE);
        }
        break;
    }
    if (now - last_poll >= POLL_INTERVAL_MS) {
        poll();
        last_poll = now;
    }
    if (now - last_bat_read >= BATTERY_READ_MS) {
        battery_pct = battery.percent();
        last_bat_read = now;
    }
    if (mode == Mode::DATA && now - last_touch_time >= DATA_TIMEOUT_MS) {
        switchMode(Mode::IDLE);
    }
    alert.check(status.quota_percent);
    if (alert.justTriggered() && mode == Mode::IDLE) {
        last_touch_time = now;
        switchMode(Mode::DATA);
    }
    switch (mode) {
    case Mode::IDLE:
        mascot.setState(status.quota_percent, status.valid, wifi_ok);
        mascot.drawFrame();
        break;
    case Mode::DATA:
        if (data_dirty) {
            display.drawDataScreen(status, battery_pct, wifi_ok);
            data_dirty = false;
        }
        display.animateHeaderDots();
        if (!wifi_ok) {
            display.drawErrorOverlay("WiFi disconnected");
        } else if (!status.valid) {
            display.drawErrorOverlay("Agent offline");
        }
        break;
    case Mode::SETTINGS:
        break;
    }
#endif

    delay(ANIM_FRAME_MS);
}
