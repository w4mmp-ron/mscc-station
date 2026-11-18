# Note for Stew: series resistors on the bit clock and word clock (2026-11-18)

Stew, a request for the next spin of the Black Pill daughter board: add series resistors on
the bit clock (BCK) and word clock (LRCK), the same way as the MCLK resistors already in
`psoc-replacement-stm32/proficio-stm32f411-25MHz/docs/BOARD-SPIN-NOTES.md` (item 1).

## The change

```
PB13 ──┬── 22-47 Ω ── BCK1  (J5 A12) ── codec pin 5
       └── 22-47 Ω ── BCK2  (J5 A14) ── codec pin 10

PB12 ──┬── 22-47 Ω ── LRCK1 (J5 A20) ── codec pin 4
       └── 22-47 Ω ── LRCK2 (J5 A16) ── codec pin 11
```

- One resistor per line, placed at the pill end, as close to PB13 / PB12 as possible. The lines
  split at the pin, before the resistors (same as PA3).
- Start with 22 Ω. Your scope shows whether the edges still look clean at the codec pins.
- Optional, only if there's room: a pad for a small cap (about 22 pF) to ground at the
  connector end of each line, left unfitted.

## Why

The transmitter's own RF upsets the receive stream from the codec. False edges on the bit
clock push the receive bits out of step, and the spectrum goes all yellow (S9+40, no audio).

- Seen on 11-16 (Windows) and 11-17 (Pi) after a TUNE into a high SWR, and once on 11-17
  during normal FT8 on 20 m.
- A raw capture on 11-17 showed every sample shifted one bit, which means the codec and the pill
  were one bit clock out of step.
- Ron's test on 11-18: about 10 TUNEs at SWR 2.3, up to 169 W. The pill counted about 4,600
  receive frame errors.

## What the firmware does now

Pill firmware 3.22 spots the slip and restarts I2S and the codec by itself (about 0.2 s of
receive lost). In Ron's 11-18 test it recovered 7 times and the spectrum stayed normal.

That only covers up the problem. The resistors should stop it at the source. The firmware
recovery stays in as a backstop.

## How we'll check the new board

Run the same high-SWR TUNE test and compare the pill's receive frame errors with about 4,600
(the `frame_err` count in `audio-stats.py`). Near zero means the resistors fixed it.
