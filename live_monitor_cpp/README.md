# LoginShield NG live-monitor C++ desktop app

A native Win32/GDI C++17 companion in the same visual style as the existing LoginShield NG dashboard. It reads the local Windows Security log for failed-logon Event ID 4625 and shows a passive review list with a repeated-attempt signal.

## Build and run

- Open `LoginShieldLiveMonitor.cbp` in Code::Blocks and choose **Build → Build and Run**, or run `build_windows.bat` from a MinGW-w64/Code::Blocks command prompt.
- Run normally to query the local Security log. Use `LoginShieldLiveMonitor.exe --demo` to show synthetic sample events without reading the Windows log.
- The signed-in Windows account must already be permitted to read the Security log, and Windows auditing must record failed logons. The app reports an access issue and does not change permissions or audit policy.

## Alerts, privacy, and limits

The app flags 10 or more recent attempts for one account or source IP in a five-minute window. This is a review prompt, not a finding of malicious activity. It reads at most 250 recent events every 15 seconds and keeps event details in memory only. No events are sent to a service or written to a new database. It never blocks accounts, changes firewall rules, clears logs, or modifies Windows settings.

This is a local single-PC tool, not a fleet monitor or a replacement for a SIEM. It is separate from the existing LoginShield C++ demo dashboard and from the Python live-monitor companion.
