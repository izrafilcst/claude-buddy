#pragma once
#include <TFT_eSPI.h>
#include "api_client.h"

class DisplayManager {
public:
    void begin();
    void drawDataScreen(const ClaudeStatus& status, int battery_pct, bool wifi_ok);
    void drawIdleBackground();
    void animateHeaderDots();
    void drawErrorOverlay(const char* message);
    void setBacklight(bool on);
    TFT_eSPI* tft() { return &_tft; }

private:
    TFT_eSPI _tft;

    void _drawCard(int y, int h);
    void _drawHeader();
    void _drawQuotaCard(int percent, int reset_min);
    void _drawSessionsCard(int count);
    void _drawPlanCard(const char* plan);
    void _drawStatusBar(int battery_pct, bool wifi, unsigned long fetch_time);
};
