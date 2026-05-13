#include <Arduino.h>
#include <WiFi.h>
#include "config.h"
#include "colors.h"
#include "api_client.h"
#include "display_manager.h"
#include "touch_handler.h"
#include "tamagotchi.h"
#include "alert.h"
#include "battery.h"

enum class Mode : uint8_t { IDLE, DATA };

static DisplayManager display;
static TouchHandler touch;
static ApiClient api;
static Tamagotchi mascot;
static Alert alert;
static Battery battery;
static ClaudeStatus status;

static Mode mode = Mode::IDLE;
static bool wifi_ok = false;
static bool data_dirty = false;
static int battery_pct = 100;
static unsigned long last_poll = 0;
static unsigned long last_touch_time = 0;
static unsigned long last_bat_read = 0;

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
    if (next == Mode::IDLE) {
        display.drawIdleBackground();
    } else {
        display.drawDataScreen(status, battery_pct, wifi_ok);
        data_dirty = false;
    }
}

void setup() {
    Serial.begin(115200);
    display.begin();
    touch.begin(display.tft());
    api.begin(AGENT_HOST, AGENT_PORT);
    mascot.begin(display.tft());
    alert.begin(PIN_PIEZO);
    alert.setThreshold(ALERT_THRESHOLD_DEFAULT);
    battery.begin(PIN_BAT_ADC);

    display.tft()->setTextColor(Colors::CLOUDY, Colors::PAMPAS);
    display.tft()->setTextDatum(MC_DATUM);
    display.tft()->drawString("Connecting...", SCREEN_W / 2, SCREEN_H / 2, 2);

    WiFi.begin(WIFI_SSID, WIFI_PASS);
    for (int i = 0; i < 20 && WiFi.status() != WL_CONNECTED; i++) delay(500);
    wifi_ok = (WiFi.status() == WL_CONNECTED);

    battery_pct = battery.percent();
    poll();
    switchMode(Mode::IDLE);
}

void loop() {
    unsigned long now = millis();

    TouchEvent ev = touch.update();
    if (ev == TouchEvent::TAP) {
        last_touch_time = now;
        if (mode == Mode::IDLE) {
            poll();
            switchMode(Mode::DATA);
        }
    } else if (ev == TouchEvent::LONG_PRESS) {
        Serial.println("Long press - settings placeholder");
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
    }

    delay(ANIM_FRAME_MS);
}
