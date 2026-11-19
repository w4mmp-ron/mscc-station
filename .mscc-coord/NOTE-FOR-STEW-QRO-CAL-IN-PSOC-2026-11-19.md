# Note for Stew: QRO (amplifier) calibration in the PSoC radios (2026-11-19)

Ron would like you to add the QRO calibration store to the PSoC firmware, the same way the Black
Pill got it in 3.24 (R11-19-0).

## What it is

Today each radio keeps the QRP power and the RX / TX I/Q calibration per band (CMD_CALIBRATION 0xC8).
The QRO (amplifier) calibration lives only in `amplifier_cal.ini` on the host. Black Pill 3.24 now
keeps it in the radio too, and the Pi ms-sdr reads and writes it. The PSoC radios should do the same,
with the **same protocol**, so ms-sdr talks to every radio the same way.

## Protocol (as the Black Pill 3.24)

Same opcode 0xC8, wValue = type << 8 | band, MSCC band order (0 = 10 m ... 9 = 160 m, 10 = 630 m,
11 = 2200 m), little endian, exact length or stall.

- **Type 8 = QRO value**, int16, 2 bytes, read and write. Same numbers as `amplifier_cal.ini`
  (-99 = not calibrated, otherwise about -99 to 0). A write marks the band as set and the radio
  stores it by itself, as for type 3 (power).
- **Type 9 = QRO valid mask**, uint16, 2 bytes, **read only** (a write stalls). Bit N = band N set.
- **Type 5 (the valid masks) stays 6 bytes.** Don't add the QRO mask to it: older ms-sdr builds read
  exactly 6 bytes.

## Where it could go (Ron's earlier look at the MKII EEPROM, code read only)

PSoC MKII (PTT / ATU, 3.238 and later): EEPROM 2 KB = 128 rows of 16 bytes. Rows 0, 1 and 8-19 are
used; the calibration row per band is full (only byte 14 spare). Plan: 12 new QRO rows **20-31**, one
row per band, written with the same one-row-per-band state machine as the existing calibration
(`Cal_Main`, one row per pass, never while transmitting). Please check the other PSoC trees that
have CMD_CALIBRATION for free rows the same way.

## The server side

ms-sdr only sends type 8 / 9 to a radio it knows has the store. Today that check
(`Radio_has_qro_store()` in `rpi/ms-sdr-linux/source/power_calibration.c`) is "Black Pill, firmware
minor 24 or later". ms-sdr tells the radios apart by the minor number: **PSoC = minor 200 or more,
Black Pill = 1 to 199.**

When your PSoC images have it, tell Ron the first PSoC version per radio that has the store. He adds
the second line to the check on the Pi, for example "PSoC and minor >= 249". The rest of ms-sdr
(startup read, the radio is the master, file values copied into an empty radio except -99, slider
save after 5 s out of transmit) already works for any radio that passes the check.

How it was tested on the Black Pill (worth repeating on a PSoC radio): 12 file values copied into the
empty radio; a server restart read them back; a 20 m recalibration reached the radio; the old file
put back -> the radio's value won; after a rig power cycle all 12 came back from the radio.

Details of the Pi side: `NOTE-FOR-STEW-PI-SERVER-CHANGES-NOT-ON-WINDOWS-2026-11-18.md`, section
R11-19-0.
