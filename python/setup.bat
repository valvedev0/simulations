@echo off
REM Setup script for Python Simulations project
REM Creates virtual environment, installs dependencies, and activates venv

echo.
echo ======================================
echo  Python Simulations - Setup Script
echo ======================================
echo.

if exist .venv (
    echo Virtual environment already exists at .venv
    echo Activating existing environment...
    call .\.venv\Scripts\activate.bat
    echo Upgrading pip and installing/updating requirements...
    python -m pip install --upgrade pip
    python -m pip install -r requirements.txt
) else (
    echo Creating virtual environment...
    python -m venv .venv
    
    if errorlevel 1 (
        echo Error: Failed to create virtual environment
        echo Make sure Python 3.12+ is installed and accessible
        pause
        exit /b 1
    )
    
    echo Activating virtual environment...
    call .\.venv\Scripts\activate.bat
    
    echo Upgrading pip...
    python -m pip install --upgrade pip
    
    if errorlevel 1 (
        echo Error: Failed to upgrade pip
        pause
        exit /b 1
    )
    
    echo Installing requirements...
    python -m pip install -r requirements.txt
    
    if errorlevel 1 (
        echo Error: Failed to install requirements
        pause
        exit /b 1
    )
)

echo.
echo ======================================
echo  Setup Complete!
echo ======================================
echo Virtual environment is now ACTIVE
echo.
echo You can now run:
echo   - python sim_launcher.py          (launch GUI)
echo   - python simulations\ant\run.py   (run ant simulation)
echo   - python simulations\flight\run.py (run flight simulation)
echo   - python simulations\laser\run.py (run laser simulation)
echo.
echo To deactivate the environment later, type: deactivate
echo To reactivate, run: .\.venv\Scripts\activate.bat
echo.
pause
