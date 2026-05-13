# Claude Tamagotchi Monitor — Design Spec

## Overview

An IoT device that monitors Claude Code usage (quota remaining + active sessions) on a small TFT display, featuring a Tamagotchi-style animated Claude mascot that reacts to quota levels. When idle, the mascot lives on screen with dynamic animations. On touch, the display shows real-time usage data.

## System Architecture

```
┌─────────────────────┐          HTTP/JSON          ┌─────────────────────┐
│   PC (Linux)        │ ◄──────── polling 30s ──────│   ESP32-S3          │
│                     │                              │                     │
│  ┌───────────────┐  │   GET /status                │  ┌───────────────┐ │
│  │ Agent Python  │  │ ──────────────────────────►  │  │ ILI9341 240x320│ │
│  │               │  │   { quota: 62,               │  │ + XPT2046     │ │
│  │ - detect      │  │     sessions: 3,             │  │               │ │
│  │   claude-code │  │     reset_min: 134,          │  │ Modo Idle:    │ │
│  │   processes   │  │     plan: "max" }            │  │  Tamagotchi   │ │
│  │ - serve HTTP  │  │                              │  │               │ │
│  └───────────────┘  │                              │  │ Modo Dados:   │ │
│                     │                              │  │  Quota+Sessoes│ │
│  porta 8266         │         Wi-Fi LAN            │  └───────────────┘ │
└─────────────────────┘                              │  Piezo passivo     │
                                                     │  Li-Po + USB carga │
                                                     └─────────────────────┘
```

### Components

1. **Agent Python (PC)**: Lightweight daemon that detects Claude Code sessions via process inspection, estimates quota from local Claude Code data, and serves a JSON endpoint on the LAN.
2. **Firmware ESP32-S3 (Arduino C++)**: Connects to Wi-Fi, polls the agent, renders the Tamagotchi mascot and data screens, handles touch input and alerts.
3. **Protocol**: Simple HTTP GET returning JSON. Single endpoint.

## Hardware

### Bill of Materials

| Component | Model | Function |
|---|---|---|
| MCU | ESP32-S3 (DevKitC or similar) | Wi-Fi, logic, rendering |
| Display | ILI9341 2.8" 240x320 SPI TFT | Main screen |
| Touch | XPT2046 (integrated in display module) | User input |
| Battery | Li-Po 3.7V ~1000mAh | Portable power |
| Charger | TP4056 USB-C module | Battery charging |
| Alert | Passive piezo buzzer | Audio notification |

### Pin Mapping (ESP32-S3 → ILI9341 + XPT2046)

```
ESP32-S3          ILI9341         XPT2046
─────────         ───────         ───────
GPIO 18 (SCK)  →  CLK             CLK
GPIO 11 (MOSI) →  MOSI (DIN)     DIN
GPIO 13 (MISO) ←  MISO (DOUT)    DOUT
GPIO 10        →  CS (LCD)
GPIO 14        →  DC/RS
GPIO 9         →  RESET
GPIO 12        →                  CS (Touch)
GPIO 21        →                  IRQ (touch interrupt)
GPIO 48        →  LED (backlight)
GPIO 47        →  Piezo (+)
3.3V           →  VCC             VCC
GND            →  GND             GND
```

### Battery Notes

The ESP32-S3 DevKitC typically does not include a Li-Po charger. A TP4056 module (USB-C variant) handles charging. Battery level is read via ADC on a voltage divider.

### Arduino Libraries

- `TFT_eSPI` — ILI9341 display driver (native ESP32-S3 support)
- `XPT2046_Touchscreen` — touch driver
- `WiFi.h` / `HTTPClient.h` — networking
- `ArduinoJson` — JSON parsing

## Display Modes

### Mode Lifecycle

```
                         touch
            ┌──────────────────────────┐
            ▼                          │
    ┌──────────────┐   10s no touch   ┌──────────────┐
    │  DATA MODE   │ ───────────────► │  IDLE MODE   │
    │              │                  │ (Tamagotchi) │
    │ fetch on     │ ◄── auto-switch  │              │
    │ touch +      │  quota crossed   │ polling 30s  │
    │ polling 30s  │  alert threshold │ mascot alive │
    └──────────────┘                  └──────────────┘
```

### Idle Mode (Tamagotchi)

**Theme:** Light — Pampas background (#F4F3EE)

**Mascot:** A circle (orb) with a Crail (#C15F3C) gradient containing 3 animated dots inside (Claude's "thinking dots"). The mascot's behavior reflects the current quota level:

| Quota | State | Animation |
|---|---|---|
| 80-100% | Radiant | High bounce, fast-blinking dots, glowing particles around orb, strong glow |
| 60-79% | Content | Gentle bounce, normal dot rhythm, no particles |
| 40-59% | Normal | No bounce, breathing pulse (scale), slower dots |
| 20-39% | Tired | Orb sinks lower on screen, dots become lines (closing eyes), slight tremor |
| 5-19% | Exhausted | Orb at bottom of screen, minimal movement, faded color, floating "Zzz" |
| 0% | Off | Gray orb, dots replaced by "x x x", no movement |

**Rendering approach:**
- Partial sprite buffer via TFT_eSPI (~60x60px orb sprite, not fullscreen)
- Pampas background drawn once, only orb area redrawn per frame
- ~15fps animation via state machine with pre-calculated frames
- Particles as small circles with simple position/velocity

### Data Mode

**Theme:** Dark — background #0d0a07, Claude brand palette

**Layout (style D from brainstorming):**
- Header: `/claude` branding with animated thinking dots
- Quota: progress bar with Crail gradient, percentage, reset timer
- Sessions: count with visual indicators (filled/empty squares)
- Status bar: Wi-Fi status, battery %, last update time

**Transitions:**
- Idle → Data: orb shrinks and slides to corner, data cards slide in
- Data → Idle: cards fade out, orb grows and returns to center

### Screen Always On

Display backlight remains constant regardless of quota level. The mascot's behavior expresses energy state, not the screen brightness.

## Alert System

### Trigger

When quota crosses below a configurable threshold (e.g., 50%), the alert fires **once per crossing**. It does not repeat while quota remains below the threshold. It will fire again only if quota rises above and crosses down again, or if a second threshold is configured (e.g., 50% and 20%).

### Behavior

1. Piezo plays a short melody (3 descending notes, ~1 second)
2. Auto-switches from idle → data mode (same as a touch)
3. After 10s without touch → returns to idle (mascot in corresponding state)

### Configuration

Alert thresholds are configurable via the on-device GUI (see Settings).

## Polling Strategy

| Context | Behavior |
|---|---|
| Touch on screen | Immediate HTTP fetch + switch to data mode |
| Data mode, no touch | Poll every 30s, return to idle after 10s without touch |
| Idle mode | Poll every 30s to keep mascot state accurate |
| Agent offline | Mascot shows confused expression (dots become "?"), discrete "no connection" text |
| Wi-Fi disconnected | Reconnect attempt every 30s, mascot shows "searching" state |

## Agent Python (PC)

### Endpoint

```
GET http://<pc-ip>:8266/status

Response:
{
  "quota_percent": 62,
  "sessions_active": 3,
  "sessions_details": [
    {"pid": 12345, "uptime_min": 45},
    {"pid": 12346, "uptime_min": 12},
    {"pid": 12347, "uptime_min": 3}
  ],
  "reset_minutes": 134,
  "plan": "max",
  "timestamp": 1718000000
}
```

### Session Detection

- `pgrep -af "claude"` to list active Claude Code processes
- Each matching process = one active session

### Quota Estimation

- Parse local Claude Code data from `~/.claude/` for usage information
- Fallback: if unable to read, return `quota_percent: -1` and ESP32 shows "?" in place of the value

### Stack

- Python stdlib only (`http.server`) — zero external dependencies
- Single file: `claude_agent.py`
- Runs as systemd service or in tmux

## Settings GUI (On-Device)

### Access

Long press (2 seconds) opens settings menu.

### Screens

| Screen | Configures | Default |
|---|---|---|
| Wi-Fi | Initial setup via WiFiManager (captive portal on phone) | — |
| Agent IP | IP:port of the PC agent | 192.168.x.x:8266 |
| Alert quota | Threshold % to trigger alert | 20% |
| Alert sessions | Session count threshold to alert | Disabled |
| About | Firmware version, MAC address, ESP32 IP | — |

### Wi-Fi Setup

Uses WiFiManager library (captive portal). ESP32 creates a hotspot, user connects via phone and configures Wi-Fi credentials. Threshold configs use the touch screen directly (numeric input only).

## Firmware Structure

```
claude-esp32/
├── claude-esp32.ino          # main setup/loop
├── config.h                  # WiFi SSID/pass, agent IP, pin definitions
├── api_client.h/.cpp         # HTTP client, JSON parsing
├── display_manager.h/.cpp    # TFT control, mode rendering
├── touch_handler.h/.cpp      # XPT2046 reading, debounce, long press
├── tamagotchi.h/.cpp         # mascot: states, animations, transitions
├── alert.h/.cpp              # piezo melodies, threshold logic
└── battery.h/.cpp            # ADC battery level reading
```

## Brand Identity

### Color Palette

| Name | Hex | Usage |
|---|---|---|
| Crail | #C15F3C | Primary — orb, progress bars, accents |
| Crail Light | #e8845c | Gradients, glow effects |
| Cloudy | #B1ADA1 | Secondary text, labels |
| Pampas | #F4F3EE | Idle mode background |
| Dark BG | #0d0a07 | Data mode background |
| Dark Card | #1a1410 | Data mode card backgrounds |
| Dark Border | #3a2a1e | Data mode borders |
| White | #F8FAFC | Data mode primary text |

### Branding Elements

- `/claude` text with slash in lighter color as header identity
- Thinking dots (3 pulsing circles) as the mascot's "face" and loading indicator
- Warm color temperature throughout (no cold blues/grays)

## Power Management

- Backlight always on at constant brightness
- Light sleep between animation frames in idle mode (wake on timer or touch IRQ)
- Battery level read via ADC every 60 seconds
- Wi-Fi stays connected (no deep sleep — needs to receive alert triggers)

## Error States

| Condition | Mascot Behavior | Data Mode Display |
|---|---|---|
| Agent offline | Confused face (dots → "?") | "Agent offline" message |
| Wi-Fi disconnected | "Searching" animation | "Wi-Fi disconnected" message |
| Invalid JSON response | No change (keeps last valid state) | Shows last valid data with stale indicator |
| Battery critical (<10%) | No change to mascot | Battery icon blinks in status bar |
