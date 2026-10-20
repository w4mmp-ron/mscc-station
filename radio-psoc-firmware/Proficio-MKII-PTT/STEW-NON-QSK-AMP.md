# Non-QSK amplifier keying — note for Stew

**From:** Ron (W4MMP)  
**Date:** 2026-10-19  
**Firmware:** Proficio MKII PTT 3.234 (`Proficio-MKII-PTT`, not committed yet)

## Why

The firmware was written for the Potentia amps, which are QSK: they can switch between
receive and transmit instantly. A normal amp (like the one you are sending) uses a relay to
switch, and the relay needs a few milliseconds to close. With the old firmware, RF reached the
amp at the same moment as the "AMP on" signal. That means the relay could close with RF
already on it ("hot switching"). That is hard on the relay contacts, and the start of each
transmission can be lost.

The keyer can't raise PTT early, so the PSoC now does the timing itself.

## What the firmware does now

The rule is the same in every mode: **turn the amp on first, wait, then send RF. At the end,
stop RF first, then turn the amp off.**

### CW

1. On the first key-down, the AMP line turns on straight away.
2. The paddle/key is recorded every millisecond and played to the PA a short time later
   (the "key delay"). Every dot, dash and space is delayed by the same amount, so the
   element lengths and the speed are unchanged. You just hear and send everything a few ms
   late.
3. The first dot is sent in full; it is not cut short.
4. The TX hold (hang) time starts when the last delayed element has finished. When it runs
   out, the AMP line turns off and the radio goes back to receive, as before.

### SSB and the other modes (PTT)

1. PTT on: AMP line on → wait the key delay → PA on.
2. PTT off: PA off → wait 10 ms → AMP line off.
3. If PTT is released during the wait, the AMP turns off and no RF is sent.

## How long the delay is

There is no new setting. The key delay comes from the existing **TX hold** time:
TX hold divided by 16, limited to between 20 ms and 40 ms.

| TX hold | Key delay (amp on → RF) |
|---|---|
| 320 ms or less | 20 ms |
| 500 ms | about 31 ms |
| 640 ms or more | 40 ms |

## Worth knowing

- **It applies to every amp, the Potentia too.** The firmware has a "QSK amp" flag, but
  nothing ever sets it, so every amp gets the delay. For a Potentia this only adds a 20–40 ms
  lag, which should be harmless. If you'd ever like the old instant behaviour back for the
  Potentia, the flag is already there to switch it; that would be your call.
- **The TX LED goes off between elements and words in CW.** This is not a change. The old
  firmware does the same.
- **USB audio is not affected.** The new timer interrupt has the lowest priority, below USB.

## What I tested (no amp connected)

- CW works and sounds normal.
- SSB transmits on PTT, and receive audio comes back normally afterwards.
- CW still works after using SSB.
- Builds clean (only the usual USB library warning that the old release has too).

## Could you check with your scope and the amp?

1. **CW:** the time from AMP on to RF out at the first key-down (should be about 31 ms with
   500 ms hold), and that the first dot is full length.
2. **SSB:** the same AMP-on-to-RF gap on PTT, and that RF has stopped before the AMP line
   drops on PTT release.
3. That the AMP line drops after the hold time in CW.

If anything doesn't look right, tell me what you see and I'll change it. I'll commit the
code once you're happy with it.

## Where the changes are (for reference)

- `cw.c` — new 1 ms key-delay interrupt and the CW key-delay states
- `tx.c` — amp-then-PA order for PTT modes
- `main.c` — starts the new timer; works out the key delay from TX hold
- `basic-plus.h` — the delay limits and shared names
- Creator schematic — new `Key_Delay_Timer` (1 ms, continuous) and `Key_Delay_isr`

Questions → Ron (W4MMP).
