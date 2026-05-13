#include "display_manager.h"
#include "colors.h"
#include "config.h"

namespace L {
    constexpr int M = 12;
    constexpr int CARD_W = SCREEN_W - 2 * M;
    constexpr int PAD = 10;

    constexpr int HEADER_Y = 8;
    constexpr int QUOTA_Y = 48;
    constexpr int QUOTA_H = 100;
    constexpr int SESSION_Y = 156;
    constexpr int SESSION_H = 80;
    constexpr int PLAN_Y = 244;
    constexpr int PLAN_H = 35;
    constexpr int STATUS_Y = 295;
}

void DisplayManager::begin() {
    _tft.init();
    _tft.setRotation(0);
    _tft.fillScreen(Colors::PAMPAS);

    pinMode(PIN_TFT_BL, OUTPUT);
    setBacklight(true);

    uint16_t cal[] = TOUCH_CAL_DATA;
    _tft.setTouch(cal);
}

void DisplayManager::setBacklight(bool on) {
    digitalWrite(PIN_TFT_BL, on ? HIGH : LOW);
}

void DisplayManager::drawIdleBackground() {
    _tft.fillScreen(Colors::PAMPAS);
}

void DisplayManager::_drawCard(int y, int h) {
    _tft.fillRoundRect(L::M, y, L::CARD_W, h, 6, Colors::DARK_CARD);
    _tft.drawRoundRect(L::M, y, L::CARD_W, h, 6, Colors::DARK_BORDER);
}

void DisplayManager::_drawHeader() {
    _tft.setTextColor(Colors::CLOUDY, Colors::DARK_BG);
    _tft.setTextDatum(TL_DATUM);
    _tft.drawString("/", L::M, L::HEADER_Y, 4);
    _tft.setTextColor(Colors::WHITE, Colors::DARK_BG);
    _tft.drawString("claude", L::M + 14, L::HEADER_Y, 4);
}

void DisplayManager::animateHeaderDots() {
    float t = millis() * 0.003f;
    int bx = 170, by = L::HEADER_Y + 12;

    _tft.fillRect(bx - 5, by - 5, 50, 10, Colors::DARK_BG);
    for (int i = 0; i < 3; i++) {
        float phase = sinf(t + i * 2.094f);
        int r = 2 + (int)((phase + 1.0f) * 1.0f);
        _tft.fillCircle(bx + i * 14, by, r, Colors::CRAIL);
    }
}

void DisplayManager::_drawQuotaCard(int percent, int reset_min) {
    _drawCard(L::QUOTA_Y, L::QUOTA_H);
    int ix = L::M + L::PAD;
    int iw = L::CARD_W - 2 * L::PAD;

    _tft.setTextColor(Colors::CLOUDY, Colors::DARK_CARD);
    _tft.setTextDatum(TL_DATUM);
    _tft.drawString("QUOTA", ix, L::QUOTA_Y + 8, 2);

    int bar_y = L::QUOTA_Y + 30;
    int bar_h = 18;
    _tft.drawRoundRect(ix - 1, bar_y - 1, iw + 2, bar_h + 2, 4, Colors::DARK_BORDER);

    if (percent >= 0) {
        int fill = (iw * percent) / 100;
        for (int i = 0; i < fill; i++) {
            float frac = (float)i / iw;
            _tft.drawFastVLine(ix + i, bar_y, bar_h,
                Colors::lerp(Colors::CRAIL, Colors::CRAIL_LIGHT, frac));
        }

        char buf[8];
        snprintf(buf, sizeof(buf), "%d%%", percent);
        _tft.setTextColor(Colors::WHITE, Colors::DARK_CARD);
        _tft.setTextDatum(TL_DATUM);
        _tft.drawString(buf, ix, bar_y + bar_h + 8, 4);
    } else {
        _tft.setTextColor(Colors::CLOUDY, Colors::DARK_CARD);
        _tft.setTextDatum(MC_DATUM);
        _tft.drawString("?", ix + iw / 2, bar_y + bar_h / 2, 4);
    }

    if (reset_min > 0) {
        char rb[24];
        snprintf(rb, sizeof(rb), "resets %dh%02dm", reset_min / 60, reset_min % 60);
        _tft.setTextColor(Colors::CLOUDY, Colors::DARK_CARD);
        _tft.setTextDatum(TR_DATUM);
        _tft.drawString(rb, ix + iw, bar_y + bar_h + 10, 1);
    }
}

void DisplayManager::_drawSessionsCard(int count) {
    _drawCard(L::SESSION_Y, L::SESSION_H);
    int ix = L::M + L::PAD;

    _tft.setTextColor(Colors::CLOUDY, Colors::DARK_CARD);
    _tft.setTextDatum(TL_DATUM);
    _tft.drawString("SESSIONS", ix, L::SESSION_Y + 8, 2);

    int sy = L::SESSION_Y + 32;
    int sz = 16, gap = 6, max_sq = 8;
    for (int i = 0; i < max_sq; i++) {
        int sx = ix + i * (sz + gap);
        if (i < count)
            _tft.fillRoundRect(sx, sy, sz, sz, 2, Colors::CRAIL);
        else
            _tft.drawRoundRect(sx, sy, sz, sz, 2, Colors::DARK_BORDER);
    }

    char buf[16];
    snprintf(buf, sizeof(buf), "%d active", count);
    _tft.setTextColor(Colors::CLOUDY, Colors::DARK_CARD);
    _tft.drawString(buf, ix, sy + sz + 6, 1);
}

void DisplayManager::_drawPlanCard(const char* plan) {
    _drawCard(L::PLAN_Y, L::PLAN_H);
    int ix = L::M + L::PAD;

    _tft.setTextColor(Colors::CLOUDY, Colors::DARK_CARD);
    _tft.setTextDatum(TL_DATUM);
    _tft.drawString("PLAN:", ix, L::PLAN_Y + 10, 2);
    _tft.setTextColor(Colors::CRAIL_LIGHT, Colors::DARK_CARD);
    _tft.drawString(plan, ix + 52, L::PLAN_Y + 10, 2);
}

void DisplayManager::_drawStatusBar(int battery_pct, bool wifi, unsigned long fetch_time) {
    int y = L::STATUS_Y;

    _tft.fillCircle(L::M + 4, y + 8, 4, wifi ? Colors::CRAIL : Colors::GRAY);
    _tft.setTextColor(Colors::CLOUDY, Colors::DARK_BG);
    _tft.setTextDatum(TL_DATUM);
    _tft.drawString("WiFi", L::M + 12, y + 4, 1);

    char bb[8];
    snprintf(bb, sizeof(bb), "%d%%", battery_pct);
    _tft.drawString("Bat", L::M + 70, y + 4, 1);
    _tft.setTextColor(Colors::WHITE, Colors::DARK_BG);
    _tft.drawString(bb, L::M + 92, y + 4, 1);

    if (fetch_time > 0) {
        unsigned long ago = (millis() - fetch_time) / 1000;
        char tb[16];
        if (ago < 60)
            snprintf(tb, sizeof(tb), "%lus", ago);
        else
            snprintf(tb, sizeof(tb), "%lum", ago / 60);
        _tft.setTextColor(Colors::CLOUDY, Colors::DARK_BG);
        _tft.setTextDatum(TR_DATUM);
        _tft.drawString(tb, SCREEN_W - L::M, y + 4, 1);
    }
}

void DisplayManager::drawDataScreen(const ClaudeStatus& status, int battery_pct, bool wifi_ok) {
    _tft.fillScreen(Colors::DARK_BG);
    _drawHeader();
    _drawQuotaCard(status.quota_percent, status.reset_minutes);
    _drawSessionsCard(status.sessions_active);
    _drawPlanCard(status.plan);
    _drawStatusBar(battery_pct, wifi_ok, status.fetch_time);
}

void DisplayManager::drawErrorOverlay(const char* message) {
    _drawCard(L::PLAN_Y, L::PLAN_H);
    _tft.setTextColor(Colors::CRAIL, Colors::DARK_CARD);
    _tft.setTextDatum(MC_DATUM);
    _tft.drawString(message, SCREEN_W / 2, L::PLAN_Y + L::PLAN_H / 2, 2);
}
