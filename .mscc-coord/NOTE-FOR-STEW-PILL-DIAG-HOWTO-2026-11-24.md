# Note for Stew: how to run pill-diag

2026-11-24, from Ron. This answers the housekeeping question in `QUESTIONS-FOR-RON-blackpill-usb-audio.md`.

pill-diag is a Windows check of a radio with a Black Pill. It is one window that works through the steps in order. At the end it saves a zip on your Desktop for you to send back. Nothing needs installing.

## Where it is

- `utility-scripts\Windows\dist\pill-diag.exe` in the mscc-station tree. Copy it anywhere and run it.
- It is not in the MSCC installer.
- The source is `utility-scripts\Windows\pill-diag.py`. Rebuild with `build-pill-diag.cmd`, which needs Python with pyinstaller, pyusb, sounddevice and numpy. The rebuilt `.exe` is committed whenever `pill-diag.py` changes.

## Before you start

1. **Close MSCC: the client and all the servers.** pill-diag checks for them first and stops if any are running.
2. Keep the radio's USB cable plugged in and the radio powered on.
3. **For the transmit test (step 5) only:**
   - Put a dummy load on the antenna socket.
   - Turn the amplifier off or unplug it. The pill keys the amplifier line on every transmit.
   - Have a scope or spectrum analyser on the output if you want to read the result.

## Running it

1. Start `pill-diag.exe`. The window is called **Black Pill Diagnostics 1.0**.
2. Click **Run checks**. Steps 1 to 4 run on their own:
   1. Finds the radio: its USB IDs, its driver and the pill version.
   2. Reads the pill's state: the last reset, power-up I2C, any hang record and the counters.
   3. Checks the codec setup and the receive link.
   4. Records 10 s of raw I/Q, measures the sample rate against the PC clock, and saves it as a `.wav`.
3. **Step 5, transmit test (optional):**
   - Pick a band. The LO goes to that band's FT8 frequency.
   - Click **Key now (5 s)** and confirm the dummy-load question. **STOP** unkeys at once.
   - Expected: one carrier 12 kHz above or below the LO, plus small leftovers at the LO and at 12 kHz on the other side.
   - Write down what the analyser showed. pill-diag can't see it.
   - You can key again on other bands.
4. Click **Skip / done: save the zip**. Step 6 gathers the MSCC ini files and logs, the program versions, the USB driver and the Windows version. Step 7 saves the zip.

## What you get

- `pill-diag-<date-time>.zip` on your Desktop. A "Done" box shows the full path.
- Inside:
  - `summary.txt`, with FAIL and WARNING lines first
  - `details.txt`, the full log
  - the receive `.wav`
  - the pill counters
  - the MSCC setup files
- Send the zip back. Add what the analyser showed in step 5, if you ran it.

## If something goes wrong

| What you see | What to do |
| --- | --- |
| "MSCC is running (...)" | Close the client and servers, then click **Run checks** again. |
| Step 1 can't find the radio | Check the USB cable and power, and that no other program has the radio open. |
| A step fails | Save the zip anyway and send it. The failure is listed in `summary.txt`. |
