# For Stew — TX I/Q values measured on Ron's Proficio MKII, for the factory table if you want them

From Ron, 2026-10-14. Follows `NOTE-FOR-STEW-IQ-PHASE-ANSWERS-2026-10-13.md`.

## What Ron did

Ron set the transmit I/Q balance on every band from 10 m to 160 m on his Proficio MKII
with the automatic program `rpi/tools/tx-iq-balance/iq-auto.py`. It finds the lowest image
one step at a time, for amplitude and phase. The resulting file is in the tree:

`rpi/tools/tx-iq-balance/iq-ron-proficio-mkii-2026-10-14.ini`

## Compared with the factory table

`factory/iq/proficio-mkii/iq.ini` against Ron's measured values (amplitude, `IQ_OFFSET`):

| Band | Factory | Ron's rig | Difference | Ron's phase |
|---|---|---|---|---|
| 10 m | -80 | -74 | 6 | -6 (-0.30°) |
| 12 m | -67 | -67 | 0 | +1 |
| 15 m | -48 | -45 | 3 | +5 (+0.25°) |
| 17 m | -40 | -39 | 1 | +2 |
| 20 m | -24 | -22 | 2 | +1 |
| 30 m | -7 | -8 | 1 | +2 |
| 40 m | 6 | 4 | 2 | +1 |
| 60 m | 13 | 10 | 3 | 0 |
| 80 m | 19 | 16 | 3 | 0 |
| 160 m | 25 | 22 | 3 | 0 |

630 m and 2200 m were not measured (0 in both).

## Things to know

- **The factory table is already close:** within 3 steps on every band except 10 m. It is
  closer than the values Ron's rig had before (those were up to 10 steps off).
- **A few steps still matter.** The best point is only about one step wide. On Ron's 10 m,
  5 steps off left the image about 12 dB higher than at the best value.
- **This is one rig.** Ron does not know how much the best values differ from rig to rig.
- **Phase:** the factory table has no `IQ_PHASE=`. On Ron's rig the phase is at most 0.30°,
  and 0 from 60 m down. Adding the field to the factory file only means something to a
  trans that has the phase correction (only the Pi so far).

## Options

1. Leave the factory table as it is.
2. Use Ron's amplitude values for `proficio-mkii` (with or without the phase field).
3. Set each rig with `iq-auto.py` when it is built and tested, and ship it with its own
   `iq.ini`. That needs the I/Q Balance Detector and a 96 kHz sound card at your bench.

Your choice. Nothing in `factory/` has been changed.
