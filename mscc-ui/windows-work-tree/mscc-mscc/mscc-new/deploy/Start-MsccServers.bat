@echo off
setlocal EnableExtensions EnableDelayedExpansion

rem ============================================================
rem  MSCC backend server launcher (no GUI client)
rem  Location: same folder as the backend binaries (e.g. C:\mscc-net9)
rem
rem  Starts / stops only:
rem    Mscc-trans.exe, mscc-recv.exe, ms-sdr-*.exe
rem
rem  Usage:
rem    Start-MsccServers.bat              Interactive menu
rem    Start-MsccServers.bat start        Silent start (boot / Task Scheduler)
rem    Start-MsccServers.bat stop         Silent stop
rem    Start-MsccServers.bat restart      Silent restart
rem    Start-MsccServers.bat status       Process status + PROFICIO-MKII
rem
rem  Legacy / external electronic keyer (host mscc.ini PROFICIO-MKII):
rem    Start-MsccServers.bat legacy      PROFICIO-MKII=0 + restart backends
rem    Start-MsccServers.bat mkii        PROFICIO-MKII=1 + restart backends
rem    Start-MsccServers.bat keyer       Show current PROFICIO-MKII (no restart)
rem
rem  ms-sdr reads %%LocalAppData%%\MSCC-NET9\mscc.ini at process start.
rem  Client GUI checkbox is sticky UI only when remote; this bat sets the host flag.
rem  Aliases for start: silent, auto
rem ============================================================

title MSCC Servers

set "SERVER_ROOT=%~dp0"
if "%SERVER_ROOT:~-1%"=="\" set "SERVER_ROOT=%SERVER_ROOT:~0,-1%"

set "TRANS_EXE=Mscc-trans.exe"
set "RECV_EXE=mscc-recv.exe"
set "SDR_EXE=ms-sdr-MKII.exe"

set "START_GAP=2"
set "RUNHIDDEN=%~dp0runhidden.vbs"
set "SILENT=0"
set "MSCC_INI=%LOCALAPPDATA%\MSCC-NET9\mscc.ini"

if not exist "%SERVER_ROOT%\" (
  echo ERROR: Folder not found: "%SERVER_ROOT%"
  if "%SILENT%"=="0" pause
  exit /b 1
)

if exist "%SERVER_ROOT%\ms-sdr-MKII.exe" (
  set "SDR_EXE=ms-sdr-MKII.exe"
) else if exist "%SERVER_ROOT%\ms-sdr-proficio.exe" (
  set "SDR_EXE=ms-sdr-proficio.exe"
) else if exist "%SERVER_ROOT%\ms-sdr.exe" (
  set "SDR_EXE=ms-sdr.exe"
)

if not exist "%RUNHIDDEN%" (
  echo ERROR: Missing "%RUNHIDDEN%"
  if "%SILENT%"=="0" pause
  exit /b 1
)

cd /d "%SERVER_ROOT%" || (
  echo ERROR: Cannot cd to "%SERVER_ROOT%"
  if "%SILENT%"=="0" pause
  exit /b 1
)

rem ----- command-line mode (silent / scripted) -----
set "CMD=%~1"
if /I "%CMD%"=="" goto :Menu
if /I "%CMD%"=="menu" goto :Menu

if /I "%CMD%"=="start"  set "SILENT=1" & goto :CliStart
if /I "%CMD%"=="silent" set "SILENT=1" & goto :CliStart
if /I "%CMD%"=="auto"   set "SILENT=1" & goto :CliStart
if /I "%CMD%"=="stop"   set "SILENT=1" & goto :CliStop
if /I "%CMD%"=="restart" set "SILENT=1" & goto :CliRestart
if /I "%CMD%"=="status" set "SILENT=1" & goto :CliStatus

if /I "%CMD%"=="legacy" set "SILENT=1" & goto :CliLegacy
if /I "%CMD%"=="mkii"   set "SILENT=1" & goto :CliMkii
if /I "%CMD%"=="keyer"  set "SILENT=1" & goto :CliKeyer

echo Unknown command: %CMD%
echo Usage: %~nx0 [start^|stop^|restart^|status^|legacy^|mkii^|keyer^|menu]
exit /b 1

:CliStart
call :DoStart
exit /b %ERRORLEVEL%

:CliStop
call :DoStop
exit /b 0

:CliRestart
call :DoRestart
exit /b %ERRORLEVEL%

:CliStatus
call :DoStatus
exit /b 0

:CliLegacy
call :DoSetKeyer 0
if errorlevel 1 exit /b 1
call :DoRestart
exit /b %ERRORLEVEL%

:CliMkii
call :DoSetKeyer 1
if errorlevel 1 exit /b 1
call :DoRestart
exit /b %ERRORLEVEL%

:CliKeyer
call :DoShowKeyer
exit /b 0

rem ----- interactive menu -----
:Menu
set "SILENT=0"
cls
echo ================================================
echo   MSCC Backend Servers  (no client)
echo ================================================
echo   Folder: %SERVER_ROOT%
echo   mscc.ini: %MSCC_INI%
echo.
echo   [1] Start servers   (trans -^> recv -^> ms-sdr)
echo   [2] Stop servers
echo   [3] Restart servers
echo   [4] Status (+ keyer mode)
echo   [5] Legacy CW / external keyer  (PROFICIO-MKII=0 + restart)
echo   [6] MKII internal keyer         (PROFICIO-MKII=1 + restart)
echo   [7] Exit
echo.
echo   Silent (Task Scheduler / boot):
echo     %~nx0 start
echo   Host keyer mode (no full UI needed):
echo     %~nx0 legacy     %~nx0 mkii     %~nx0 keyer
echo.
choice /c 1234567 /n /m "Enter choice (1-7): "
if errorlevel 7 goto :EOF
if errorlevel 6 goto :MenuMkii
if errorlevel 5 goto :MenuLegacy
if errorlevel 4 goto :MenuStatus
if errorlevel 3 goto :MenuRestart
if errorlevel 2 goto :MenuStop
if errorlevel 1 goto :MenuStart
goto :Menu

:MenuStart
call :DoStart
if errorlevel 1 (
  pause
  goto :Menu
)
echo.
echo Done. Use [4] Status or Task Manager to verify.
pause
goto :Menu

:MenuStop
call :DoStop
echo Done.
pause
goto :Menu

:MenuRestart
call :DoRestart
if errorlevel 1 (
  pause
  goto :Menu
)
echo.
echo Restart complete.
pause
goto :Menu

:MenuStatus
call :DoStatus
echo.
pause
goto :Menu

:MenuLegacy
echo.
echo Setting legacy / external electronic keyer (PROFICIO-MKII=0)...
call :DoSetKeyer 0
if errorlevel 1 (
  pause
  goto :Menu
)
call :DoRestart
if errorlevel 1 (
  pause
  goto :Menu
)
echo.
echo Legacy CW mode set and backends restarted.
pause
goto :Menu

:MenuMkii
echo.
echo Setting Proficio MKII internal keyer (PROFICIO-MKII=1)...
call :DoSetKeyer 1
if errorlevel 1 (
  pause
  goto :Menu
)
call :DoRestart
if errorlevel 1 (
  pause
  goto :Menu
)
echo.
echo MKII keyer mode set and backends restarted.
pause
goto :Menu

rem ----- shared actions -----
:DoStart
echo.
call :EnsurePresent
if errorlevel 1 exit /b 1
echo Starting backends in %SERVER_ROOT% ...
echo   order: trans -^> recv -^> ms-sdr
echo.
call :StartOne "%TRANS_EXE%"
timeout /t %START_GAP% /nobreak >nul
call :StartOne "%RECV_EXE%"
timeout /t %START_GAP% /nobreak >nul
call :StartOne "%SDR_EXE%"
echo.
echo Done.
call :DoShowKeyer
exit /b 0

:DoStop
echo.
echo Stopping backends...
call :StopOne "%SDR_EXE%"
call :StopOne "%RECV_EXE%"
call :StopOne "%TRANS_EXE%"
exit /b 0

:DoRestart
echo.
call :EnsurePresent
if errorlevel 1 exit /b 1
echo Restarting in %SERVER_ROOT% ...
call :StopOne "%SDR_EXE%"
call :StopOne "%RECV_EXE%"
call :StopOne "%TRANS_EXE%"
timeout /t 2 /nobreak >nul
call :StartOne "%TRANS_EXE%"
timeout /t %START_GAP% /nobreak >nul
call :StartOne "%RECV_EXE%"
timeout /t %START_GAP% /nobreak >nul
call :StartOne "%SDR_EXE%"
call :DoShowKeyer
exit /b 0

:DoStatus
echo.
echo Folder: %SERVER_ROOT%
echo Process status:
call :ShowStatus "%TRANS_EXE%"
call :ShowStatus "%RECV_EXE%"
call :ShowStatus "%SDR_EXE%"
call :DoShowKeyer
exit /b 0

rem %1 = 0 (legacy) or 1 (MKII)
:DoSetKeyer
set "MKII_VAL=%~1"
if not "%MKII_VAL%"=="0" if not "%MKII_VAL%"=="1" (
  echo ERROR: DoSetKeyer expects 0 or 1
  exit /b 1
)
if not exist "%LOCALAPPDATA%\MSCC-NET9" mkdir "%LOCALAPPDATA%\MSCC-NET9" 2>nul
powershell -NoProfile -ExecutionPolicy Bypass -Command ^
  "$p = $env:LOCALAPPDATA + '\MSCC-NET9\mscc.ini'; $v = '%MKII_VAL%';" ^
  "if (Test-Path $p) { $t = [IO.File]::ReadAllText($p) } else { $t = '' };" ^
  "$t = [regex]::Replace($t, '(?im)^\s*\$\d+\.\d+\.\d+\.\d+;?\s*\r?\n?', '');" ^
  "$t = [regex]::Replace($t, '(?im)^\s*PROFICIO_MKII\s*=[^\r\n]*\r?\n?', '');" ^
  "if ([regex]::IsMatch($t, '(?im)^\s*PROFICIO-MKII\s*=')) {" ^
  "  $t = [regex]::Replace($t, '(?im)^(\s*PROFICIO-MKII\s*=)[^;\r\n]*', '${1}' + $v);" ^
  "} else {" ^
  "  if ($t.Length -gt 0 -and -not $t.EndsWith([Environment]::NewLine)) { $t += [Environment]::NewLine };" ^
  "  $t += 'PROFICIO-MKII=' + $v + ';' + [Environment]::NewLine;" ^
  "};" ^
  "[IO.File]::WriteAllText($p, $t); Write-Host ('Wrote PROFICIO-MKII=' + $v + ' to ' + $p)"
if errorlevel 1 (
  echo ERROR: failed to write %MSCC_INI%
  exit /b 1
)
if "%MKII_VAL%"=="0" (
  echo   Host keyer: LEGACY / external electronic keyer (PROFICIO-MKII=0^)
) else (
  echo   Host keyer: MKII internal keyer (PROFICIO-MKII=1^)
)
exit /b 0

:DoShowKeyer
echo.
echo Host keyer (ms-sdr reads at start^):
echo   File: %MSCC_INI%
if not exist "%MSCC_INI%" (
  echo   PROFICIO-MKII: not set (ms-sdr default = 1 / MKII^)
  exit /b 0
)
set "FOUND="
for /f "usebackq tokens=1,* delims==" %%A in (`findstr /I /B /C:"PROFICIO-MKII=" /C:"PROFICIO_MKII=" "%MSCC_INI%" 2^>nul`) do (
  set "KV=%%B"
  set "FOUND=1"
)
if not defined FOUND (
  echo   PROFICIO-MKII: not set (ms-sdr default = 1 / MKII^)
  exit /b 0
)
set "KV=!KV:;=!"
set "KV=!KV: =!"
if "!KV!"=="0" (
  echo   PROFICIO-MKII=0  LEGACY / external electronic keyer
) else (
  echo   PROFICIO-MKII=!KV!  MKII internal keyer
)
exit /b 0

:EnsurePresent
if not exist "%SERVER_ROOT%\%TRANS_EXE%" (
  echo ERROR: Missing "%SERVER_ROOT%\%TRANS_EXE%"
  exit /b 1
)
if not exist "%SERVER_ROOT%\%RECV_EXE%" (
  echo ERROR: Missing "%SERVER_ROOT%\%RECV_EXE%"
  exit /b 1
)
if not exist "%SERVER_ROOT%\%SDR_EXE%" (
  echo ERROR: Missing ms-sdr binary in "%SERVER_ROOT%"
  echo Tried: ms-sdr-MKII.exe / ms-sdr-proficio.exe / ms-sdr.exe
  exit /b 1
)
exit /b 0

:IsRunning
set "RUNNING=0"
tasklist /FI "IMAGENAME eq %~1" /NH 2>nul | find /I "%~1" >nul
if not errorlevel 1 set "RUNNING=1"
exit /b 0

:StartOne
set "EXE=%~1"
call :IsRunning "%EXE%"
if "!RUNNING!"=="1" (
  echo   [skip] %EXE% already running
  exit /b 0
)
if not exist "%SERVER_ROOT%\%EXE%" (
  echo   [fail] not found: %SERVER_ROOT%\%EXE%
  exit /b 1
)
cscript //nologo "%RUNHIDDEN%" "%SERVER_ROOT%" "%SERVER_ROOT%\%EXE%" test
if errorlevel 1 (
  echo   [fail] could not launch %EXE%
  exit /b 1
)
echo   [ok]   started %EXE%
exit /b 0

:StopOne
set "EXE=%~1"
call :IsRunning "%EXE%"
if "!RUNNING!"=="0" (
  echo   [skip] %EXE% was not running
  exit /b 0
)
echo   Stopping %EXE%...
taskkill /F /IM "%EXE%" >nul 2>&1
timeout /t 1 /nobreak >nul
call :IsRunning "%EXE%"
if "!RUNNING!"=="1" (
  echo   [warn] %EXE% still present after taskkill
) else (
  echo   [ok]   stopped %EXE%
)
exit /b 0

:ShowStatus
set "EXE=%~1"
call :IsRunning "%EXE%"
if "!RUNNING!"=="1" (
  echo   RUNNING  %EXE%
) else (
  echo   stopped  %EXE%
)
exit /b 0
