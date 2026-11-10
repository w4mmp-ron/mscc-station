@echo off
setlocal EnableExtensions

rem ============================================================
rem  ONE-TIME installation of MSCC config files into:
rem    %LocalAppData%\MSCC-NET9\
rem
rem  Run ONCE after the first install. Do NOT re-run on MSCC
rem  updates (that would wipe user settings).
rem
rem  Already-run marker (survives app updates):
rem    %LocalAppData%\MSCC-NET9\MSCC_INIT_COMPLETE.flag
rem
rem  Emergency re-init only: delete that flag file, then run again.
rem ============================================================

set "DEST=%LocalAppData%\MSCC-NET9"
set "FLAG=%DEST%\MSCC_INIT_COMPLETE.flag"
set "SRC=%~dp0init-files"

echo.
echo ============================================================
echo   MSCC — INITIALIZATION CONFIGURATION FILE INSTALL
echo ============================================================
echo.
echo   Destination: %DEST%
echo.
echo   This should be run ONLY ONCE after the first installation.
echo   Do NOT run again when updating MSCC (your settings would be
echo   overwritten).
echo.

if exist "%FLAG%" (
  echo   *** ALREADY INITIALIZED ***
  echo.
  echo   This PC was already set up. Initialize.bat will not run again.
  echo.
  if exist "%FLAG%" (
    echo   Marker file:
    type "%FLAG%"
    echo.
  )
  echo   To force a full re-install of default configs (DESTROYS current
  echo   settings in %DEST%^):
  echo     1. Delete: %FLAG%
  echo     2. Run Initialize.bat again
  echo.
  pause
  exit /b 1
)

if not exist "%SRC%\" (
  echo ERROR: init-files folder not found next to this script:
  echo   %SRC%
  echo.
  pause
  exit /b 1
)

echo INSTALLATION WILL OVERWRITE ANY EXISTING FILES IN:
echo   %DEST%
echo.
echo THE ANSWER IS CASE SENSITIVE AND MUST BE COMPLETE (Yes / No).
echo DO YOU WANT TO INSTALL THE INITIALIZATION CONFIGURATION FILES?
set /p Input=Enter Yes or No:

if /I not "%Input%"=="Yes" (
  echo.
  echo INSTALLATION NOT RUN
  pause
  exit /b 0
)

echo.
echo Installing...
if not exist "%DEST%\" mkdir "%DEST%"

rem Clear previous contents then copy defaults
del /Q "%DEST%\*" >nul 2>&1
for /d %%D in ("%DEST%\*") do rd /s /q "%%D" >nul 2>&1

xcopy "%SRC%\*" "%DEST%\" /s /e /h /Y
if errorlevel 1 (
  echo.
  echo ERROR: xcopy failed.
  pause
  exit /b 1
)

rem Marker: survives MSCC app updates; blocks re-run of this script
(
  echo MSCC initialization completed.
  echo Date: %DATE% %TIME%
  echo Computer: %COMPUTERNAME%
  echo User: %USERNAME%
  echo Source: %SRC%
) > "%FLAG%"

echo.
echo OK — configuration files installed.
echo Marker written: %FLAG%
echo.
echo Initialize.bat will refuse to run again on this PC.
echo.
pause
exit /b 0
