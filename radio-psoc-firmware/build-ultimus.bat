@echo off
REM Build Ultimus firmware from the Proficio trees. Build the Proficio project in PSoC Creator first.
REM Usage: build-ultimus.bat [legacy^|mkii-ptt^|mkii-atu^|all]   (default: all trees that have the ULTIMUS switch)
cd /d "%~dp0"
where py >nul 2>nul && (py -3 build-ultimus.py %*) || (python build-ultimus.py %*)
set RC=%ERRORLEVEL%
pause
exit /b %RC%
