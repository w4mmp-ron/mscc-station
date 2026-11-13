# remote_mic changes — note for Stew

**From:** Ron (changes by Claude), 2026-09-25
**Commit:** `9bb4be2` (moved into `rpi/` in `21b3024`, pushed)
**File:** `rpi/SDRcore-trans-linux/sources/remote_mic.c` (+ one change in `main.c`)
**Status:** compiled and simulated. **Not yet tested on air** with remote audio.

## Why

Likely cause of the Pi-host FT8/JT65 "skirt bump" (cmd-039 context).

The old 48 kHz → 96 kHz fill changed its read rate in steps of **0.8 % and 2.4 %**
(`step` 0.496 / 0.504 / 0.488 / 0.512) whenever the ring level crossed 1/8, 1/4,
5/8 or 3/4. That is a pitch shift: a 1500 Hz tone moves 12 Hz or 36 Hz. FT8 tones
are 6.25 Hz apart.

Packets arrive in 10 ms bursts, so the ring level swings ~480 frames. When the
average level sat near a threshold, the step flipped on and off about every 10 ms:
FM with sidebands near ±100 Hz. The two clocks drift slowly, so the level only
reaches a threshold now and then — clean for a while, then a bump. That matches
what you saw.

## What changed

1. **Fill rate:** a smooth trim of at most **±300 ppm** (≈0.5 Hz at 1500 Hz)
   keeps the ring near **100 ms** (4800 frames @ 48 kHz). No more steps.
2. **Start and underrun:** output holds (silence at start) until the ring
   reaches 100 ms, then plays at normal pitch. The old code started empty and
   played 2.4 % then 0.8 % flat for the first ~7 s.
3. **Too much queued (> 250 ms):** skip the oldest audio back to 100 ms, once.
   No speed-up.
4. **TUNE, two-stream mode** (`main.c` `sdrIqPlayOnlyCallback`): the remote ring is
   now drained during TUNE. Before, it filled up, and the next TX started with
   ~340 ms of stale audio played sharp for ~9 s.
5. **No log writes from the audio callback.** The callback only counts events;
   the UDP receiver thread writes them (it wakes every 100 ms even with no packets).
   An SD-card stall inside the callback could itself starve the I/Q output.

Not changed: MSA1 format, port 9101, sample rates, gains, the WPF sender.
cmd-034 (clear-on-TX / fixed 2:1) is not needed for this and was not implemented.

## Log lines (cmd-039 names changed)

`grep "remote_mic EVENT" ~/.local/mscc/sdrcore-trans.log`

| EVENT | Meaning | Expect |
|---|---|---|
| `primed` | Ring reached 100 ms; playback started | Once after each switch to Remote |
| `hold_last reprime` | Ring ran empty; holding and refilling | Rare (network gap > 100 ms) |
| `resync skipped=` | > 250 ms queued; skipped back to 100 ms | Rare (after a stall + burst) |
| `overflow dropped=` | Ring full (nothing reading it) | Only when not in Remote mode |
| `udp_gap gap_ms=` | No packet for ≥ 50 ms | Network hiccup |

**Removed:** `adaptive step=` and `low_occ` (no longer meaningful).

Periodic summary (`remote_mic: pkt ok=...`, every 500 packets): `occ` should sit
near **4800**, `step` between **0.49985 and 0.50015**, `under=0`.

## If you have uncommitted cmd-039 edits on the Pi

`git pull` will conflict in `remote_mic.c`. Keep the pulled version; the cmd-039
logging is covered by the EVENT lines above.

## To test

Win client → Pi host, Remote Digital, FT8/JT65 full TX with the SA running. Look
for the bump; grep the EVENT lines at the same time.
