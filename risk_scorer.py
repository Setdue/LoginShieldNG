def calculate_risk(failed_logins, accounts_targeted):
    score = 0

    if failed_logins >= 3:
        score += 40

    if failed_logins >= 5:
        score += 20

    if accounts_targeted >= 3:
        score += 40

    if score >= 70:
        level = "HIGH"
    elif score >= 40:
        level = "MEDIUM"
    else:
        level = "LOW"

    return score, level


# Test with simulated activity
tests = [
    {"name": "Normal activity", "failures": 1, "accounts": 1},
    {"name": "Repeated login failures", "failures": 3, "accounts": 1},
    {"name": "Multiple accounts targeted", "failures": 5, "accounts": 3},
]

print("=== LOGINSHIELD NG RISK SCORER ===")

for test in tests:
    score, level = calculate_risk(
        test["failures"],
        test["accounts"]
    )

    print(f"\nScenario: {test['name']}")
    print(f"Risk Score: {score}/100")
    print(f"Risk Level: {level}")
