#pragma once
#include <Arduino.h>

class Battery {
public:
    void begin(uint8_t pin);
    int percent();

private:
    uint8_t _pin = 0;
};
