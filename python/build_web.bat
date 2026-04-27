@echo off
setlocal

set "SIM=%~1"
if "%SIM%"=="" set "SIM=laser"

if exist ".\.venv\Scripts\python.exe" (
    .\.venv\Scripts\python.exe tools\build_web_sim.py %SIM%
) else (
    python tools\build_web_sim.py %SIM%
)
