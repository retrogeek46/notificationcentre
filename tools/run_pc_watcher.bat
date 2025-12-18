@echo off
REM PC Watcher for ESP32 - Run Script
REM Handles both media watching (normal) and PC stats (gaming mode)
REM
REM Usage:
REM   run_pc_watcher.bat         - Run in visible console
REM   run_pc_watcher.bat hidden  - Run hidden (background)
REM   run_pc_watcher.bat install - Add to Windows startup
REM   run_pc_watcher.bat remove  - Remove from startup

cd /d "%~dp0"

if "%1"=="hidden" (
    echo Starting PC Watcher in background...
    start "" pythonw pc_watcher.py
    exit /b
)

if "%1"=="install" (
    echo Installing startup task...
    schtasks /create /tn "PCWatcher" /tr "pythonw \"%~dp0pc_watcher.py\"" /sc onlogon /rl limited /f
    if %ERRORLEVEL%==0 (
        echo SUCCESS: PCWatcher will start automatically at login.
    ) else (
        echo FAILED: Could not create scheduled task. Try running as Administrator.
    )
    pause
    exit /b
)

if "%1"=="remove" (
    echo Removing startup task...
    schtasks /delete /tn "PCWatcher" /f
    if %ERRORLEVEL%==0 (
        echo SUCCESS: PCWatcher removed from startup.
    ) else (
        echo FAILED or task didn't exist.
    )
    pause
    exit /b
)

REM Default: run in console
python pc_watcher.py
pause
