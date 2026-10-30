@echo off
setlocal
cd /d "%~dp0"
g++ LiveMonitor.cpp -std=c++17 -O2 -Wall -mwindows -lgdi32 -lwevtapi -o LoginShieldLiveMonitor.exe
if errorlevel 1 pause
