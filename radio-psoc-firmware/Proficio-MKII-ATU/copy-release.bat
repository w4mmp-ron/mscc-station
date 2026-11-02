@echo off
REM Copy the PSoC Creator build into this tree's Release\ folder, named by version:
REM   Release\Proficio-MKII-ATU-<major>.<minor>.cyacd / .hex   (radio-psoc-firmware\copy-release.py)
REM Run by <Project>.cydsn\post-build-release.bat (PSoC Creator post-build), or double-click it.
setlocal
set "SCRIPT=%~dp0..\copy-release.py"
where py >nul 2>nul
if %ERRORLEVEL%==0 (py -3 "%SCRIPT%" Proficio-MKII-ATU ) else (python "%SCRIPT%" Proficio-MKII-ATU )
exit /b %ERRORLEVEL%
