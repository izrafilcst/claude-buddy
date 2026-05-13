#include <Arduino.h>
#include <WiFi.h>
#include "config.h"
#include "colors.h"
#include "api_client.h"
#include "display_manager.h"
#include "touch_handler.h"
#include "tamagotchi.h"

enum class Mode : uint8_t { IDLE, DATA };

static DisplayManager display;
static TouchHandler touch;
static ApiClient api;
static Tamagotchi mascot;
static ClaudeStatus status;

static Mode mode = Mode::IDLE;
static bool wifi_ok = false;
static bool data_dirty = false;
static unsigned long last_poll = 0;
static unsigned long last_touch_time = 0;

static void poll() {
    wifi_ok = (WiFi.status() == WL_CONNECTED);
    if (!wifi_ok) { WiFi.reconnect(); return; }
    if (api.fetch(status)) data_dirty = true;
}

static void switchMode(Mode next) {
    if (mode == next) return;
    mode = next;
    if (next == Mode::IDLE) {
        display.drawIdleBackground();
    } else {
        display.drawDataScreen(status, 100, wifi_ok);
        data_dirty = false;
    }
}

void setup() {
    Serial.begin(115200);
    display.begin();
    touch.begin(display.tft());
    api.begin(AGENT_HOST, AGENT_PORT);
    mascot.begin(display.tft());

    display.tft()->setTextColor(Colors::CLOUDY, Colors::PAMPAS);
    display.tft()->setTextDatum(MC_DATUM);
    display.tft()->drawString("Connecting...", SCREEN_W / 2, SCREEN_H / 2, 2);

    WiFi.begin(WIFI_SSID, WIFI_PASS);
    for (int i = 0; i < 20 && WiFi.status() != WL_CONNECTED; i++) delay(500);
    wifi_ok = (WiFi.status() == WL_CONNECTED);

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
    }

    if (now - last_poll >= POLL_INTERVAL_MS) {
        poll();
        last_poll = now;
    }

    if (mode == Mode::DATA && now - last_touch_time >= DATA_TIMEOUT_MS) {
        switchMode(Mode::IDLE);
    }

    switch (mode) {
    case Mode::IDLE:
        mascot.setState(status.quota_percent, status.valid, wifi_ok);
        mascot.drawFrame();
        break;
    case Mode::DATA:
        if (data_dirty) {
            display.drawDataScreen(status, 100, wifi_ok);
            data_dirty = false;
        }
        display.animateHeaderDots();
        break;
    }

    delay(ANIM_FRAME_MS);
}
