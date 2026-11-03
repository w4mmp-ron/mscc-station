# For Stew: how fast can a PSoC board key non-QSK CW cleanly?

From Ron, 2026-11-03. A request for an analyzer check, when you have time. No rush.

## Why I'm asking

When I tell people about CW on any PSoC daughter board (Proficio MKII PTT / ATU / Legacy and both
Geminus boards), I'd like to give a top speed for non-QSK CW. All the current PSoC trees shape
non-QSK CW with the PCM3060 DAC soft mute. The soft mute's fixed ramp time sets a speed limit.

## What the numbers say (worked out, not measured)

From the TI PCM3060 datasheet (SLAS533B, pages 32 and 34):

- The soft mute steps the DAC attenuator 0.5 dB every 8/fs, in both directions. The rate is fixed.
- At 96 kHz, full to mute is 201 steps x 83 us = 16.75 ms. Mute to full takes the same.

The firmware starts the unmute 14 ms before each delayed key-down (`DAC_RISE_LEAD_MS`) and the mute at
each delayed key-up. In CW the space between elements is one dit (1200 / wpm ms), so:

| Speed | Space | How quiet the space gets |
|---|---|---|
| 39 wpm or slower | 30.8 ms or more | full mute |
| 58 wpm | 20.7 ms | about -40 dB |
| 69 wpm | 17.3 ms | about -20 dB |
| 86 wpm or faster | under 14 ms | no gap at all (spaces merge, as the cw.c comment says) |

## The open question

The datasheet says that while an attenuation change is in progress, a new attenuation command waits
until it finishes. It doesn't say whether the mute bit works the same way.

- If the mute bit **can** reverse in the middle of a ramp, keying should stay clean up to about 58 wpm.
- If it **can't**, then above 39 wpm each new element would start late (after the mute finishes) and
  come out short.

## The check

Non-QSK CW, steady dits at **45 wpm** and at **55 wpm**, on your analyzer in zero span:

1. Do the spaces go fully quiet?
2. Are the elements the right length, or are they short or late?

That tells us which case it is. Until then I'll say: "PSoC daughter boards, non-QSK CW: clean keying
up to at least 39 wpm." QSK CW isn't shaped on the PSoC, so this limit doesn't apply to QSK.
