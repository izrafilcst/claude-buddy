import json
import os
import subprocess
import time
from http.server import BaseHTTPRequestHandler, HTTPServer
from pathlib import Path

PORT = 8266


def _get_process_uptime(pid):
    try:
        with open(f"/proc/{pid}/stat") as f:
            starttime_ticks = int(f.read().split()[21])
        with open("/proc/uptime") as f:
            system_uptime = float(f.read().split()[0])
        clk_tck = os.sysconf(os.sysconf_names["SC_CLK_TCK"])
        uptime_sec = system_uptime - (starttime_ticks / clk_tck)
        return max(0, int(uptime_sec / 60))
    except (FileNotFoundError, IndexError, ValueError, KeyError):
        return 0


def detect_sessions():
    try:
        result = subprocess.run(
            ["pgrep", "-af", "claude"], capture_output=True, text=True, timeout=5
        )
        if result.returncode != 0:
            return []
    except (subprocess.TimeoutExpired, FileNotFoundError):
        return []

    skip = ("claude_agent", "test_claude_agent", "pgrep")
    sessions = []
    for line in result.stdout.strip().split("\n"):
        if not line:
            continue
        parts = line.split(None, 1)
        if len(parts) < 2:
            continue
        pid_str, cmdline = parts
        if any(s in cmdline for s in skip):
            continue
        try:
            pid = int(pid_str)
        except ValueError:
            continue
        sessions.append({"pid": pid, "uptime_min": _get_process_uptime(pid)})
    return sessions


def estimate_quota():
    result = {"quota_percent": -1, "plan": "unknown", "reset_minutes": -1}
    settings_path = Path.home() / ".claude" / "settings.json"
    if settings_path.exists():
        try:
            with open(settings_path) as f:
                settings = json.load(f)
            if "plan" in settings:
                result["plan"] = settings["plan"]
        except (json.JSONDecodeError, OSError):
            pass
    return result


def build_status():
    sessions = detect_sessions()
    quota = estimate_quota()
    return {
        "quota_percent": quota["quota_percent"],
        "sessions_active": len(sessions),
        "sessions_details": sessions,
        "reset_minutes": quota["reset_minutes"],
        "plan": quota["plan"],
        "timestamp": int(time.time()),
    }


class StatusHandler(BaseHTTPRequestHandler):
    def do_GET(self):
        if self.path == "/status":
            body = json.dumps(build_status()).encode()
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.send_header("Content-Length", str(len(body)))
            self.end_headers()
            self.wfile.write(body)
        else:
            self.send_error(404)

    def log_message(self, format, *args):
        pass


def main():
    server = HTTPServer(("0.0.0.0", PORT), StatusHandler)
    print(f"Claude Agent listening on :{PORT}")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass
    server.server_close()


if __name__ == "__main__":
    main()
