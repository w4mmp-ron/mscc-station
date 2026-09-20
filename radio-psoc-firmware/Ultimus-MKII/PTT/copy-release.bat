@echo off
REM Copy PSoC Creator outputs (.cyacd + .hex) into this tree's Release\
REM Always resolve paths from THIS script's location (not current directory).
setlocal EnableExtensions
set "ROOT=%~dp0"
set "ROOT=%ROOT:~0,-1%"
set "BUILD=%ROOT%\Proficio-MKII-PTT.cydsn\DP8051\DP8051_Keil_951\Debug"
set "DEST=%ROOT%\Release"

set "STAMP=%date:~10,4%%date:~4,2%%date:~7,2%"

echo copy-release: ROOT=%ROOT%
echo copy-release: BUILD=%BUILD%
echo copy-release: cwd=%CD%

if not exist "%BUILD%\Proficio-MKII-PTT.hex" (
  echo ERROR: missing "%BUILD%\Proficio-MKII-PTT.hex"
  echo Build the project in PSoC Creator first.
  exit /b 1
)
if not exist "%BUILD%\Proficio-MKII-PTT.cyacd" (
  echo ERROR: missing "%BUILD%\Proficio-MKII-PTT.cyacd"
  exit /b 1
)

if not exist "%DEST%" mkdir "%DEST%"

copy /Y "%BUILD%\Proficio-MKII-PTT.hex"   "%DEST%\Proficio-MKII-PTT-%STAMP%.hex"   >nul
copy /Y "%BUILD%\Proficio-MKII-PTT.cyacd" "%DEST%\Proficio-MKII-PTT-%STAMP%.cyacd" >nul
copy /Y "%BUILD%\Proficio-MKII-PTT.hex"   "%DEST%\Proficio-MKII-PTT.hex"           >nul
copy /Y "%BUILD%\Proficio-MKII-PTT.cyacd" "%DEST%\Proficio-MKII-PTT.cyacd"         >nul

echo OK: copied hex+cyacd to "%DEST%"
dir /B "%DEST%\Proficio-MKII-PTT*"
endlocal
exit /b 0
