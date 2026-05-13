#include "battery.h"

void Battery::begin(uint8_t pin) {
    _pin = pin;
    analogReadResolution(12);
    analogSetAttenuation(ADC_11db);
}

int Battery::percent() {
    int raw = analogRead(_pin);
    float voltage = (raw / 4095.0f) * 3.3f * 2.0f;
    float pct = (voltage - 3.0f) / (4.2f - 3.0f) * 100.0f;
    return constrain((int)pct, 0, 100);
}
