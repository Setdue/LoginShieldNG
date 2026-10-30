# LoginShield NG
# Stage 1, Lesson 1: Login Event Processor

login_events = [
    {"username": "user_01", "status": "failed"},
    {"username": "user_02", "status": "success"},
    {"username": "user_01", "status": "failed"},
    {"username": "user_03", "status": "failed"},
    {"username": "user_01", "status": "failed"},
    {"username": "user_02", "status": "success"},
    {"username": "user_03", "status": "failed"},
]

failed_attempts = {}

for event in login_events:
    if event["status"] == "failed":
        username = event["username"]

        if username not in failed_attempts:
            failed_attempts[username] = 0

        failed_attempts[username] += 1

print("LOGINSHIELD NG")
print("==============")
print("Failed Login Summary")
print()

for username, count in failed_attempts.items():
    print(f"Account: {username} | Failed attempts: {count}")

print()
print("Event processing complete.")
