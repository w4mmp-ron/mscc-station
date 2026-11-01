# For Stew: one ALC ceiling for voice and digital?

From Ron, 2026-10-31. About your trans 3.152 (`e7e4056`): ALC ceiling 1.0 x drive for digital,
0.45 for voice.

Question: why does voice keep 0.45?

## What I found on the Pi

SSB voice on the Proficio MKII only reached about 40 W of 100. Raising the mic gain didn't
help: the ALC held the peaks at 0.45 x drive. TUNE drives at 0.8, so voice peaks could reach
only about 56% of the TUNE amplitude, which is about 32% of the power.

Pi trans 3.149 (pushed `1e3ecb4`) now uses one ceiling, 0.89 x drive, for voice and digital
alike. After a QRP and QRO power recalibration, the output power is right.

## How I see it

WSJT and voice are the same thing to the transmitter: audio feeding the rig. A WSJT tone and a
voice peak at the same level drive the PA the same way.

The ALC is there to stop overdriving the rig. So its ceiling should be the rig's full
calibrated power. Below that, the mic gain (or the WSJT audio level) sets how hard the rig is
driven.

## Options

1. One ceiling for both on Windows too (your 1.0, or 0.89 as on the Pi).
2. Keep 0.45 for voice, if your analyzer showed a reason that a single digital tone doesn't
   have. If so, please tell me what you saw and I'll look at it on the Pi.

## Could you check

SSB voice with a 0.89 ceiling (the Pi's value) on your setup: power and splatter on the
Signal Hound.
