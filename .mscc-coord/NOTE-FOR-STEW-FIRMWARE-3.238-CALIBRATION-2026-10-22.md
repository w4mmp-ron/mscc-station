# For Stew — radio firmware 3.238: calibration stored in the radio

From Ron, 2026-10-22. For your information. Commit `6680c11` (source + release binaries in
`radio-psoc-firmware/Proficio-MKII-PTT/Release/`, `Proficio-MKII-PTT-20261022.hex` = 3.238).

## What it does

The radio keeps RX I/Q, TX I/Q and power calibration for each band, so the values go with
the radio instead of living only in the ini files on one computer. The radio only stores
them; the server applies them.

One opcode, `CMD_CALIBRATION` 0xC8 (see `basic-plus.h`). wValue = type << 8 | band.
Read = get, write = set, exact length or the request stalls. Little endian.

| Type | Data | Notes |
|---|---|---|
| 1 RX I/Q | int16 gain + int16 phase (4 bytes) | |
| 2 TX I/Q | same | |
| 3 power | uint16 (2 bytes) | |
| 4 status | read only: [write status, loaded at boot] | |
| 5 valid | read only: 3 x uint16 masks [RX, TX, power], bit N = band N | 0 is a real value |
| 6 clear | removed in 3.239 (stalls) | was a no-data request like the old save |

Bands in MSCC order: 0 = 10 m ... 9 = 160 m, 10 = 630 m, 11 = 2200 m.

## Why 3.238 (3.235 to 3.237 had a save command)

Up to 3.237 a set went to RAM and a separate save command (type 4 write, no data) wrote a
9-row record to EEPROM. On Ron's MKII the radio's USB then hung at the next audio interface
change (calibration band select, or a client connecting), 1 to 3 s after a save. It needed a
power cycle. Band changes with no saves never hung. 3.237 (no temperature update before
the save) did not fix it.

3.238 does it the way the PPM update already does: when a value arrives, the radio writes
that band's EEPROM row by itself (one row per band, rows 8-19, one `EEPROM_StartWrite`).
No save command: type 4 write now stalls. Writes wait while the rig is transmitting.

## Tested by Ron on his MKII

- Values written, power cycle, all 10 bands back at boot.
- Full power calibration of all 10 bands with the Pi ms-sdr: no hang. One value went out
  during a band change, also fine.

## Things to know

- After flashing 3.238 the radio has no calibration (the 3.235-3.237 record is not read).
  On the Pi, write the values first, or ms-sdr sets every band to factory values at start.
- Pi ms-sdr (power only so far): 5 s after the slider stops, and out of TX, each changed
  band is sent once. At start the radio's values win over `power_cal.ini`; bands the radio
  lacks get the factory value for that radio type (the file may hold another radio's values).
  The file is never copied into the radio.
- The Black Pill firmware still has the save command (type 4 write). Options: leave it, or
  change it to work like 3.238 so both radios behave the same. Your call.
- Windows / Ubuntu ms-sdr and the client do nothing with 0xC8 yet. Options, if and when you
  want them: the same as the Pi ms-sdr, or nothing (radios still work as before).
- Test script on Ron's Pi: `~/cal-test.py` (pyusb, EP0 only, runs with the servers up).
