@echo off
REM Copy the PSoC Creator build into this tree's Release\ folder, named by version:
REM   Release\Geminus-MKII-<major>.<minor>.cyacd / .hex   (radio-psoc-firmware\copy-release.py)
REM Run by the Creator post-build (full path, see POST-BUILD.txt), or double-click it.
setlocal
set "SCRIPT=%~dp0..\copy-release.py"
where py >nul 2>nul
if %ERRORLEVEL%==0 (py -3 "%SCRIPT%" Geminus-MKII Geminus Geminus-MKII) else (python "%SCRIPT%" Geminus-MKII Geminus Geminus-MKII)
exit /b %ERRORLEVEL%
