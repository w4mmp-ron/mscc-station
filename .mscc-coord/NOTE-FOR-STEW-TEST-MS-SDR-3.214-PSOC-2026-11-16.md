**Subject: Quick test please: PSoC radio on the new Windows ms-sdr 3.214**

Stew,

Before I release, could you run one PSoC radio (any MKII) on the new Windows ms-sdr?

**What's new in 3.214**
- It can talk to the radio through either driver: the Zadig driver (as now) or Windows' own WinUSB driver. The Black Pill 3.17 uses WinUSB, so it needs no Zadig.
- It picks the driver by itself. On a PC with Zadig it uses exactly the same code as today.
- The WiFi SWR meter now works with Windows too. Before, the meter link failed at start-up on Windows, so the meter never got the "tuning now" message and could trip during an ATU tune.

**Where the files are (after a pull)**
- `mscc-ui\Release\windows-wpf\ms-sdr-MKII.exe` (3.214)
- `mscc-ui\Release\windows-wpf\libusb-1.0.dll` (new, needed by 3.214)

**Test (about 5 minutes)**
1. Close MSCC. Rename your old `C:\mscc-net9\ms-sdr-MKII.exe` (to go back if needed).
2. Copy the two files above into `C:\mscc-net9`.
3. Start MSCC. Check receive and do a short TUNE.
4. Open `%LOCALAPPDATA%\MSCC-NET9\logs\ms-sdr.log` and look for the line `srOpen . using libusb-win32 (Zadig)`.
5. Tell me OK or not. If not, please send me that log.

Thanks,
Ron
