# For Ron: QRP CAL status lamps should come from the radio

From Stew, 2026-11-01. A proposal, with no code yet. Stew is testing the new firmware on all
the radios first (MKII ATU 4.243, Ultimus MKII PTT 8.242 / MKII ATU 7.243). Nothing changes in
the client or the servers until that is done.

## The problem

The CALIBRATION STATUS lamps on the QRP CAL tab (green = calibrated, white = default) are
stored only on the PC, in `%LocalAppData%\MSCC-NET9\client-settings.ini`
(`PROFICIO_B*` / `GEMINUS_B*`, `PowerCalStatusStore.cs`). The calibration itself has lived in
the radio since 0xC8, so the two can disagree:

- A new or blank radio on a PC that already calibrated another radio shows that radio's green
  lamps.
- A calibrated radio moved to a fresh PC or Pi shows all white, even though its calibration
  loads from the radio and works.

## What Stew wants

| Situation | Lamps |
|---|---|
| New or blank radio | all white (defaults in use) |
| Radio calibrated on one PC, then moved to another PC / Pi | green on every band calibrated |
| Bands skipped (e.g. the antenna doesn't cover them) | white |

## Why it fits what we have

The radio stores only real calibration, never factory values (decision 10-30). So the
valid mask (0xC8 type 5: three uint16 masks, RX IQ / TX IQ / power, bit N = band N in MSCC
order, 0 = 10 m ... 9 = 160 m, 10 = 630 m, 11 = 2200 m) means exactly "the user calibrated
this band". ms-sdr already reads those masks at start (`Radio_cal_load_power()` /
`Radio_cal_load_iq()` on Windows).

## Proposal

1. **ms-sdr (Windows and Pi):** send the three masks to the client at connect, and again after
   each band is written to the radio (and after a clear, see 3). One new opcode, payload
   6 bytes = 3 x uint16 little endian [RX IQ, TX IQ, power]. Opcode number: your pick, so it
   doesn't clash with anything on the Pi side.
   With a radio that has no store (older firmware, type 5 stalls), send nothing, or a
   "no store" flag. The client then keeps using its local file as it does today.
2. **Client (WPF and Avalonia):** QRP CAL lamps follow the power mask. Bands 2200/630 map to
   bits 11/10, 160...10 m to bits 9...0. `client-settings.ini` is used only when no masks
   arrive. After a QRP CAL save, the lamp goes green when ms-sdr resends the masks showing
   the radio wrote it. That takes a few seconds on Windows, because the save waits about two
   Flusher ticks and is not written while transmitting. So the lamp shows what is really in
   the radio.
3. **Reset to factory:** with your type 6 clear back as a control READ (band or 0xFF, your
   NOTE-FOR-STEW-CAL-CLEAR-2026-10-31), ms-sdr clears, rereads the masks and resends them,
   and the lamps go white. That also fixes the guide's "reset to factory doesn't stick"
   finding.
4. Optional later: the same masks could light TX IQ / RX IQ status lamps. AMP CAL is not in
   the radio, so its lamps stay PC-based.

Questions:
- OK with one opcode carrying all three masks, sent by ms-sdr unasked (at connect and after
  each change)? Or would you rather have the client ask for them?
- Which opcode number is free on the Pi side?

## Answers to your Windows guide questions (NOTE-FOR-STEW-WINDOWS-GUIDE-2026-11-01)

1. **Firmware table (2.6):** Stew would rather not tie the guide to exact versions, since they
   change. List the current versions with a note that a newer one may ship with the
   installer, and that the radio's version shows in MSCC. As of today: Legacy 1.236,
   Geminus MKII 2.153, MKII PTT 3.242, MKII ATU 4.243, Ultimus Legacy 6.236, Ultimus MKII ATU
   7.243, Ultimus MKII PTT 8.242 (plus 5.120 as you have it). 4.243, 7.243 and 8.242 are built
   and pushed but not flashed yet. Stew is testing them now.
2. **Does the calibration in the radio survive a firmware update?** Not checked yet. Stew
   will calibrate a band, reflash, and check it on each radio as he goes through the firmware
   testing. Answer to follow.
3. to 5. (ALC target reading, AMP CAL target, new pictures): later, after the firmware
   testing.

Your ALC one-ceiling question, the NR / NB ports and the NR Off/Low/Med/High suggestion all
look good to Stew. They are queued after the firmware testing, so no code until then.
