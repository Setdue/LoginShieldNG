# LoginShield NG
# Stage 1, Lesson 2: Login Detection Engine

from datetime import datetime, timedelta

# Configuration
FAILED_LOGIN_THRESHOLD = 3
TIME_WINDOW_SECONDS = 60

# Simulated login events
login_events = [
    {
        "username": "user_01",
        "status": "failed",
        "timestamp": "2026-10-26 10:00:00"
    },
    {
        "username": "user_01",
        "status": "failed",
        "timestamp": "2026-10-26 10:00:10"
    },
    {
        "username": "user_01",
        "status": "failed",
        "timestamp": "2026-10-26 10:00:20"
    },
    {
        "username": "user_02",
        "status": "success",
        "timestamp": "2026-10-26 10:00:25"
    },
    {
        "username": "user_03",
        "status": "failed",
        "timestamp": "2026-10-26 10:00:30"
    },
]

# Group failed login timestamps by account
failed_logins = {}

for event in login_events:
    if event["status"] == "failed":
        username = event["username"]
        timestamp = datetime.strptime(
            event["timestamp"], "%Y-%m-%d %H:%M:%S"
        )

        if username not in failed_logins:
            failed_logins[username] = []

        failed_logins[username].append(timestamp)

# Analyse login activity
print("LOGINSHIELD NG")
print("======================")
print("Login Security Analysis")
print()

for username, timestamps in failed_logins.items():
    latest_time = max(timestamps)

    window_start = latest_time - timedelta(
        seconds=TIME_WINDOW_SECONDS
    )

    recent_failures = [
        timestamp
        for timestamp in timestamps
        if window_start <= timestamp <= latest_time
    ]

    print(f"Account: {username}")
    print(f"Failed attempts in window: {len(recent_failures)}")

    if len(recent_failures) >= FAILED_LOGIN_THRESHOLD:
        print("ALERT: Suspicious repeated login failures!")
        print("Risk level: HIGH")
    else:
        print("Status: No alert triggered")

    print()

print("Analysis complete.")
