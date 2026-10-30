from datetime import datetime, timedelta
from collections import defaultdict
from risk_scorer import calculate_risk

THRESHOLD = 3
WINDOW = timedelta(seconds=60)

events = [
    {"user": "user_01", "status": "failed", "ip": "192.0.2.10", "time": "10:00:00"},
    {"user": "user_01", "status": "failed", "ip": "192.0.2.10", "time": "10:00:10"},
    {"user": "user_01", "status": "failed", "ip": "192.0.2.10", "time": "10:00:20"},
    {"user": "user_02", "status": "failed", "ip": "192.0.2.10", "time": "10:00:25"},
    {"user": "user_03", "status": "failed", "ip": "192.0.2.10", "time": "10:00:30"},
    {"user": "user_04", "status": "success", "ip": "192.0.2.20", "time": "10:00:40"},
]

user_failures = defaultdict(list)
ip_failures = defaultdict(list)

for event in events:
    if event["status"] == "failed":
        timestamp = datetime.strptime(event["time"], "%H:%M:%S")
        user_failures[event["user"]].append(timestamp)
        ip_failures[event["ip"]].append((event["user"], timestamp))

print("=" * 45)
print("       LOGINSHIELD NG SECURITY REPORT")
print("=" * 45)

print(f"\nTotal events: {len(events)}")
print(f"Failed logins: {sum(e['status'] == 'failed' for e in events)}")
print(f"Successful logins: {sum(e['status'] == 'success' for e in events)}")

alerts = 0

print("\n--- BRUTE-FORCE DETECTION ---")

for user, times in user_failures.items():
    times.sort()

    for start in range(len(times)):
        end = start

        while end < len(times) and times[end] - times[start] <= WINDOW:
            end += 1

        attempts = end - start

        if attempts >= THRESHOLD:
            print(f"[HIGH RISK] {user}: {attempts} failed logins.")
            alerts += 1
            break

print("\n--- PASSWORD-SPRAYING DETECTION ---")

for ip, attempts in ip_failures.items():
    attempts.sort(key=lambda item: item[1])

    for start in range(len(attempts)):
        end = start
        targeted_users = set()

        while end < len(attempts) and attempts[end][1] - attempts[start][1] <= WINDOW:
            targeted_users.add(attempts[end][0])
            end += 1

        if len(targeted_users) >= THRESHOLD:
            print(f"[WARNING] {len(targeted_users)} accounts targeted from {ip}.")
            alerts += 1
            break

print("\n--- RISK ASSESSMENT ---")

failed_count = sum(e["status"] == "failed" for e in events)
accounts_targeted = max(
    (len({user for user, _ in attempts}) for attempts in ip_failures.values()),
    default=0
)

score, level = calculate_risk(failed_count, accounts_targeted)

print(f"Risk Score: {score}/100")
print(f"Risk Level: {level}")

print("\n--- FINAL SUMMARY ---")
print(f"Alerts generated: {alerts}")
print("Scan complete.")

with open("security_report.txt", "w", encoding="utf-8") as report:
    report.write("LOGINSHIELD NG SECURITY REPORT\n")
    report.write("=" * 35 + "\n")
    report.write(f"Total events: {len(events)}\n")
    report.write(f"Failed logins: {failed_count}\n")
    report.write(f"Risk Score: {score}/100\n")
    report.write(f"Risk Level: {level}\n")
    report.write(f"Alerts generated: {alerts}\n")
    report.write("Scan complete.\n")

print("Report saved to security_report.txt")
