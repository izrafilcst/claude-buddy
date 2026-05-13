#include <Arduino.h>
#include <WiFi.h>
#include "config.h"
#include "colors.h"

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
}

void loop() {
    delay(1000);
}
