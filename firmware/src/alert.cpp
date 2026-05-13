#include "alert.h"

void Alert::begin(uint8_t pin) {
    _pin = pin;
    pinMode(_pin, OUTPUT);
}

void Alert::check(int quota_percent) {
    _triggered = false;
    if (quota_percent < 0) return;
    if (_prev_quota >= _threshold && quota_percent < _threshold) {
        _triggered = true;
        _playMelody();
    }
    _prev_quota = quota_percent;
}

bool Alert::justTriggered() {
    return _triggered;
}

void Alert::setThreshold(int threshold) {
    _threshold = threshold;
}

void Alert::_playMelody() {
    tone(_pin, 880, 200);
    delay(250);
    tone(_pin, 659, 200);
    delay(250);
    tone(_pin, 440, 300);
    delay(350);
    noTone(_pin);
}
