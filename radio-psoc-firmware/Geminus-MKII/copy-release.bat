@echo off
REM Copy the PSoC Creator build into this tree's Release\ folder, named by version:
REM   Release\Geminus-MKII-<major>.<minor>.cyacd / .hex   (radio-psoc-firmware\copy-release.py)
REM Run by <Project>.cydsn\post-build-release.bat (PSoC Creator post-build), or double-click it.
setlocal
set "SCRIPT=%~dp0..\copy-release.py"
where py >nul 2>nul
if %ERRORLEVEL%==0 (py -3 "%SCRIPT%" Geminus-MKII Geminus) else (python "%SCRIPT%" Geminus-MKII Geminus)
exit /b %ERRORLEVEL%
