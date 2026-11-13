@echo off
rem Builds the keyer I2C bootloader -> build\keyer-boot.hex (0x0000-0x03FF)
pushd "%~dp0"
if not exist build mkdir build
"C:\Program Files\Microchip\xc8\v2.45\bin\xc8-cc.exe" -mcpu=16F18326 -O2 -mrom=0-3FF -o build\keyer-boot.elf boot.c
if errorlevel 1 (popd & exit /b 1)
echo Built build\keyer-boot.hex
popd
