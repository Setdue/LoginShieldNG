# LoginShield NG

Hi, I'm **Setdue**, the creator of LoginShield NG. I built this project to explore a clearer, more visual way to review login activity and spot patterns that may deserve attention. It grew from an earlier Python prototype into a Windows desktop interface, alongside a standalone browser dashboard.

LoginShield NG is a local prototype for reviewing login events from synthetic demos or CSV files.

## What it does

- Replays a richer synthetic scenario with ordinary sign-ins, repeated failures, account spraying, and activity from multiple IPs.
- Shows login activity, correlated IPs, review alerts, signal flow, and per-device event details.
- Lets you simulate blocking accounts or source IPs during a demo replay. Simulated outcomes are labelled `BLOCKED (DEMO)` and do not affect real systems.
- Imports CSV events and exports a text report.
- Includes a standalone browser dashboard and an earlier Python prototype.

I made this as a learning and interface-design project: I wanted to make event patterns easier to see and explore, while keeping the analysis understandable. The score is a demonstration heuristic, not a reliable identity signal. IP addresses can be shared by many people and devices. This version does not connect to a live login service or block real accounts, firewalls, or networks.

## C++ desktop app

Open `LoginShieldNG.cbp` in Code::Blocks and choose **Build → Build and Run**. The project uses C++17, MinGW, and Windows system libraries. It does not need Python, Qt, or additional UI packages.

The sidebar opens Overview, Login activity, IP intelligence, Review alerts, Signal flow, Device scan, and Demo actions. Device scan inspects device IDs and event records in the loaded feed; it does not probe network devices. Demo blocking rules apply only to subsequent demo replays and are cleared by Reset demo or when a new CSV is imported.

## CSV format

Use **Import CSV** in the desktop app or browser dashboard. Required columns are:

```csv
time,username,ip,device,status
14:32:00,user_01,198.51.100.80,office_shared,success
```

Time format is `HH:MM:SS`. Status must be `failed` or `success`. `demo_events.csv` is a compact sample; `demo_events_elaborate.csv` contains a richer 71-event scenario. The detector reviews events in a 60-second window.

## Browser dashboard

Open `dashboard.html` in a browser. The page supports CSV import, event search and filtering, alert summaries, device event inspection, report export, demo replay, and demo-only account/IP block simulations. It runs locally in the browser and has no live-service connection.

## Earlier Python prototype

`event_processor.py`, `login_detector.py`, `main.py`, `risk_scorer.py`, `test_detector.py`, and `threat_detector.py` are earlier Python prototype files. They are not needed to build the C++ desktop app.

All sample events use documentation-only IP addresses. Use only logs you are authorized to review.

