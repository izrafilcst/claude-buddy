#pragma once

#define WIFI_SSID "YOUR_SSID"
#define WIFI_PASS "YOUR_PASS"

#define AGENT_HOST "192.168.1.100"
#define AGENT_PORT 8266

#define PIN_TFT_BL    48
#define PIN_TOUCH_CS  12
#define PIN_TOUCH_IRQ 21
#define PIN_PIEZO     47
#define PIN_BAT_ADC   1

#define SCREEN_W 240
#define SCREEN_H 320

#define POLL_INTERVAL_MS  30000
#define DATA_TIMEOUT_MS   10000
#define ANIM_FRAME_MS     66
#define BATTERY_READ_MS   60000

// Run TFT_eSPI Touch_calibrate example to get values for your display
#define TOUCH_CAL_DATA {300, 3600, 300, 3600, 7}

#define ALERT_THRESHOLD_DEFAULT 20
