# For Stew: recv NR replaced (spectral subtraction), Pi recv 3.151

From Ron, 2026-10-31. Your Windows recv has the same `denoise()` as the old Pi one (checked:
`mscc-ui/windows-work-tree/SDRcore-recv/sources/dsputils.c`, identical function). Suggest you
port the new one.

## What was wrong

With NR on, the audio got "really distorted". Measured on the Pi (Remote Phones capture, NR
switched on and off during one rag chew, level 50):

- a ~3.8 kHz whistle at the top of the 4 kHz filter (the strongest thing in the audio)
- peaks up to clipping (0.37 vs 0.20 with NR off)
- speech band 300-1000 Hz down by up to 10 dB

Cause: the old NR is an LMS "adaptive line enhancer". It boosts anything correlated over its
delay. Noise through recv's sharp audio filter is strongly correlated at the filter edge, so it
whistles. The step size is normalised by one sample's power, so it over-corrects at quiet moments.

## What replaced it

Spectral subtraction, the usual SDR method. Only `denoise()` in `dsputils.c` changed (the
`nr_delay()` helper is gone), plus the version. Same place in the chain (after the AGC), same
CMD_SET_NR opcode and level 1..100, no client change.

- 2048-point FFT at 96 kHz (21 ms blocks, 47 Hz per bin), 50 % overlap, square-root Hann.
- Per bin noise estimate: smoothed power, follows it down at once, may rise only 3 dB/s.
- Per bin gain: Wiener gain with the "decision-directed" SNR (this keeps musical noise low).
- Gain is between a floor and 1.0, never above: NR can only make things quieter.
- Level: floor -6 dB (level 1) to -20 dB (level 100), noise over-count 1.0 to 2.0.
- Adds 21 ms of audio delay while NR is on. Passes audio unchanged for the first 0.1 s after
  switching on, while it learns the noise.
- Uses the existing `jimfft()` / `jimifft()`, static buffers only.

Source: `rpi/SDRcore-recv-linux/sources/dsputils.c`, function `denoise()`. It is plain C with no
Linux-only parts, so it should drop into the Windows file as is.

## Result on the air (Pi, 40 m rag chew, level 50)

| | NR off | NR on |
|---|---|---|
| Noise between words | -41 dB | -53.5 dB |
| Voice level | -29.6 dB | -30.7 dB |
| Voice above noise | 11 dB | 23 dB |
| Peaks | 0.11-0.20 | 0.07-0.10 |
| Whistle | - | none |

Ron: "just a touch of warble, not bad ... much better than it was".

## Possible later tweak (parked)

The slight warble is the usual leftover of this method. Smoothing the gain over neighbouring
bins (about +/-100 Hz) should reduce it. Not done yet on the Pi.
