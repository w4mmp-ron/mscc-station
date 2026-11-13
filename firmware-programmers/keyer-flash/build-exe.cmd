@echo off
rem Builds the Windows programs from the scripts in this folder:
rem   ..\windows\keyer-flash\keyer-flash.exe      command line
rem   ..\windows\keyer-flash\keyer-flash-gui.exe  window (Keyer Firmware Update)
rem Python, pyusb and the engine are built in; the user's PC needs no Python.
rem Needs here: Python 3 + pyusb + PyInstaller (python -m pip install --user pyusb pyinstaller)
setlocal
pushd "%~dp0"
set OUT=%~dp0..\windows\keyer-flash
set WORK=%TEMP%\keyer-flash-build
set HID=--hidden-import usb.core --hidden-import usb.backend.libusb0 --hidden-import usb.backend.libusb1 --hidden-import subprocess
python -m PyInstaller --noconfirm --clean --onefile --console --name keyer-flash --distpath "%OUT%" --workpath "%WORK%" --specpath "%WORK%" %HID% keyer-flash.py
if errorlevel 1 goto fail
python -m PyInstaller --noconfirm --clean --onefile --windowed --name keyer-flash-gui --distpath "%OUT%" --workpath "%WORK%" --specpath "%WORK%" --add-data "%~dp0keyer-flash.py;." %HID% keyer-flash-gui.py
if errorlevel 1 goto fail
echo Built %OUT%\keyer-flash.exe and keyer-flash-gui.exe
popd
exit /b 0
:fail
echo BUILD FAILED
popd
exit /b 1
