# For Stew: new noise blanker in recv (Pi recv 3.152)

From Ron, 2026-11-01. Your Windows recv has the same noise blanker as the old Pi one
(`mscc-ui/windows-work-tree/SDRcore-recv/sources/blanker.c`, identical file). Suggest you port
the new one. No client change is needed for it to work.

## Why it was changed

The old blanker was hard to set. Its threshold was a fixed amount above the signal level, so
the same slider position did different things on a quiet band and on a noisy one. It also
followed the signal so quickly that it nearly followed the pulse itself.

Also found: the client's Pulse slider is labelled "uS" (10..510), but recv used the number as
96 kHz samples. So the real width was about 10 times the label (510 = 5.3 ms of blanked audio).

## What it does now

- **Threshold is relative.** A pulse is anything more than 4 to 8 times the band's average
  level (measured over 2 ms, pulses left out). The same setting works on any band.
- **Threshold slider** (client 1..1009): left end = 8x (blanks only big pulses), right end = 4x
  (blanks more). Middle = 6x, a good start. Below 4x it would start blanking plain noise.
- **Pulse slider is now really microseconds.** 10..510 us. Good start: about 200 us. Below about
  42 us recv uses 42 us (4 samples), because a pulse at the radio is never shorter than that.
- **Signals are never blanked.** Anything that stays above the threshold longer than the pulse
  width (2 ms at most) is a signal, not a pulse, and is left alone. The audio is delayed a little
  (under 1 ms at 200 us) so this is decided before the audio is played.
- **Smooth blanking.** The gain goes down smoothly before the pulse and comes back after it, and
  stays down through a burst of pulses. No clicks from the blanker itself.
- **Protects itself.** If it blanks more than 400 pulses a second it becomes less sensitive on
  its own, and goes back when the rate drops.
- **Moving a slider does not interrupt the audio.** (The old one restarted on every slider
  step and lost a few ms of audio each time.)
- **Log line** every 10 s while NB is on, and when it is turned off:
  `Blanker . 10.0 s: 1.3 pulses/s (biggest 25x), too long 0.0/s (biggest 0x), near 1.5/s (biggest 5.9x), K 6.01 ...`
  pulses = blanked; too long = left alone as a signal; near = just under the threshold.

## Results (Pi, 20 m, 2026-11-01, Remote Phones captures)

Counting clicks in the audio that stand out more than 25x from the audio around them:

| | NB off | NB on |
|---|---|---|
| Big clicks per second | 0.41 | 0.05 |
| Biggest click | 35-60x | 26x (60 s run, 200 us, middle) |

About 9 of 10 big clicks removed, no gaps or holes in the audio, nothing "too long" in any log
interval. Offline tests (noise + made-up pulses + strong SSB signals): no false blanking at the
middle setting, a strong signal that starts instantly is not touched.

## To port

Copy from the Pi tree:

- `rpi/SDRcore-recv-linux/sources/blanker.c` (rewritten, plain C, no Linux-only parts)
- `rpi/SDRcore-recv-linux/sources/sdrcore.h` (only the `blanker_state` struct changed)

Nothing else: same opcodes (0x80 / 0x81 / 0x82), same `udp_thread.c` handling, same call in
`complex_shift()`.

## Questions

1. Client: change the Pulse slider minimum from 10 to 50 us? Below about 42 us it makes no
   difference.
2. Client: show the threshold as Low / Med / High instead of a number? For example
   Low = 8x, Med = 6x, High = 5x, which would send 1, 500 and 750 (no recv change).
3. Client: slider values 1001..1009 all mean the same (4x). Make the maximum 1000?
4. The startup default is still "NB on" until the client sends its setting (as before). Should
   the client always send the NB on/off state when it connects?
