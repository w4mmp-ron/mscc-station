# For Stew — Ron's answers: I/Q phase, Pi phase term, power cal, UP 24 kHz

From Ron, 2026-10-13. This answers `NOTE-FOR-RON-IQ-PHASE-AUTOCAL-2026-10-13.md`.

## In short

- **I/Q phase:** Ron added a transmit phase correction to sdrcore-trans on his Pi and
  tested it on 10 m. It works. Most of the improvement came from setting the amplitude
  more carefully, and phase added about 8 dB more. Whether to take phase further is up to you.
- **Does the Pi have a phase correction?** It did not, in either recv or trans. Trans on
  Ron's Pi now has the test version described below. It is committed together with this note.
- **Automatic power calibration (Solidus):** Ron has not answered this yet. He will come
  back to it.
- **UP 24 kHz:** leave it as it is. Ron does not want either of the two options.

## How Ron measured

Ron has an I/Q Balance Detector on the transmitter output. Its audio goes into a sound
card on his PC, and a small program shows two tones from it:

- 12 kHz, caused by LO leakage
- 24 kHz, caused by the image

The lower the 24 kHz tone, the smaller the image. All the figures below are the level of
that tone as the sound card reads it. They are good for comparing settings, but they are
not dB below the carrier.

## What Ron found on 10 m

Only 10 m has been tried, on one rig.

1. **The saved amplitude value was 5 steps off.** It was -69; the best value is -74.
   Moving to -74 lowered the image by about 12 dB.
2. **The best point is only about one step wide**, for amplitude and for phase. Moving the
   slider at normal speed goes straight past it. It can only be found one step at a time,
   with a short pause on each step.
3. **The phase error is very small**: 0.30 degrees. Ron's view is that this is what the
   hardware design leads you to expect. Even so, because the best point is so narrow,
   correcting it lowered the image by about 8 dB.

Final result: amplitude -74, phase -6 (-0.30 degrees). The image fell from -37.7 to
-45.5 on the meter and is now about 17 dB below the LO leakage tone. A rough estimate puts
it somewhere near 70 dB below the carrier, but that has not been measured directly.

## Automatic setting

Ron also wrote a program that does the adjustment by itself, `rpi/tools/tx-iq-balance/iq-auto.py`. You key the rig
from the TX IQ tab; the program steps the value, finds the lowest image and saves it in
`iq.ini`. It takes about one to three minutes per band. The 10 m result above came from it.

It needs the I/Q Balance Detector and a 96 kHz sound card. Ron's idea is that it could be
used to set up each rig once it is built and tested, so the rig ships with its own
`iq.ini`. That is only a suggestion; it is up to you.

## What was changed in trans on Ron's Pi

For your reference, in case you want to use any of it. Six files in
`rpi/SDRcore-trans-linux/sources/`. Committed together with this note; the version number is still 3.142.

- **The correction:** it is applied to the transmit output. With phase set to 0 the
  output is exactly as before.
- **Range and step:** plus or minus 20 degrees, in steps of 0.05 degrees (your example
  step). Plus or minus 5 degrees would be enough for what was seen here.
- **Saving:** each line in `iq.ini` now has an `IQ_PHASE=` value next to `IQ_OFFSET=`.
  It is read by name, so older files without it still load, with phase 0.
- **Commands:** `0x2A` sets the phase and `0x2B` reads it back. These numbers were picked
  because they are free in trans and in ms-sdr. They have not been agreed with you, so
  change them if you wish.
- **Save command:** the existing save command `0x57` now also works outside a TX IQ
  session, as long as a band is set.
- **Other programs that write `iq.ini`:** mscc-init and the Windows and Ubuntu versions
  of trans read the new value without trouble, but leave it out the next time they save
  the file.

Nothing was changed in recv, ms-sdr or the client. Receive phase has not been tried.

## Phase from here

Ron is keeping the phase code in trans on his Pi as it is. He is not asking for a phase
slider in the client or for any change in ms-sdr. That decision is yours.

If you want to look further (other bands, other rigs, or receive), the Pi version is there
to test with, and Ron can take measurements with the detector.

## Automatic power calibration (Solidus)

No answer yet. Ron will come back to how Solidus did it and whether any of that code can be
reused.

## UP 24 kHz

Leave it as it is. No changes. Ron does not want the display-only slide or moving the
receive window to the mirror image.
