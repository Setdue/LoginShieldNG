# LoginShield NG

LoginShield NG is a Windows desktop prototype for reviewing synthetic login events. Its main interface is written in C++ with the Windows API. A standalone browser dashboard and the earlier Python prototype are included too.

## C++ desktop app

The desktop UI includes:

- A live demo that replays synthetic login events.
- Per-device health bars. Failed attempts and account/IP spread increase the device signal score; particle count and speed rise with that score.
- IP correlation and account-targeting review alerts.
- CSV import and text report export.
- A painterly cosmic visual treatment with a vortex emblem and curved, luminous trails.

The device score is a demonstration heuristic. It is not a reliable identity signal: an IP address can be shared by many people and devices. Alerts are for review; the program does not block accounts or IP addresses.

## Build on Windows

1. Open `LoginShieldNG.cbp` in Code::Blocks.
2. Choose **Build → Build and Run**.

The project uses C++17, MinGW, and Windows system libraries. It does not need Python, Qt, or additional UI packages.

The editable desktop UI source is `loginshield_ui.cpp`. See `analyze`, `drawDeviceHealthBar`, `drawPainterlyWash`, and `drawVortexPainting` to adjust the score and visuals.

## CSV format

Use **Import CSV** in the desktop app. Required columns are:

```csv
time,username,ip,device,status
10:00:00,user_01,192.0.2.10,device_A,failed
```

Time format is `HH:MM:SS`. Status must be `failed` or `success`. `demo_events.csv` contains a synthetic sample. The app analyzes the latest 60 seconds of the supplied event sequence.

## Other project files

- `dashboard.html` is the standalone browser dashboard; it has demo data, CSV import, event filters, and report export.
- `event_processor.py`, `login_detector.py`, `main.py`, `risk_scorer.py`, `test_detector.py`, and `threat_detector.py` are earlier Python prototype files; they are not needed to build the C++ app.

Generated reports, compiled executables, Code::Blocks build folders, and the empty `live_monitor.py` placeholder are left out of the source bundle.

All sample events use documentation-only IP addresses. Use only logs you are authorized to review.

