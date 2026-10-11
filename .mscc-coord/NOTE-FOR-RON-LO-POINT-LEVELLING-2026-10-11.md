# For Ron - LO point (DC) levelling: Windows test results, paused (2026-10-11)

From Stew (n8vet), written up by Build Commander. Nothing to do yet; this is on hold.

## What was tested
- Windows SDRcore-recv 3.146 (cmd-071) = your Pi 1.0.62 recv code: `panadapter.c` copied as-is,
  `dsputils.c` a8094ee hunks, `DC_BLOCK_A` 0.9999, `PAN_DC_HALF_BINS` 16 (375 Hz each side),
  `PAN_DC_REF_BINS` 16, `PAN_DC_ALPHA` 0.01, after-TX hold 47 FFTs. Client 10.11.2.
- Stew's rig on NEW-HP, Geminus/MF around 0.475-0.477 MHz, signal generator at about -80 dB.
- Screenshots on NEW-HP: `OneDrive\Pictures\Screenshots\Screenshot 2026-10-11 124812.png`
  and `...125003.png` (not in the repo).

## What Stew saw
1. **Slice edges show.** The levelled +/-375 Hz slice is a lighter band in the waterfall with a
   thin line at each edge.
2. **Signal parked at the LO point:** it is scaled down in under about 10 s, but a hump stays on
   the trace.
3. **Signal removed:** the hole left behind fills back in within about 20 s.
4. **Fine tuning near the LO point:** tune 100 Hz and the signal appears, then gets buried again.
   A signal that happens to sit near the centre jumps up and down as you tune.
5. After-TX dip: much better than before (as you found).
6. Smoothing (`PANADAPTER_AVERAGE`) and the gentle DC blocker: no problems found.

Stew: "better than a hole but not sure if we found the correct way to handle this yet."
PowerSDR seems to handle it better; ours is thought better than HDSDR. How either of them
handles the DC/LO spike was not looked into.

## Why (from the code, not measured)
Each of the 32 slice bins learns a slow (about 2 s) average and is scaled down to the quieter
neighbours. A signal inside the slice is learned as "hump" and pushed down, then leaves a hole
until the averages relearn. Any adaptive per-bin scaling will pump like this; a narrower or slower
slice only changes how it pumps.

## Idea discussed, not adopted
A fixed narrow fill: replace only +/-2-3 bins (about 50-70 Hz) with the neighbours' average every
frame, no learning (no pumping, no hole, signal hidden only within about 70 Hz of the LO).
**Stew's objection:** the width of the DC hump depends on each shack's power-supply noise and
ground loops, so a fixed narrow area may not work in every shack. Paused here; unusual case.

## State
- Windows recv 3.146 ships with your 16-bin levelling as-is (Pi parity).
- Ubuntu `linux/` recv still has the old code; it is being brought up to this same point next.
- Open: a better LO-point method (maybe width set from the measured hump, or a user setting),
  and how PowerSDR/HDSDR do it.