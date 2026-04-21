@echo off
REM Launcher script for Python Simulations GUI
REM Activates the virtual environment and runs the launcher

if not exist .venv (
    echo Error: Virtual environment not found!
    echo Please run setup.bat first to create and configure the environment.
    pause
    exit /b 1
)

call .\.venv\Scripts\activate.bat

echo Launching Simulation Launcher GUI...
python sim_launcher.py

if errorlevel 1 (
    echo.
    echo Error: Failed to launch the simulator
    pause
    exit /b 1
)
