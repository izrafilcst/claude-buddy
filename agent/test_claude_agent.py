import json
import os
import threading
import unittest
from http.server import HTTPServer
from unittest.mock import patch
from urllib.request import urlopen


class TestDetectSessions(unittest.TestCase):
    def _import(self):
        from claude_agent import detect_sessions
        return detect_sessions

    @patch("claude_agent.subprocess.run")
    @patch("claude_agent._get_process_uptime", return_value=10)
    def test_detects_claude_processes(self, mock_uptime, mock_run):
        mock_run.return_value = type(
            "R", (), {"returncode": 0, "stdout": "12345 /usr/bin/claude --flag\n"}
        )()
        sessions = self._import()()
        self.assertEqual(len(sessions), 1)
        self.assertEqual(sessions[0]["pid"], 12345)
        self.assertEqual(sessions[0]["uptime_min"], 10)

    @patch("claude_agent.subprocess.run")
    def test_filters_own_process(self, mock_run):
        mock_run.return_value = type(
            "R", (), {"returncode": 0, "stdout": "99999 python claude_agent.py\n"}
        )()
        sessions = self._import()()
        self.assertEqual(len(sessions), 0)

    @patch("claude_agent.subprocess.run")
    def test_returns_empty_when_no_processes(self, mock_run):
        mock_run.return_value = type("R", (), {"returncode": 1, "stdout": ""})()
        sessions = self._import()()
        self.assertEqual(len(sessions), 0)

    @patch("claude_agent.subprocess.run")
    @patch("claude_agent._get_process_uptime", return_value=5)
    def test_filters_pgrep_itself(self, mock_uptime, mock_run):
        mock_run.return_value = type(
            "R",
            (),
            {
                "returncode": 0,
                "stdout": "111 pgrep -af claude\n222 /usr/bin/claude\n",
            },
        )()
        sessions = self._import()()
        self.assertEqual(len(sessions), 1)
        self.assertEqual(sessions[0]["pid"], 222)


class TestBuildStatus(unittest.TestCase):
    @patch(
        "claude_agent.detect_sessions",
        return_value=[{"pid": 123, "uptime_min": 5}],
    )
    @patch(
        "claude_agent.estimate_quota",
        return_value={"quota_percent": 62, "plan": "max", "reset_minutes": 134},
    )
    def test_builds_complete_response(self, mock_quota, mock_sessions):
        from claude_agent import build_status

        status = build_status()
        self.assertEqual(status["quota_percent"], 62)
        self.assertEqual(status["sessions_active"], 1)
        self.assertEqual(len(status["sessions_details"]), 1)
        self.assertEqual(status["plan"], "max")
        self.assertEqual(status["reset_minutes"], 134)
        self.assertIn("timestamp", status)


class TestHTTPEndpoint(unittest.TestCase):
    PORT = 18266

    @classmethod
    def setUpClass(cls):
        from claude_agent import StatusHandler

        cls.server = HTTPServer(("127.0.0.1", cls.PORT), StatusHandler)
        cls.thread = threading.Thread(target=cls.server.serve_forever)
        cls.thread.daemon = True
        cls.thread.start()

    @classmethod
    def tearDownClass(cls):
        cls.server.shutdown()

    @patch(
        "claude_agent.build_status",
        return_value={
            "quota_percent": 50,
            "sessions_active": 2,
            "sessions_details": [],
            "reset_minutes": 100,
            "plan": "max",
            "timestamp": 1718000000,
        },
    )
    def test_status_returns_json(self, mock_status):
        resp = urlopen(f"http://127.0.0.1:{self.PORT}/status")
        self.assertEqual(resp.status, 200)
        data = json.loads(resp.read())
        self.assertEqual(data["quota_percent"], 50)
        self.assertEqual(data["sessions_active"], 2)

    def test_unknown_path_returns_404(self):
        from urllib.error import HTTPError

        with self.assertRaises(HTTPError) as ctx:
            urlopen(f"http://127.0.0.1:{self.PORT}/unknown")
        self.assertEqual(ctx.exception.code, 404)


if __name__ == "__main__":
    unittest.main()
