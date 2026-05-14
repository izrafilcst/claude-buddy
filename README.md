<div align="center">

<img src="docs/mascot-beach.gif" width="480" alt="Claw'd — day fishing, night sleeping"/>

# Claude Buddy

**Physical status monitor for Claude Code sessions — ESP32-S3 + 240×320 TFT display**

[![GitHub stars](https://img.shields.io/github/stars/izrafilcst/claude-buddy?style=flat-square&color=C15F3C)](https://github.com/izrafilcst/claude-buddy/stargazers)
[![GitHub license](https://img.shields.io/github/license/izrafilcst/claude-buddy?style=flat-square)](LICENSE)
[![PlatformIO](https://img.shields.io/badge/PlatformIO-ESP32--S3-orange?style=flat-square)](https://platformio.org)
[![Python](https://img.shields.io/badge/Agent-Python%203-blue?style=flat-square)](agent/)

</div>

---

## Why Claude Buddy?

Spending hours coding with Claude Code is productive — but invisible. You don't know how much of your monthly quota you've burned, how many sessions are running in parallel, or when you're approaching the limit without opening a web dashboard.

**Claude Buddy** solves this with a physical device that sits on your desk. A virtual mascot — **Claw'd** — lives on the display and reflects your session state in real time: relaxed when everything is fine, tired when the quota is high, exhausted when you're close to the cap. No browser needed. No distractions.

It's a Tamagotchi for your AI agent.

---

## How it works

```
┌─────────────────────────────────────────────────────────┐
│  Your computer                                           │
│                                                          │
│  python agent/claude_agent.py  ──►  HTTP :8266          │
│         │                                                │
│         ├── detects claude processes (pgrep)             │
│         └── reads ~/.claude/settings.json (quota)        │
└────────────────────────┬────────────────────────────────┘
                         │ WiFi (same network)
┌────────────────────────▼────────────────────────────────┐
│  ESP32-S3 + ILI9341 240×320                              │
│                                                          │
│  ┌──────────┐  ┌────────────────┐  ┌────────────────┐  │
│  │  Idle    │  │   Data screen  │  │    Settings    │  │
│  │  Claw'd  │  │  quota+sessions│  │  WiFi / agent  │  │
│  │ animated │  │  battery+WiFi  │  │  alert thresh  │  │
│  └──────────┘  └────────────────┘  └────────────────┘  │
└─────────────────────────────────────────────────────────┘
```

---

## Mascot states

| State | Quota | Sessions | Visual |
|--------|-------|---------|--------|
| **RADIANT** | < 30% | ≥ 1 | Bright orb, golden particles |
| **CONTENT** | 30–50% | ≥ 1 | Calm movement |
| **NORMAL** | 50–65% | ≥ 1 | Steady |
| **TIRED** | 65–80% | ≥ 1 | Slower, cooler tone |
| **EXHAUSTED** | > 80% | ≥ 1 | Slow pulse, Zzz |
| **OFF** | — | 0 | Dim |
| **SEARCHING** | — | — | WiFi disconnected |
| **CONFUSED** | — | — | Agent offline |

---

## Hardware

| Component | Model |
|---|---|
| Microcontroller | ESP32-S3 DevKit-C1 (N16R8 recommended) |
| Display | ILI9341 240×320 SPI with XPT2046 touch |
| Buzzer | Passive piezo |
| Battery (optional) | Li-Po 3.7V with 100kΩ + 100kΩ voltage divider |

### Wiring

| ESP32-S3 | Display | Function |
|---|---|---|
| GPIO 11 | MOSI / T_DIN | SPI data (display + touch) |
| GPIO 18 | SCK / T_CLK | SPI clock (display + touch) |
| GPIO 13 | MISO / T_DO | SPI return (display + touch) |
| GPIO 10 | CS | Display chip select |
| GPIO 14 | DC | Data / Command |
| GPIO 9 | RST | Reset |
| GPIO 48 | LED / BL | Backlight |
| GPIO 12 | T_CS | Touch chip select |
| GPIO 21 | T_IRQ | Touch interrupt |
| GPIO 47 | Piezo (+) | Buzzer |
| GPIO 1 | Voltage divider | Battery ADC |

> ⚡ The ILI9341 module runs at **3.3V**. Touch and display share SCK/MOSI/MISO — only the CS pins differ.

---

## Project structure

```
claude-buddy/
│
├── agent/
│   ├── claude_agent.py       # HTTP server — detects sessions and quota
│   └── test_claude_agent.py  # 7 unit tests
│
├── firmware/
│   ├── platformio.ini        # Envs: esp32s3, esp32s3-demo-web, esp32s3-debug
│   └── src/
│       ├── main.cpp          # Main loop, WiFiManager, NVS settings
│       ├── config.h          # Pins, timings, defaults
│       ├── colors.h          # RGB565 palette (Claude brand)
│       ├── api_client.h/cpp  # HTTP GET /status → ClaudeStatus
│       ├── display_manager.h/cpp  # Idle, data, settings, error screens
│       ├── tamagotchi.h/cpp  # Mascot, states, 64×64 sprite animation
│       ├── touch_handler.h/cpp   # TAP / LONG_PRESS events
│       ├── alert.h/cpp       # Threshold crossing → piezo melody
│       ├── battery.h/cpp     # ADC → Li-Po percentage
│       └── debug_scenarios.h # 7 hardcoded mock scenarios for debug mode
│
├── demo/
│   ├── mock_agent.py         # Mock HTTP server — auto-cycle + /control endpoint
│   ├── scenarios.json        # 7 scenario payloads
│   ├── diagram.json          # Wokwi circuit (ESP32-S3 + ILI9341 + piezo)
│   ├── wokwi.toml            # Wokwi VS Code config (localhost)
│   ├── wokwi-web.toml        # Wokwi web config (Railway URL)
│   ├── Procfile              # Railway deployment
│   └── requirements.txt      # Python stdlib only
│
└── docs/
    ├── mascot-beach.gif      # Claw'd animation above
    └── gen_mascot_gif.py     # Script that generated the GIF
```

---

## Setup — Agent (PC)

The agent is a dependency-free Python HTTP server that runs on your computer.

```bash
# Start the real agent
python3 agent/claude_agent.py

# Run tests
python3 -m pytest agent/ -v
```

The agent detects `claude` processes via `pgrep` and reads quota from `~/.claude/settings.json`. Serves on port **8266**.

---

## Setup — Firmware

### Prerequisites

```bash
pip install platformio
```

### Build and flash

```bash
cd firmware

# Production firmware (connects to real agent)
pio run -e esp32s3 -t upload

# Serial monitor
pio device monitor
```

### Configure WiFi and agent

On **first boot**, the device creates a hotspot called `ClaudeTamagotchi`. Connect to it and open `192.168.4.1` to configure:

- **WiFi SSID / Password** — your home network
- **Agent Host** — your computer's IP on the network (e.g. `192.168.1.100`)
- **Agent Port** — `8266` (default)
- **Alert threshold** — quota percentage to trigger the alert (default: 20%)

Settings are saved to NVS and survive reboots. To reconfigure: **long press** → Settings → **long press again** → opens the captive portal.

---

## Controls

| Gesture | Idle screen | Data screen | Settings |
|---|---|---|---|
| **Tap** | Opens data screen | Stays on data | Returns to Idle |
| **Long press** | Opens Settings | Opens Settings | Opens WiFi portal |

The data screen returns to Idle automatically after **10 seconds** of inactivity.

---

## Debug mode (no WiFi, real hardware)

Test all visual states on real hardware without WiFi or an agent:

```bash
cd firmware
pio run -e esp32s3-debug -t upload
```

The device shows `[DEBUG MODE]` on boot and opens directly on the data screen with hardcoded scenarios:

- **Tap** → next scenario
- **Long press** → previous scenario
- **Auto-advances** every 10 seconds

Sequence: `RADIANT → CONTENT → NORMAL → TIRED → EXHAUSTED → OFFLINE → CONFUSED → (repeat)`

---

## Demo mode (Wokwi + Railway)

Simulate the firmware without physical hardware.

### 1. Start the mock agent

```bash
# Auto-cycle (30s per scenario)
python3 demo/mock_agent.py

# Lock to a specific state
python3 demo/mock_agent.py --scenario tired

# Custom interval
python3 demo/mock_agent.py --interval 10
```

### 2. Manual control via HTTP

```bash
# Force a state
curl "http://127.0.0.1:8266/control?scenario=exhausted"

# List all scenarios
curl "http://127.0.0.1:8266/scenarios"
```

Available scenarios: `radiant`, `content`, `normal`, `tired`, `exhausted`, `offline`, `error`

### 3. Simulate with Wokwi (VS Code)

```bash
cd firmware && pio run -e esp32s3
```

Install the [Wokwi VS Code extension](https://marketplace.visualstudio.com/items?itemName=wokwi.wokwi-vscode) and press `F1 → Wokwi: Start Simulator`. The virtual WiFi reaches `mock_agent.py` at `localhost:8266`.

### 4. Simulate with public mock (Railway)

The mock agent is deployed at:
```
https://claude-tamagotchi-mock-production.up.railway.app
```

```bash
# Check it's running
curl https://claude-tamagotchi-mock-production.up.railway.app/scenarios

# Force a state
curl "https://claude-tamagotchi-mock-production.up.railway.app/control?scenario=exhausted"

# Flash firmware pointing to Railway
cd firmware && pio run -e esp32s3-demo-web -t upload
```

---

## Star History

[![Star History Chart](https://api.star-history.com/svg?repos=izrafilcst/claude-buddy&type=Date)](https://star-history.com/#izrafilcst/claude-buddy&Date)

---

## License

MIT — see [LICENSE](LICENSE).

---

<div align="center">
  <sub>Built with ☕ and many Claude Code sessions monitored by Claude Buddy itself.</sub>
</div>
