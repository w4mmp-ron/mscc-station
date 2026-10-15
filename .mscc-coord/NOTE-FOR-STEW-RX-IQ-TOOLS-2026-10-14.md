# For Stew — receive I/Q balance: three Pi scripts and Ron's measured values

From Ron, 2026-10-14. Follows `NOTE-FOR-STEW-RX-IQ-BAND-DEFAULTS-2026-10-12.md` (the idea of
per-band receive I/Q defaults). For your information; nothing here needs a client change.

## What Ron did

Three Python scripts, in the tree as `rpi/tools/rx-iq-balance/` (commit d2fed18). They run on
the Pi and need no change to recv, ms-sdr or the client:

- `rx-image-meter.py` reads the spectrum recv sends to ms-sdr (it copies the packets off the
  loopback link, so it needs sudo) and shows the carrier, the image and the image rejection
  in dB.
- `rx-iq-auto.py` does the same, steps I/Q OFFSET in recv by itself (0x52), keeps the best
  value and saves it (0x57). Uses the RX IQ tab session (START) for the band.
- `rx-iq-fill.py` fits a straight line through the calibrated bands and writes the other
  bands in `recv-iq.ini`.

## Ron's values (Proficio MKII, signal generator at S9)

| Band | Value | Rejection | At 0 | How |
|---|---|---|---|---|
| 80 m | 21 | | | measured |
| 40 m | 27 | | | measured |
| 30 m | 37 | about 53 dB | about 40 dB | measured |
| 20 m | 51 | about 51 dB | about 37 dB | measured |
| 160 m | 14 | | | from the line |
| 60 m | 24 | | | from the line |
| 17 m | 61 | | | from the line (beyond the measured bands) |
| 15 m | 70 | | | from the line (beyond the measured bands) |
| 12 m | 81 | | | from the line (beyond the measured bands) |
| 10 m | 91 | | | from the line (beyond the measured bands) |

Before this, every band on Ron's rig was 58.

## Things to know

- **The side matters.** The image that lands in the passband comes from a signal BELOW the
  LO point. Ron measured with UP 24 kHz ticked (VFO 24 kHz above the generator). Measured
  the other way (generator just below the VFO), the best values come out quite different
  (20 m: 94 against 51), because the correction is amplitude only.
- **The values rise steadily with frequency**, about 2.9 steps per MHz on Ron's rig. That
  bears on the per-band defaults idea: a line through a few calibrated bands may be enough
  for the rest.
- **The best point is broad** (about +/-4 steps within 1 dB), so a few steps off costs little.
  Around 51-53 dB the image is only a few dB above the noise; that is the limit of this
  measurement with the generator at S9.
- **This is one rig.** How much the values differ from rig to rig is not known.
- **Client, for your information:** the RX IQ tab slider does not show a value set by the
  script, and APPLY ALL sends the slider's value to all bands. Ron avoids APPLY ALL now.

## Options

1. Nothing; Ron keeps using the scripts on his rig.
2. Use the scripts at your bench for the rigs you build (they need a generator on the band,
   sudo on the Pi).
3. Later, if useful: a small recv command "send spectrum frames to me" would let the scripts
   run without sudo, and from a PC. Not needed now.
4. Put the same thing in the client. The client already gets the same spectrum (ms-sdr
   passes it on unchanged) and already sends the I/Q value and the save. So the RX IQ tab
   could have an AUTO button: it moves the I/Q value up and down by itself, watches the
   image on the spectrum, and stops at the lowest one. A second button could fill the bands
   that have no signal generator from the bands that were set. It would then work with
   every recv (Pi, Windows, Ubuntu), with no scripts and no sudo. The scripts show how it
   is done.

Your choice. Nothing in `factory/` or the client has been changed.
