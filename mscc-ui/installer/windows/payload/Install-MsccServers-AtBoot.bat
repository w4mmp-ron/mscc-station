@echo off
setlocal EnableExtensions EnableDelayedExpansion

rem ============================================================
rem  Install (or remove) a Windows Scheduled Task that starts
rem  MSCC backends when the user logs on (desktop session).
rem
rem  Covers:
rem    - Password login  → starts after you sign in
rem    - Auto-login / no password → starts when Windows lands
rem      on the desktop (feels like "at boot" for most users)
rem
rem  Runs as the CURRENT USER (not SYSTEM) — same privilege as
rem  double-clicking Start-MsccServers.bat. UDP does not need SYSTEM.
rem
rem  Double-click this file. It will request Administrator rights
rem  only so the task can be registered reliably.
rem
rem  Usage:
rem    Install-MsccServers-AtBoot.bat           Install / update task
rem    Install-MsccServers-AtBoot.bat install   Same
rem    Install-MsccServers-AtBoot.bat remove    Remove the task
rem    Install-MsccServers-AtBoot.bat status    Show if task exists
rem ============================================================

set "TASK_NAME=MSCC Backend Servers"
set "SCRIPT_DIR=%~dp0"
if "%SCRIPT_DIR:~-1%"=="\" set "SCRIPT_DIR=%SCRIPT_DIR:~0,-1%"
set "START_BAT=%SCRIPT_DIR%\Start-MsccServers.bat"

rem Who should own the task? The interactive user (before/after elevation).
rem Prefer the user who launched us; fall back to %USERNAME%.
set "TASK_USER=%USERNAME%"
if defined USERDOMAIN if /I not "%USERDOMAIN%"=="%COMPUTERNAME%" (
  set "TASK_USER=%USERDOMAIN%\%USERNAME%"
) else (
  set "TASK_USER=%COMPUTERNAME%\%USERNAME%"
)

rem --- self-elevate if not admin (needed to create scheduled tasks cleanly) ---
net session >nul 2>&1
if errorlevel 1 (
  echo Requesting Administrator rights...
  powershell -NoProfile -ExecutionPolicy Bypass -Command ^
    "Start-Process -FilePath '%~f0' -ArgumentList '%*' -Verb RunAs"
  exit /b
)

cd /d "%SCRIPT_DIR%"

set "ACTION=%~1"
if /I "%ACTION%"=="" set "ACTION=install"
if /I "%ACTION%"=="install" goto :Install
if /I "%ACTION%"=="add" goto :Install
if /I "%ACTION%"=="update" goto :Install
if /I "%ACTION%"=="remove" goto :Remove
if /I "%ACTION%"=="uninstall" goto :Remove
if /I "%ACTION%"=="delete" goto :Remove
if /I "%ACTION%"=="status" goto :Status

echo Unknown option: %ACTION%
echo.
echo Usage:
echo   %~nx0              Install logon task
echo   %~nx0 remove       Remove logon task
echo   %~nx0 status       Show task status
echo.
pause
exit /b 1

:Install
if not exist "%START_BAT%" (
  echo ERROR: Missing "%START_BAT%"
  echo Place this installer next to Start-MsccServers.bat.
  echo.
  pause
  exit /b 1
)

echo.
echo ============================================================
echo   Install MSCC backend start-at-logon task
echo ============================================================
echo   Task name : %TASK_NAME%
echo   Script    : %START_BAT% start
echo   Trigger   : At user logon (password or auto-login)
echo   Run as    : %TASK_USER%
echo   Privilege : Same as interactive user ^(not SYSTEM^)
echo.

rem /SC ONLOGON  = when this user gets a desktop session
rem /RU          = that user (not SYSTEM)
rem /IT          = interactive only (when logged on)
rem /RL LIMITED  = do not force highest privileges
rem /F           = create or overwrite
rem
rem No /RP password: task runs only when the user is logged on
rem (works for password login and for auto-login / empty password).

schtasks /Create /TN "%TASK_NAME%" ^
  /TR "\"%ComSpec%\" /c \"\"%START_BAT%\" start\"" ^
  /SC ONLOGON ^
  /RU "%TASK_USER%" ^
  /IT ^
  /RL LIMITED ^
  /F

if errorlevel 1 (
  echo.
  echo First try failed — retrying with username only: %USERNAME%
  schtasks /Create /TN "%TASK_NAME%" ^
    /TR "\"%ComSpec%\" /c \"\"%START_BAT%\" start\"" ^
    /SC ONLOGON ^
    /RU "%USERNAME%" ^
    /IT ^
    /RL LIMITED ^
    /F
)

if errorlevel 1 (
  echo.
  echo ERROR: Failed to create scheduled task.
  echo.
  echo Tips:
  echo   - Confirm you approved the UAC prompt.
  echo   - Run: Start-MsccServers.bat start
  echo     to verify backends work under this user first.
  echo.
  pause
  exit /b 1
)

echo.
echo OK — task installed.
echo.
echo When this Windows user logs on ^(or auto-logs on^), backends start
echo if they are not already running:
echo   Mscc-trans, mscc-recv, ms-sdr
echo.
echo This does NOT use the SYSTEM account.
echo.
echo To remove later:
echo   %~nx0 remove
echo.
echo To start servers now without logging off:
echo   Start-MsccServers.bat start
echo.
pause
exit /b 0

:Remove
echo.
echo Removing scheduled task "%TASK_NAME%"...
schtasks /Delete /TN "%TASK_NAME%" /F >nul 2>&1
if errorlevel 1 (
  echo Task was not found or could not be deleted (may already be gone).
) else (
  echo OK — logon task removed.
)
echo.
pause
exit /b 0

:Status
echo.
echo Task: %TASK_NAME%
echo Expected user: %TASK_USER%
echo.
schtasks /Query /TN "%TASK_NAME%" /V /FO LIST 2>nul
if errorlevel 1 (
  echo Status: NOT INSTALLED
) else (
  echo.
  echo Status: INSTALLED (see details above)
)
echo.
pause
exit /b 0
