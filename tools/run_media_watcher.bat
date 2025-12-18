@echo off
REM Media Watcher for ESP32 - Run Script
REM Usage:
REM   run_media_watcher.bat         - Run in visible console
REM   run_media_watcher.bat hidden  - Run hidden (background)
REM   run_media_watcher.bat install - Add to Windows startup
REM   run_media_watcher.bat remove  - Remove from startup

cd /d "%~dp0"

if "%1"=="hidden" (
    echo Starting Media Watcher in background...
    start "" pythonw media_watcher.py
    exit /b
)

if "%1"=="install" (
    echo Installing startup task...
    schtasks /create /tn "MediaWatcher" /tr "pythonw \"%~dp0media_watcher.py\"" /sc onlogon /rl limited /f
    if %ERRORLEVEL%==0 (
        echo SUCCESS: MediaWatcher will start automatically at login.
    ) else (
        echo FAILED: Could not create scheduled task. Try running as Administrator.
    )
    pause
    exit /b
)

if "%1"=="remove" (
    echo Removing startup task...
    schtasks /delete /tn "MediaWatcher" /f
    if %ERRORLEVEL%==0 (
        echo SUCCESS: MediaWatcher removed from startup.
    ) else (
        echo FAILED or task didn't exist.
    )
    pause
    exit /b
)

REM Default: run in console
python media_watcher.py
pause
