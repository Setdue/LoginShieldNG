
from datetime import datetime, timedelta
from collections import defaultdict

FAILED_LOGIN_THRESHOLD = 3
TIME_WINDOW_SECONDS = 60

login_events = [
    {"username": "user_01", "status": "failed", "ip": "192.0.2.10",
     "time": "2026-10-26 10:00:00"},
    {"username": "user_01", "status": "failed", "ip": "192.0.2.10",
     "time": "2026-10-26 10:00:10"},
    {"username": "user_01", "status": "failed", "ip": "192.0.2.10",
     "time": "2026-10-26 10:00:20"},
    {"username": "user_02", "status": "failed", "ip": "192.0.2.10",
     "time": "2026-10-26 10:00:25"},
    {"username": "user_03", "status": "failed", "ip": "192.0.2.10",
     "time": "2026-10-26 10:00:30"},
    {"username": "user_04", "status": "success", "ip": "192.0.2.20",
     "time": "2026-10-26 10:00:40"},
]

failed_by_user = defaultdict(list)
failed_by_ip = defaultdict(list)

for event in login_events:
    if event["status"] == "failed":
        event_time = datetime.strptime(
            event["time"], "%Y-%m-%d %H:%M:%S"
        )

        failed_by_user[event["username"]].append(event_time)
        failed_by_ip[event["ip"]].append(
            (event["username"], event_time)
        )

print("=== LoginShield NG Threat Detector ===")

# Detect repeated failures against one account
for username, times in failed_by_user.items():
    if len(times) >= FAILED_LOGIN_THRESHOLD:
        print(
            f"[HIGH RISK] Possible brute-force activity "
            f"against {username}: {len(times)} failed logins."
        )

# Detect failures across multiple accounts from one IP
for ip, attempts in failed_by_ip.items():
    unique_users = {username for username, _ in attempts}

    if len(unique_users) >= FAILED_LOGIN_THRESHOLD:
        print(
            f"[WARNING] Possible password spraying: "
            f"{len(unique_users)} accounts targeted from {ip}."
        )

print("=== Scan Complete ===")
