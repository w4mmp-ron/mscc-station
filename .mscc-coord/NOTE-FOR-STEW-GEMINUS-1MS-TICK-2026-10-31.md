# For Stew: Geminus 1 ms tick, main loop or hardware timer

From Ron, 2026-10-31. Answer to question 3 in `NOTE-FOR-RON-CAL-IN-RADIO-GEMINUS-2026-10-31.md`.

Short answer: fine to keep for now. For dependable CW, add a 1 ms hardware timer interrupt like
the Proficio's.

## How the two radios do it

- **Proficio MKII-PTT:** `Key_Delay_Timer` (fixed-function timer on the 10 kHz CW clock,
  period 10) fires `KEY_DELAY_interrupt` every 1 ms (`cw.c`). The interrupt samples the key
  lines, plays them to the PA 20-40 ms late (non-QSK key delay), asks for the DAC soft mute
  around each element, and counts `E_ms_tick` for `TX_Main`. Its timing is exact no matter
  what the main loop is doing.
- **Geminus MKII 2.153:** no timer. `E_ms_tick` is counted in the main loop from
  `STATUS_BEAT`, which changes every 0.5 ms. The CW key sampling and replay run from that tick
  too.

## What can go wrong on the Geminus

A beat change is only counted if the main loop comes back around within 0.5 ms. Any pass that
takes longer (an I2C write such as the DAC mute, USB work) misses ticks, so the count runs
slow. It can never run fast.

- **TUN / PTT:** a slow count only makes the delays longer (25 ms becomes, say, 27 ms). The
  relays still never switch with RF on. Fine as it is.
- **CW:** each key-down and key-up goes through the 30 ms delay buffer on this tick, so missed
  ticks stretch single elements unevenly. You measured it clean, so it is probably fine in
  practice, but it depends on the loop never stalling. Most likely to show at high speed or
  with other activity at the same time (band change, frequency changes, USB traffic).

## Suggestion

1. Keep 2.153 as it is for now; it measured clean.
2. When there is time: add a 1 ms timer interrupt as on the Proficio (a fixed-function timer
   on an existing clock), and move the key sampling, the delay buffer replay and `E_ms_tick`
   into it. Leave the I2C write for the DAC mute in the main loop, with the interrupt only
   setting a request flag, as the Proficio does.
3. A quick check before that: time the longest main-loop pass on the Geminus (toggle a spare
   pin and scope it). If it never gets near 0.5 ms, the main-loop tick is safe.
