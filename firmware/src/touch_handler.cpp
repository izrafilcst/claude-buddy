#include "touch_handler.h"

void TouchHandler::begin(TFT_eSPI* tft) {
    _tft = tft;
}

TouchEvent TouchHandler::update() {
    uint16_t x, y;
    bool touched = _tft->getTouch(&x, &y);
    unsigned long now = millis();

    if (now - _last_event < DEBOUNCE_MS) return TouchEvent::NONE;

    if (touched && !_was_touched) {
        _touch_start = now;
        _was_touched = true;
    } else if (touched && _was_touched) {
        if (now - _touch_start >= LONG_PRESS_MS) {
            _was_touched = false;
            _last_event = now;
            return TouchEvent::LONG_PRESS;
        }
    } else if (!touched && _was_touched) {
        _was_touched = false;
        _last_event = now;
        if (now - _touch_start < LONG_PRESS_MS) {
            return TouchEvent::TAP;
        }
    }

    return TouchEvent::NONE;
}
