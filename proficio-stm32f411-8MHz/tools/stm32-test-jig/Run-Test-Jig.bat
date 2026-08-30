@echo off
REM Double-click this in Explorer to start the STM32 USB test jig.
cd /d "%~dp0"

where pythonw >nul 2>&1
if %ERRORLEVEL%==0 (
  start "Proficio STM32 Test Jig" pythonw "%~dp0jig.py"
  exit /b 0
)

where python >nul 2>&1
if %ERRORLEVEL%==0 (
  start "Proficio STM32 Test Jig" python "%~dp0jig.py"
  exit /b 0
)

echo Python not found on PATH.
echo Install Python 3 and ensure "python" works in a Command Prompt.
echo Then:  python -m pip install -r requirements.txt
pause
exit /b 1
