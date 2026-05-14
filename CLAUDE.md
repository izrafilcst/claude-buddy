# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Commands

### Agent (PC side)
```bash
python3 agent/claude_agent.py          # start the real HTTP agent on :8266
python3 -m pytest agent/ -v            # run all 7 tests
python3 -m pytest agent/test_claude_agent.py::TestDetectSessions::test_name -v  # single test
```

### Firmware (ESP32-S3)
All commands run from `firmware/`.
```bash
pio run -e esp32s3              # compile production firmware
pio run -e esp32s3 -t upload    # compile + flash
pio device monitor              # serial monitor at 115200 baud
pio run -e esp32s3-debug -t upload     # flash offline debug mode (no WiFi)
pio run -e esp32s3-demo-web -t upload  # flash with Railway mock host baked in
```

### Mock agent (demo/test without hardware)
```bash
python3 demo/mock_agent.py                      # auto-cycle all scenarios every 30s
python3 demo/mock_agent.py --scenario tired     # lock to a specific state
python3 demo/mock_agent.py --interval 5         # faster cycle for testing
curl "http://127.0.0.1:8266/control?scenario=exhausted"  # change state live
```

---

## Architecture

### Two separate programs that talk over HTTP

The project is split into two independent parts that never share code:

- **`agent/`** — Python server running on the developer's PC. Detects live Claude Code processes via `pgrep -af claude` and reads quota from `~/.claude/settings.json`. Serves `GET /status` on port 8266.
- **`firmware/`** — Arduino/C++ on the ESP32-S3. Polls `http://<host>:8266/status` every 30s and renders the result on the TFT display.

The JSON contract between them is defined by what `api_client.cpp` parses: `quota_percent`, `sessions_active`, `sessions_details[].pid`, `sessions_details[].uptime_min`, `reset_minutes`, `plan`. Any change to this shape must be reflected in both `claude_agent.py` (produces) and `api_client.cpp` (consumes).

### Firmware module responsibilities

| File | What it owns |
|---|---|
| `main.cpp` | State machine (IDLE/DATA/SETTINGS), WiFiManager setup, NVS settings, poll timer |
| `display_manager.cpp` | All TFT drawing — idle background, data screen, error overlay, settings screen |
| `tamagotchi.cpp` | Mascot state enum, 64×64 sprite animation, particle system, dirty-rect rendering |
| `api_client.cpp` | HTTP GET, JSON parsing via ArduinoJson v7, returns `ClaudeStatus` |
| `touch_handler.cpp` | Debounced TAP / LONG_PRESS events from TFT_eSPI built-in touch |
| `alert.cpp` | Threshold crossing detection → piezo melody via `tone()` |
| `battery.cpp` | ADC read on GPIO1, voltage divider ×2, Li-Po % calculation |
| `debug_scenarios.h` | Header-only: 7 hardcoded `DebugData` entries + `debugBuildStatus()` helper |

### TFT_eSPI configuration

TFT_eSPI is configured **entirely via PlatformIO build flags** (`-DUSER_SETUP_LOADED`, `-DILI9341_DRIVER`, pin defines). Do **not** edit the library's `User_Setup.h`. The touch controller (XPT2046) shares the SPI bus with the display — only the CS pins differ (`TFT_CS=10`, `TOUCH_CS=12`). Touch is accessed via `tft.getTouch()`, not a separate library.

### Display rendering pattern

The mascote orb uses a `TFT_eSprite` (64×64, allocated on heap as a pointer) with `TRANSPARENT` color key for push. Animations use dirty-rect: fill the previous bounding box with `PAMPAS` before drawing the new frame. The main data screen is only redrawn when `data_dirty = true` — never on every loop tick.

### PlatformIO environments

- `esp32s3` — production, connects to real agent at `AGENT_HOST:8266` (NVS overrides the config.h defaults at runtime via WiFiManager captive portal)
- `esp32s3-debug` — `#define DEBUG_MODE`: skips WiFiManager entirely, loads 7 hardcoded scenarios, TAP cycles forward, LONG_PRESS cycles back
- `esp32s3-demo-web` — same as production but `AGENT_HOST` baked in as the Railway public URL at compile time, `AGENT_PORT=80`

### Settings persistence

User settings (agent host, port, alert threshold) live in ESP32 NVS under the `"claude"` namespace via the `Preferences` library. Defaults come from `config.h` macros. WiFiManager credentials are stored separately by the WiFiManager library itself. To reset: hold long-press on the Settings screen → long-press again → opens captive portal.

### Demo folder

`demo/` is purely for development and simulation — nothing in it is included in the firmware build. `mock_agent.py` serves the same JSON schema as `claude_agent.py` and accepts `GET /control?scenario=<name>` to switch states live. `diagram.json` + `wokwi.toml` target the Wokwi VS Code extension (WiFi reaches localhost). `wokwi-web.toml` targets the Railway-hosted mock via a public URL.

### Colors

All display colors use RGB565 format defined in `colors.h` under the `Colors::` namespace. The brand palette (`CRAIL`, `CRAIL_LIGHT`, `CLOUDY`, `PAMPAS`, `DARK_BG`, `DARK_CARD`) maps to Claude's visual identity. `Colors::lerp()` interpolates between two RGB565 values for the quota progress bar gradient.
