# For Stew — Solidus power calibration: it was manual

From Ron, 2026-10-14. Answers point 4 of `NOTE-FOR-RON-IQ-PHASE-AUTOCAL-2026-10-13.md`
(the part marked "no answer yet" in `NOTE-FOR-STEW-IQ-PHASE-ANSWERS-2026-10-13.md`).

## Answer

Solidus did not set its power automatically. Ron cannot find any such code, and the copy of
Solidus in the tree (`Solidus/`) only has manual calibration. An automatic power calibration
using the SWR meter would have to be built from scratch.

## What Solidus has

All of it is set by hand with a slider:

- **QRP drive per band** (`Solidus/servers/ms-sdr-linux/source/power_calibration.c`):
  the rig goes into TUNE on a fixed calibration frequency for the chosen band (for example
  14.010 MHz), the user moves a slider while watching the power, and the value is saved per
  band in `power_cal.ini` (`POWER_LEVEL=`). This is the same scheme as today.
- **100 W amplifier (Potentia):** a second per-band file, `amplifier_cal.ini`, holds a
  correction for each band, also set with a slider. Trans uses it as a multiplier on the
  user's power setting.
- **Client:** a band list, TX ON, CALIBRATE, a slider, ACCEPT, and a dummy load warning
  before keying.
- **SWR / power meters (Mensuro and the WiFi SWR meter):** they read forward and reflected
  power and have their own zero calibration, but their reading is never used to set the
  drive. There is no loop that steps the power until it reaches a target.

## What could be reused

Only the outline: the per-band files and "TUNE on a fixed calibration frequency per band".
The control loop, the safety limits and the timing do not exist yet.
