#pragma once
#include <TFT_eSPI.h>

enum class TouchEvent : uint8_t {
    NONE,
    TAP,
    LONG_PRESS
};

class TouchHandler {
public:
    void begin(TFT_eSPI* tft);
    TouchEvent update();

private:
    TFT_eSPI* _tft = nullptr;
    bool _was_touched = false;
    unsigned long _touch_start = 0;
    unsigned long _last_event = 0;

    static constexpr unsigned long DEBOUNCE_MS = 50;
    static constexpr unsigned long LONG_PRESS_MS = 2000;
};
