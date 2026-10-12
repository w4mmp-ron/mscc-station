# For Stew — WPF 10.11.3 against the Pi: spectrum and S meter agree

From Ron, 2026-10-12. For your records, nothing to do.

## Result

With your client 10.11.3 on the Pi host (mscc 1.0.63, recv 143), the spectrum and the S meter
dBm readout agree and track.

| SMSG setting | S meter readout | Spectrum carrier peak | Difference |
|---|---|---|---|
| S9 | -77 dBm | about -75 to -76 | spectrum 1 to 2 dB higher |
| S4 | -104 dBm | about -102 (range -99 to -104) | spectrum about 2 dB higher |
| Step | 27 dB | about 26 dB | 1 dB |

- The true dB scale is confirmed on the Pi: a 27 dB step on the meter is about 26 dB on the
  spectrum. On the old scale the same step showed as about 13 dB.
- Your dB CAL centre of -83.6 is right within about 2 dB, which is inside the reading error
  of this test. dB CAL was at 0.
- The Pi needed nothing new for 10.11.3.

## How it was measured

- Multus SDR SMSG signal generator, 20 m, CW, VFO 14.0667 so the carrier shows beside the
  tuning marker. Two screen recordings, 12 to 14 s each.
- S meter: the dBm readout, steady in every sampled frame.
- Spectrum: the carrier tip read in every frame (409 and 349 frames) against the grid lines.
  One pixel is 0.5 dB, so the reading error is about 1 dB.

## Things to know

- The generator's S9 setting gave -77 dBm, about S8, not -73. Its S9 to S4 step is 27 dB by
  both readings, not the nominal 30. This test cannot say whether the generator or the rig is
  low; it only shows that the two displays agree.
- Videos, if you want them: `Video_2026-10-12_123922.wmv` (S9 setting) and
  `Video_2026-10-12_124321.wmv` (S4 setting), on Ron's PC.
