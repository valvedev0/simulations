@echo off
REM Clean script for Python Simulations project
REM Removes virtual environment and build artifacts

echo.
echo ======================================
echo  Python Simulations - Clean Script
echo ======================================
echo.

setlocal enabledelayedexpansion

REM Counter for removed items
set removed_count=0

REM Remove virtual environment
if exist .venv (
    echo Removing virtual environment (.venv)...
    rmdir /s /q .venv
    set /a removed_count+=1
    if errorlevel 1 (
        echo Warning: Some files in .venv may not have been deleted
    )
)

REM Remove __pycache__ directories
echo Removing Python cache files (__pycache__)...
for /d /r . %%d in (__pycache__) do (
    if exist "%%d" (
        rmdir /s /q "%%d" 2>nul
        set /a removed_count+=1
    )
)

REM Remove .pytest_cache
if exist .pytest_cache (
    echo Removing pytest cache (.pytest_cache)...
    rmdir /s /q .pytest_cache
    set /a removed_count+=1
)

REM Remove build directory
if exist build (
    echo Removing build directory...
    rmdir /s /q build
    set /a removed_count+=1
)

REM Remove dist directory
if exist dist (
    echo Removing dist directory...
    rmdir /s /q dist
    set /a removed_count+=1
)

REM Remove *.egg-info directories
for /d %%d in (*.egg-info) do (
    if exist "%%d" (
        echo Removing egg-info: %%d
        rmdir /s /q "%%d" 2>nul
        set /a removed_count+=1
    )
)

REM Remove .pyc files
for /r . %%f in (*.pyc) do (
    if exist "%%f" (
        del "%%f" 2>nul
        set /a removed_count+=1
    )
)

echo.
echo ======================================
echo  Cleanup Complete!
echo ======================================
echo.
if !removed_count! gtr 0 (
    echo Removed !removed_count! item(s)
) else (
    echo Project was already clean
)
echo.
echo Next steps:
echo   - Run setup.bat to rebuild the environment
echo   - Or run rebuild.bat to clean and rebuild automatically
echo.
pause
