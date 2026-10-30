
import unittest
from datetime import datetime, timedelta


def detect_brute_force(events, threshold=3, window_seconds=60):
    for user in set(event["user"] for event in events):
        failures = [
            datetime.strptime(event["time"], "%H:%M:%S")
            for event in events
            if event["user"] == user and event["status"] == "failed"
        ]

        failures.sort()

        for i in range(len(failures)):
            window_end = failures[i] + timedelta(seconds=window_seconds)
            attempts = [
                time for time in failures[i:]
                if time <= window_end
            ]

            if len(attempts) >= threshold:
                return True

    return False


class TestLoginShield(unittest.TestCase):

    def test_detects_repeated_failures(self):
        events = [
            {"user": "user_01", "status": "failed", "time": "10:00:00"},
            {"user": "user_01", "status": "failed", "time": "10:00:10"},
            {"user": "user_01", "status": "failed", "time": "10:00:20"},
        ]
        self.assertTrue(detect_brute_force(events))

    def test_ignores_too_few_failures(self):
        events = [
            {"user": "user_01", "status": "failed", "time": "10:00:00"},
            {"user": "user_01", "status": "failed", "time": "10:00:10"},
        ]
        self.assertFalse(detect_brute_force(events))

    def test_ignores_successful_logins(self):
        events = [
            {"user": "user_01", "status": "success", "time": "10:00:00"},
            {"user": "user_01", "status": "success", "time": "10:00:10"},
            {"user": "user_01", "status": "failed", "time": "10:00:20"},
            {"user": "user_03", "status": "failed", "time": "10:00:30"},
            {"user": "user_03", "status": "failed", "time": "10:00:50"},
            {"user": "user_03", "status": "failed", "time": "10:00:59"},
        ]
        self.assertFalse(detect_brute_force(events))


if __name__ == "__main__":
    unittest.main(verbosity=2)

