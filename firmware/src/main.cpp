#include <Arduino.h>
#include <WiFi.h>
#include "config.h"
#include "colors.h"
#include "api_client.h"

static ApiClient api;
static ClaudeStatus status;
static unsigned long last_poll = 0;

void setup() {
    Serial.begin(115200);
    Serial.println("Claude Tamagotchi booting...");

    pinMode(PIN_TFT_BL, OUTPUT);
    digitalWrite(PIN_TFT_BL, HIGH);

    WiFi.begin(WIFI_SSID, WIFI_PASS);
    Serial.print("WiFi");
    for (int i = 0; i < 20 && WiFi.status() != WL_CONNECTED; i++) {
        delay(500);
        Serial.print(".");
    }
    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf(" OK %s\n", WiFi.localIP().toString().c_str());
    } else {
        Serial.println(" FAIL");
    }

    api.begin(AGENT_HOST, AGENT_PORT);
    if (api.fetch(status)) {
        Serial.printf("Quota: %d%% Sessions: %d Plan: %s\n",
            status.quota_percent, status.sessions_active, status.plan);
    } else {
        Serial.println("Agent fetch failed");
    }
}

void loop() {
    unsigned long now = millis();
    if (now - last_poll >= POLL_INTERVAL_MS) {
        if (api.fetch(status)) {
            Serial.printf("Quota: %d%% Sessions: %d\n",
                status.quota_percent, status.sessions_active);
        }
        last_poll = now;
    }
    delay(100);
}
