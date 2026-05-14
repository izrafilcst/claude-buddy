# Wokwi Demo Mode Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Create a `demo/` folder with Wokwi circuit definition, mock agent server, and scenario payloads so the Claude Tamagotchi Monitor firmware runs in the Wokwi VS Code simulator without physical hardware.

**Architecture:** `mock_agent.py` serves the same JSON format as the real agent on port 8266, cycling through 7 predefined scenarios automatically and accepting manual scenario changes via `GET /control?scenario=<name>`. Wokwi reads `wokwi.toml` to load the compiled `.elf` and simulates ESP32-S3 with a virtual ILI9341 display whose WiFi routes to localhost. No firmware changes required.

**Tech Stack:** Python 3 stdlib (`http.server`, `threading`, `json`, `argparse`), Wokwi VS Code extension, PlatformIO (compile firmware before simulating)

---

### Task 1: `demo/scenarios.json` — Mock data payloads

**Files:**
- Create: `demo/scenarios.json`

- [ ] **Step 1: Create `demo/scenarios.json`**

Fields match exactly what `firmware/src/api_client.cpp` parses (lines 35–48): `quota_percent`, `sessions_active`, `sessions_details[].pid`, `sessions_details[].uptime_min`, `reset_minutes`, `plan`.

```json
{
  "radiant":   { "quota_percent": 10, "sessions_active": 1, "sessions_details": [{"pid": 1001, "uptime_min": 45}],  "reset_minutes": 320, "plan": "pro" },
  "content":   { "quota_percent": 40, "sessions_active": 2, "sessions_details": [{"pid": 1001, "uptime_min": 120}, {"pid": 1002, "uptime_min": 20}], "reset_minutes": 200, "plan": "pro" },
  "normal":    { "quota_percent": 58, "sessions_active": 1, "sessions_details": [{"pid": 1003, "uptime_min": 90}],  "reset_minutes": 130, "plan": "pro" },
  "tired":     { "quota_percent": 72, "sessions_active": 1, "sessions_details": [{"pid": 1004, "uptime_min": 240}], "reset_minutes": 80,  "plan": "pro" },
  "exhausted": { "quota_percent": 88, "sessions_active": 3, "sessions_details": [{"pid": 1005, "uptime_min": 480}, {"pid": 1006, "uptime_min": 60}, {"pid": 1007, "uptime_min": 15}], "reset_minutes": 20, "plan": "pro" },
  "offline":   { "quota_percent": -1, "sessions_active": 0, "sessions_details": [], "reset_minutes": -1, "plan": "unknown" }
}
```

- [ ] **Step 2: Verify JSON is valid**

```bash
python3 -c "import json; data = json.load(open('demo/scenarios.json')); print(list(data.keys()))"
```

Expected: `['radiant', 'content', 'normal', 'tired', 'exhausted', 'offline']`

- [ ] **Step 3: Commit**

```bash
git add demo/scenarios.json
git commit -m "feat(demo): add mock scenario payloads"
```

---

### Task 2: `demo/mock_agent.py` — HTTP mock server

**Files:**
- Create: `demo/mock_agent.py`

- [ ] **Step 1: Create `demo/mock_agent.py`**

```python
#!/usr/bin/env python3
import argparse
import json
import threading
from http.server import BaseHTTPRequestHandler, HTTPServer
from pathlib import Path
from urllib.parse import parse_qs, urlparse

SCENARIOS_FILE = Path(__file__).parent / "scenarios.json"
CYCLE_ORDER = ["radiant", "content", "normal", "tired", "exhausted", "offline", "error"]
PORT = 8266


def load_scenarios():
    with open(SCENARIOS_FILE) as f:
        return json.load(f)


class MockState:
    def __init__(self, scenarios, initial, interval):
        self.scenarios = scenarios
        self.current = initial
        self.interval = interval
        self._lock = threading.Lock()
        self._event = threading.Event()

    def set(self, name):
        with self._lock:
            self.current = name
        self._event.set()  # interrupt current sleep, reset timer
        print(f"[mock] scenario={name} (manual)")

    def get(self):
        with self._lock:
            return self.current

    def advance(self):
        with self._lock:
            idx = (CYCLE_ORDER.index(self.current) + 1) % len(CYCLE_ORDER)
            self.current = CYCLE_ORDER[idx]
            name = self.current
        print(f"[mock] scenario={name} (auto)")

    def cycle_loop(self):
        while True:
            interrupted = self._event.wait(timeout=self.interval)
            self._event.clear()
            if not interrupted:
                self.advance()


def make_handler(state):
    class Handler(BaseHTTPRequestHandler):
        def log_message(self, fmt, *args):
            pass  # suppress default per-request log

        def do_GET(self):
            parsed = urlparse(self.path)
            params = parse_qs(parsed.query)

            if parsed.path == "/status":
                scenario = state.get()
                if scenario == "error":
                    self.send_response(500)
                    self.end_headers()
                    return
                payload = json.dumps(state.scenarios[scenario]).encode()
                self.send_response(200)
                self.send_header("Content-Type", "application/json")
                self.send_header("Content-Length", str(len(payload)))
                self.end_headers()
                self.wfile.write(payload)

            elif parsed.path == "/control":
                name = params.get("scenario", [None])[0]
                valid = list(state.scenarios.keys()) + ["error"]
                if name not in valid:
                    body = json.dumps({"ok": False, "error": f"unknown scenario '{name}'"}).encode()
                    self.send_response(400)
                    self.send_header("Content-Type", "application/json")
                    self.send_header("Content-Length", str(len(body)))
                    self.end_headers()
                    self.wfile.write(body)
                    return
                state.set(name)
                body = json.dumps({"ok": True, "scenario": name}).encode()
                self.send_response(200)
                self.send_header("Content-Type", "application/json")
                self.send_header("Content-Length", str(len(body)))
                self.end_headers()
                self.wfile.write(body)

            elif parsed.path == "/scenarios":
                names = list(state.scenarios.keys()) + ["error"]
                body = json.dumps({"scenarios": names}).encode()
                self.send_response(200)
                self.send_header("Content-Type", "application/json")
                self.send_header("Content-Length", str(len(body)))
                self.end_headers()
                self.wfile.write(body)

            else:
                self.send_response(404)
                self.end_headers()

    return Handler


def main():
    parser = argparse.ArgumentParser(description="Claude Tamagotchi mock agent")
    parser.add_argument("--interval", type=int, default=30,
                        help="seconds per scenario in auto-cycle (default: 30)")
    parser.add_argument("--scenario", default=None,
                        help="lock to a specific scenario, disables auto-cycle")
    args = parser.parse_args()

    scenarios = load_scenarios()
    valid = list(scenarios.keys()) + ["error"]

    initial = args.scenario if args.scenario else CYCLE_ORDER[0]
    if initial not in valid:
        parser.error(f"unknown scenario '{initial}'. Valid: {valid}")

    state = MockState(scenarios, initial, args.interval)

    if args.scenario is None:
        t = threading.Thread(target=state.cycle_loop, daemon=True)
        t.start()
        print(f"[mock] auto-cycle every {args.interval}s | starting at '{initial}'")
    else:
        print(f"[mock] fixed scenario='{initial}'")

    print(f"[mock] listening on http://127.0.0.1:{PORT}")
    print(f"[mock] control: curl 'http://127.0.0.1:{PORT}/control?scenario=tired'")

    server = HTTPServer(("127.0.0.1", PORT), make_handler(state))
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\n[mock] stopped")


if __name__ == "__main__":
    main()
```

- [ ] **Step 2: Verify server starts and `/status` responds**

```bash
python3 demo/mock_agent.py --scenario radiant &
MOCK_PID=$!
sleep 1
curl -s http://127.0.0.1:8266/status | python3 -m json.tool
kill $MOCK_PID
```

Expected: JSON with `"quota_percent": 10`, `"sessions_active": 1`.

- [ ] **Step 3: Verify `/control` switches scenario**

```bash
python3 demo/mock_agent.py &
MOCK_PID=$!
sleep 1
curl -s "http://127.0.0.1:8266/control?scenario=exhausted"
curl -s http://127.0.0.1:8266/status | python3 -c "import sys,json; d=json.load(sys.stdin); print('quota:', d['quota_percent'])"
kill $MOCK_PID
```

Expected: first curl returns `{"ok": true, "scenario": "exhausted"}`, second prints `quota: 88`.

- [ ] **Step 4: Verify `error` scenario returns HTTP 500**

```bash
python3 demo/mock_agent.py --scenario error &
MOCK_PID=$!
sleep 1
curl -s -o /dev/null -w "%{http_code}" http://127.0.0.1:8266/status
kill $MOCK_PID
```

Expected: `500`

- [ ] **Step 5: Verify `/scenarios` lists all scenarios**

```bash
python3 demo/mock_agent.py &
MOCK_PID=$!
sleep 1
curl -s http://127.0.0.1:8266/scenarios
kill $MOCK_PID
```

Expected: `{"scenarios": ["radiant", "content", "normal", "tired", "exhausted", "offline", "error"]}`

- [ ] **Step 6: Commit**

```bash
git add demo/mock_agent.py
git commit -m "feat(demo): add mock agent with auto-cycle and HTTP control"
```

---

### Task 3: `demo/diagram.json` — Wokwi circuit

**Files:**
- Create: `demo/diagram.json`

- [ ] **Step 1: Create `demo/diagram.json`**

Pinos mapeados conforme `firmware/platformio.ini` e `firmware/src/config.h`.

```json
{
  "version": 1,
  "author": "Rafael Costa",
  "editor": "wokwi",
  "parts": [
    {
      "type": "board-esp32-s3-devkitc-1",
      "id": "esp",
      "top": 0,
      "left": 0,
      "attrs": {}
    },
    {
      "type": "wokwi-ili9341",
      "id": "lcd",
      "top": -160,
      "left": 220,
      "attrs": {}
    },
    {
      "type": "wokwi-buzzer",
      "id": "bz",
      "top": 230,
      "left": 220,
      "attrs": { "volume": "0.1" }
    }
  ],
  "connections": [
    ["esp:3V3.1",  "lcd:VCC",    "red",    []],
    ["esp:GND.1",  "lcd:GND",    "black",  []],
    ["esp:GPIO11", "lcd:MOSI",   "green",  []],
    ["esp:GPIO18", "lcd:SCK",    "blue",   []],
    ["esp:GPIO13", "lcd:MISO",   "orange", []],
    ["esp:GPIO10", "lcd:CS",     "yellow", []],
    ["esp:GPIO14", "lcd:DC",     "purple", []],
    ["esp:GPIO9",  "lcd:RST",    "brown",  []],
    ["esp:GPIO48", "lcd:LED",    "white",  []],
    ["esp:GPIO18", "lcd:T_CLK",  "blue",   []],
    ["esp:GPIO12", "lcd:T_CS",   "gray",   []],
    ["esp:GPIO11", "lcd:T_DIN",  "green",  []],
    ["esp:GPIO13", "lcd:T_DO",   "orange", []],
    ["esp:GPIO21", "lcd:T_IRQ",  "pink",   []],
    ["esp:GPIO47", "bz:1",       "red",    []],
    ["esp:GND.2",  "bz:2",       "black",  []]
  ]
}
```

- [ ] **Step 2: Verify JSON is valid**

```bash
python3 -c "import json; d = json.load(open('demo/diagram.json')); print(len(d['parts']), 'parts,', len(d['connections']), 'connections')"
```

Expected: `3 parts, 16 connections`

- [ ] **Step 3: Commit**

```bash
git add demo/diagram.json
git commit -m "feat(demo): add Wokwi circuit diagram for ESP32-S3 + ILI9341 + piezo"
```

---

### Task 4: `demo/wokwi.toml` + fix `.gitignore`

**Files:**
- Create: `demo/wokwi.toml`
- Modify: `.gitignore`

- [ ] **Step 1: Create `demo/wokwi.toml`**

```toml
[wokwi]
version = 1
firmware = '../firmware/.pio/build/esp32s3/firmware.elf'
elf = '../firmware/.pio/build/esp32s3/firmware.elf'

[wifi]
ssid = "Wokwi-GUEST"
password = ""
```

A seção `[wifi]` pré-popula o NVS do Wokwi para que `WiFiManager.autoConnect()` conecte direto sem abrir portal captivo.

- [ ] **Step 2: Add `.superpowers/` to `.gitignore`**

Append to `.gitignore`:

```
# Superpowers brainstorm sessions
.superpowers/
```

- [ ] **Step 3: Commit**

```bash
git add demo/wokwi.toml .gitignore
git commit -m "feat(demo): add wokwi.toml and fix .gitignore"
```

---

## Verificação Final (manual)

```bash
# Terminal 1 — compilar firmware
cd firmware && pio run

# Terminal 2 — iniciar mock agent
python3 demo/mock_agent.py

# VS Code — iniciar simulador
# F1 → "Wokwi: Start Simulator"
# Display virtual aparece, WiFi conecta em ~5s ao mock

# Forçar estado específico
curl "http://127.0.0.1:8266/control?scenario=exhausted"

# Listar todos os cenários
curl "http://127.0.0.1:8266/scenarios"
```
