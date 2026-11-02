@echo off
REM Copy the PSoC Creator build into this tree's Release\ folder, named by version:
REM   Release\Proficio-MKII-PTT-<major>.<minor>.cyacd / .hex   (radio-psoc-firmware\copy-release.py)
REM Run by the Creator post-build (full path, see POST-BUILD.txt), or double-click it.
setlocal
set "SCRIPT=%~dp0..\copy-release.py"
where py >nul 2>nul
if %ERRORLEVEL%==0 (py -3 "%SCRIPT%" Proficio-Ultimus-MKII-PTT Proficio-MKII-PTT Proficio-MKII-PTT) else (python "%SCRIPT%" Proficio-Ultimus-MKII-PTT Proficio-MKII-PTT Proficio-MKII-PTT)
exit /b %ERRORLEVEL%
