@echo off
setlocal
cd /d "%~dp0"

echo Flashing bootloader (ST-Link / CubeProgrammer CLI)...
echo.
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0stm32-bootloader-flash.ps1" %*
set ERR=%ERRORLEVEL%

echo.
if %ERR% neq 0 (
  echo FAILED - exit code %ERR%
) else (
  echo Done.
)
echo.
pause
exit /b %ERR%
