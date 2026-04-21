@echo off
REM Rebuild script for Python Simulations project
REM Cleans the project and rebuilds the environment from scratch

echo.
echo ======================================
echo  Python Simulations - Rebuild Script
echo ======================================
echo.
echo This will clean the project and rebuild from scratch.
echo.

REM Call clean.bat without pausing
call clean.bat < nul >nul 2>&1

echo.
echo ======================================
echo  Rebuilding Environment...
echo ======================================
echo.

REM Call setup.bat
call setup.bat
