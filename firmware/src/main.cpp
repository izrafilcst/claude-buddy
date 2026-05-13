#include <Arduino.h>
#include <WiFi.h>
#include "config.h"
#include "colors.h"
#include "api_client.h"
#include "display_manager.h"

static DisplayManager display;
static ApiClient api;
static ClaudeStatus status;
static unsigned long last_poll = 0;
static bool wifi_ok = false;

void setup() {
    Serial.begin(115200);
    display.begin();

    WiFi.begin(WIFI_SSID, WIFI_PASS);
    display.tft()->setTextColor(Colors::CLOUDY, Colors::PAMPAS);
    display.tft()->setTextDatum(MC_DATUM);
    display.tft()->drawString("Connecting WiFi...", SCREEN_W / 2, SCREEN_H / 2, 2);

    for (int i = 0; i < 20 && WiFi.status() != WL_CONNECTED; i++) {
        delay(500);
    }
    wifi_ok = (WiFi.status() == WL_CONNECTED);

    api.begin(AGENT_HOST, AGENT_PORT);
    api.fetch(status);

    display.drawDataScreen(status, 100, wifi_ok);
}

void loop() {
    unsigned long now = millis();
    if (now - last_poll >= POLL_INTERVAL_MS) {
        wifi_ok = (WiFi.status() == WL_CONNECTED);
        if (wifi_ok) api.fetch(status);
        display.drawDataScreen(status, 100, wifi_ok);
        last_poll = now;
    }
    display.animateHeaderDots();
    delay(ANIM_FRAME_MS);
}
