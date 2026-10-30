# LoginShield NG

Hi, I'm **Setdue**, the creator of LoginShield NG. I built this project to explore a clearer, more visual way to review login activity and spot patterns that may deserve attention. It grew from an earlier Python prototype into a Windows desktop interface, alongside a standalone browser dashboard.

LoginShield NG is a prototype for reviewing synthetic login events. The main interface is written in C++ with the Windows API.

## What it does

- Replays synthetic login events in a live demo.
- Shows per-device health bars. Failed attempts and account/IP spread affect a demonstration signal score, which also drives the particle effects.
- Highlights IP correlations and account-targeting patterns for human review.
- Imports event data from CSV and exports a text report.
- Includes a standalone browser dashboard and an earlier Python prototype.

I made this as a learning and interface-design project: I wanted to make event patterns easier to see and explore, while keeping the analysis understandable. The score is a demonstration heuristic, not a reliable identity signal. IP addresses can be shared by many people and devices. Alerts are for review; the program does not block accounts or IP addresses and is not connected to a live login service.

## C++ desktop app

### Build on Windows

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
