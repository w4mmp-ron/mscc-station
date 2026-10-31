@echo off
rem Black Pill counters on a Windows host (same output as audio-stats.py on the Pi).
rem   audio-stats            one reading
rem   audio-stats watch 60   one line every 0.5 s for 60 s
rem Needs Python 3 and pyusb (pip install pyusb). Uses libusb0.dll (libusb-win32,
rem as the Windows ms-sdr) or libusb-1.0.dll, from System32 or next to the script.
rem Uses audio-stats.py next to this file, else the master copy in ..\Pi.
if exist "%~dp0audio-stats.py" (
    python "%~dp0audio-stats.py" %*
) else (
    python "%~dp0..\Pi\audio-stats.py" %*
)
