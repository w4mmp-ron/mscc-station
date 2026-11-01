# For Stew: NR control as Off / Low / Med / High (client only)

From Ron, 2026-11-01. A suggestion for the client. **No server change is needed**: recv
already takes any level 1..100.

## Why

The NR level slider (MainWindow.xaml, 0..100, default 50) has far more steps than anyone
needs. With the spectral-subtraction NR (Pi recv 3.151, see
`NOTE-FOR-STEW-RECV-NR-SPECTRAL-2026-10-31.md`) the level only trades noise reduction against
warble. Three settings cover it.

## What the client sends

The same command as today: `CMD_SET_NR` (0xA3), int16 payload = level, 0 = off.

| Button | Payload | Background between words (measured, Pi) | Sound |
|---|---|---|---|
| Off | 0 | unchanged | |
| Low | 15 | about 7-8 dB quieter | natural, hardly any warble |
| Med | 50 | about 12 dB quieter | Ron's on-air test: "a touch of warble" |
| High | 85 | about 16 dB quieter | strongest, more warble; weak signals in heavy noise |

How recv uses the level (`denoise()` in `dsputils.c`): the floor for a noise-only bin is
-6 dB (level 1) to -20 dB (level 100), and the noise estimate is over-counted 1.0x to 2.0x.
Speech stays at about the same loudness; mainly the hiss between words drops.

The values 15 / 50 / 85 are a proposal and easy to change.

## Questions

1. OK with a 3-position control (buttons or a switch) in place of the slider?
2. OK with 15 / 50 / 85, or do you want other values?
3. A saved slider value (e.g. 37) on upgrade: map it to the nearest of Low / Med / High?
