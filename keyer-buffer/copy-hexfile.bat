@echo off
rem MPLAB post-build step. Makes the two release files in Release\:
rem   Keyer-MCU-full-YYYYMMDD.hex  bootloader + app + EEPROM defaults  (PICkit 3)
rem   Keyer-MCU-app-YYYYMMDD.hex   app only, from 0x400               (Pi flash utility)
cd /d "%~dp0"
set D=%date:~10,4%%date:~4,2%%date:~7,2%
set APP=dist\production\production\keyer.production.hex
call bootloader\build.bat
if errorlevel 1 exit /b 1
copy /Y /B %APP% Release\Keyer-MCU-app-%D%.hex
if errorlevel 1 exit /b 1
"C:\Program Files\Microchip\xc8\v2.45\pic\bin\hexmate.exe" bootloader\build\keyer-boot.hex %APP% -ORelease\Keyer-MCU-full-%D%.hex
if errorlevel 1 exit /b 1
echo Made Release\Keyer-MCU-full-%D%.hex and Release\Keyer-MCU-app-%D%.hex
