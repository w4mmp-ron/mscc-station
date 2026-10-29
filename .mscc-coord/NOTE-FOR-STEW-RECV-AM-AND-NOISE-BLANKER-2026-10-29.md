# Note for Stew: two sdrcore-recv fixes for the Windows recv (AM audio, noise blanker)

From Ron, 2026-10-29. Pi commit **c9850ca** (sdrcore-recv 3.147). Both are in code the
Windows recv shares with the Pi (`SDRcore-recv/sources`), so the same edits should apply.
Your call whether and when to port them.

## 1. AM receive audio is distorted (filter removes the carrier)

**What Ron heard:** AM has never sounded good.

**Cause:** `sdrcore.c` initDSP builds the same filter for AM as for SSB:
`wsfirBP(low cut .. high cut)`. That real bandpass passes +/-(low..high) around 0 Hz and
blocks 0 Hz, where the AM carrier sits. The AM detector (`mag`, "Simple AM demod") then
has no carrier and full-wave rectifies the sidebands. Your Windows recv log shows it:
`New Mode A lastHR=[500,4000] filt=[500,8000]`, i.e. the carrier and everything within
500 Hz of it removed.

**Fix (two small edits, `sdrcore.c`):**
- initDSP: in AM use a lowpass 0 .. high cut:
  `if (opmode == MODE_AM) wsfirLP(hfilt, taps, W_HANNING, lastHRFiltHigh / samplerate); else wsfirBP(...as now...);`
- fastconv: after `mag`, a DC blocker (0.999, about 15 Hz at 96 kHz) so the carrier level
  does not go into the AGC / NR / notch as a large DC; state reset when leaving AM
  (`am_dc_x1`, `am_dc_y1`, `am_demod_inited`, same style as the FM ones).

**Result on the Pi:** Ron, 9.930 MHz AM broadcast: "sounds much better".

## 2. Noise blanker can leave the audio silent until restart

**What Ron saw:** turned the noise blanker on and lost all audio; not repeatable at will.

**Likely cause (from the code, not proven):** `initBlanker()` runs in the UDP thread and
rebuilds `nbstate` while the audio thread is inside `doBlanker()`. It sets `oindex = 0`
before the new `delaylen`; with a shorter width (default 30 -> client 10, delaylen 15 -> 5)
the read index can end up past the new length, the `==` wrap never hits, and the delay
line is read (and `iindex` written) past its 10000 entries. A NaN from there gets into the
AGC gain (a NaN passes the `> maxgain` / `< 0.01` limits) and the audio stays silent
until the recv restarts.

Also found: with the blanker on but not blanking, the audio is cut to 0.7 (-3 dB at
width 10): `gainramp[0]` is `1 - ga`, the comment says it should be 1.0.

**Fix (`blanker.c`, `blanker.h`, `dsputils.c`):**
- `initBlanker()` only stores the request (and logs); new `blankerPoll()` applies it in the
  audio thread, called at the top of `complex_shift()` before `doBlanker()`.
- Delay-line wraps use `>=`; width clamped 2..9998; `tripcounter` reset if past the width.
- `gainramp[0] = 1.0f` (resting gain).
- `doAGC()`: if the gain or `maxval` is not finite, reset it (1.0 / 0.0); NaN output samples
  become 0, so the audio cannot stay stuck silent.

**On the Pi:** running since 2026-10-29 with blanker on/off, width and threshold changes:
no errors, no audio loss so far (it was never repeatable, so "not seen again" is the test).

## Files to compare (Pi commit c9850ca)

`rpi/SDRcore-recv-linux/sources/`: `sdrcore.c`, `blanker.c`, `blanker.h`, `dsputils.c`
(`blankerPoll` call + AGC NaN lines only). The Windows `blanker.c` was the same as the
Pi's before this change (apart from line endings).
