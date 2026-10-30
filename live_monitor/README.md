# LoginShield NG — Live Windows monitor

An optional Windows companion for the existing LoginShield NG prototype. It reads local failed-logon audit events (Security Event ID 4625), displays recent records, and flags repeated attempts in a five-minute window. It does not replace the existing app.

## Run

- Demo mode, with synthetic events only: `python live_monitor.py --demo`
- Live local Windows Security log: `python live_monitor.py`
- Parser self-check: `python live_monitor.py --self-test`

No third-party Python packages are used. The live reader calls the built-in Windows `wevtutil` utility. The signed-in Windows account must already be allowed to read the Security event log. If it is not, the app reports an access error; it does not elevate itself or alter permissions. Windows audit policy must also be configured to record failed logons.

## Alerts and data

This initial rule flags 10 or more failed attempts for one non-local source IP or account within five minutes. It is a review signal, not proof of an attack: shared NAT addresses, mistyped passwords, background services, and normal account activity can trigger it. Event fields are stored only in `%LOCALAPPDATA%\LoginShieldNG\LiveMonitor\events.sqlite3` and pruned after 30 days. They can include account names and source IP addresses, so protect the Windows account and its backups.

## Safety boundary

This monitor is read-only. It never blocks logins, locks accounts, changes firewall rules, clears event logs, or uploads telemetry. Do not use an alert alone to take action against a user. Validate the source, account, logon type, status/substatus, and surrounding context first.

## Scope

This is a local single-computer monitoring tool, not an enterprise SIEM or an incident-response guarantee. It queries a bounded set of recent events every 15 seconds; it may miss events if the Security log rolls over between checks or auditing is disabled. Remote fleet collection, alert delivery, configurable rules, packaging, and code signing are not included.
