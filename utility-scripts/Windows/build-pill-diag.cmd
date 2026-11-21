@echo off
rem Build pill-diag.exe (one file, no console) into dist\ next to this script.
rem Needs: python with pyinstaller, pyusb, sounddevice, numpy.
cd /d "%~dp0"
python -m PyInstaller --noconfirm --onefile --windowed --name pill-diag ^
  --add-data "%~dp0audio-stats.py;." --add-binary "%~dp0libusb-1.0.dll;." ^
  --distpath "%~dp0dist" --workpath "%TEMP%\pill-diag-build" --specpath "%TEMP%\pill-diag-build" ^
  pill-diag.py
