#!/usr/bin/env python3
import argparse
import json
import os
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

        def do_OPTIONS(self):
            self.send_response(204)
            self._cors()
            self.end_headers()

        def do_GET(self):
            parsed = urlparse(self.path)
            params = parse_qs(parsed.query)

            if parsed.path == "/status":
                scenario = state.get()
                if scenario == "error":
                    self.send_response(500)
                    self._cors()
                    self.end_headers()
                    return
                self._reply(200, json.dumps(state.scenarios[scenario]).encode())

            elif parsed.path == "/control":
                name = params.get("scenario", [None])[0]
                valid = list(state.scenarios.keys()) + ["error"]
                if name not in valid:
                    self._reply(400, json.dumps({"ok": False, "error": f"unknown scenario '{name}'"}).encode())
                    return
                state.set(name)
                self._reply(200, json.dumps({"ok": True, "scenario": name}).encode())

            elif parsed.path == "/scenarios":
                names = list(state.scenarios.keys()) + ["error"]
                self._reply(200, json.dumps({"scenarios": names}).encode())

            else:
                self.send_response(404)
                self.end_headers()

        def _cors(self):
            self.send_header("Access-Control-Allow-Origin", "*")
            self.send_header("Access-Control-Allow-Methods", "GET, OPTIONS")
            self.send_header("Access-Control-Allow-Headers", "Content-Type")

        def _reply(self, code, body: bytes):
            self.send_response(code)
            self.send_header("Content-Type", "application/json")
            self.send_header("Content-Length", str(len(body)))
            self._cors()
            self.end_headers()
            self.wfile.write(body)

    return Handler


def main():
    parser = argparse.ArgumentParser(description="Claude Tamagotchi mock agent")
    parser.add_argument("--interval", type=int, default=30,
                        help="seconds per scenario in auto-cycle (default: 30)")
    parser.add_argument("--scenario", default=None,
                        help="lock to a specific scenario, disables auto-cycle")
    parser.add_argument("--host", default=os.environ.get("HOST", "127.0.0.1"),
                        help="bind host (default: 127.0.0.1, use 0.0.0.0 for cloud)")
    parser.add_argument("--port", type=int, default=int(os.environ.get("PORT", PORT)),
                        help="bind port (default: 8266, Railway sets PORT env var)")
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

    print(f"[mock] listening on http://{args.host}:{args.port}")
    print(f"[mock] control: curl 'http://{args.host}:{args.port}/control?scenario=tired'")

    server = HTTPServer((args.host, args.port), make_handler(state))
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\n[mock] stopped")


if __name__ == "__main__":
    main()
