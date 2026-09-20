@echo off
REM Copy PSoC Creator outputs (.cyacd + .hex) into this tree's Release\
REM Always resolve paths from THIS script's location (not current directory).
setlocal EnableExtensions
set "ROOT=%~dp0"
set "ROOT=%ROOT:~0,-1%"
set "BUILD=%ROOT%\Proficio-Legacy.cydsn\DP8051\DP8051_Keil_951\Debug"
set "DEST=%ROOT%\Release"

REM Windows date stamp YYYYMMDD (locale-safe enough for en-US)
set "STAMP=%date:~10,4%%date:~4,2%%date:~7,2%"

echo copy-release: ROOT=%ROOT%
echo copy-release: BUILD=%BUILD%
echo copy-release: cwd=%CD%

if not exist "%BUILD%\Proficio-Legacy.hex" (
  echo ERROR: missing "%BUILD%\Proficio-Legacy.hex"
  echo Build the project in PSoC Creator first.
  exit /b 1
)
if not exist "%BUILD%\Proficio-Legacy.cyacd" (
  echo ERROR: missing "%BUILD%\Proficio-Legacy.cyacd"
  exit /b 1
)

if not exist "%DEST%" mkdir "%DEST%"

copy /Y "%BUILD%\Proficio-Legacy.hex"   "%DEST%\Proficio-Legacy-%STAMP%.hex"   >nul
copy /Y "%BUILD%\Proficio-Legacy.cyacd" "%DEST%\Proficio-Legacy-%STAMP%.cyacd" >nul
copy /Y "%BUILD%\Proficio-Legacy.hex"   "%DEST%\Proficio-Legacy.hex"           >nul
copy /Y "%BUILD%\Proficio-Legacy.cyacd" "%DEST%\Proficio-Legacy.cyacd"         >nul

echo OK: copied hex+cyacd to "%DEST%"
dir /B "%DEST%\Proficio-Legacy*"
endlocal
exit /b 0
