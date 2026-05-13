#pragma once
#include <Arduino.h>

class Alert {
public:
    void begin(uint8_t pin);
    void check(int quota_percent);
    bool justTriggered();
    void setThreshold(int threshold);

private:
    uint8_t _pin = 0;
    int _threshold = 20;
    int _prev_quota = 100;
    bool _triggered = false;

    void _playMelody();
};
