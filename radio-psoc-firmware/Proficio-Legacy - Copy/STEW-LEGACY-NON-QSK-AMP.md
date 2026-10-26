# Legacy Proficio: non-QSK amp keying, key-click shaping, and an SSB-after-CW bug — note for Stew

**From:** Ron (W4MMP)  
**Date:** 2026-10-20  
**Firmware:** Proficio Legacy **1.235** (updated 2026-10-26, see "Update 1.235" below; 1.232 was the first), in the folder `radio-psoc-firmware\Proficio-Legacy - Copy`
(a copy of your Legacy tree; your `Proficio-Legacy` is untouched)  
**Status:** builds clean. **Not tested on a radio**; I don't have a Legacy rig. Over to you.

This is the same idea as the MKII PTT 3.234 change (`STEW-NON-QSK-AMP.md` in Proficio-MKII-PTT),
adapted to the way Legacy keys CW. Take it, change it or leave it; your call.

## Update 1.235 (2026-10-26)

Three changes bring Legacy in line with MKII-PTT 3.240:

1. **CW keying held steady (1.233).** During CW, the 1 ms key-delay interrupt now writes the
   I/Q gate (DIN) on every tick: open through each element and its 18 ms fade-out, closed
   after. Before, a routine main-loop control write could close DIN early, cutting an element
   short or causing a click.
2. **Switching to CW drops any transmit (1.234).** If PTT or TUNE from the host is still on
   when the mode changes to CW, the radio now turns PA, AMP and the TX LED off and goes back to
   receive. Before, it could stay stuck in transmit.
3. **Calibration stored in the radio (1.235).** Same as MKII-PTT: per-band receive IQ,
   transmit IQ and power values (CMD_CALIBRATION 0xC8), kept in EEPROM rows 8-19. Nothing else
   in Legacy uses those rows. New file `calibration.c`.

Flash file: `Proficio-Legacy - Copy\Debug\Proficio-Legacy.cyacd`. Built clean; not tested on a
Legacy rig yet. Test items 6 and 7 at the end cover these.

## How Legacy keys CW (unchanged idea)

Legacy does not key the PA line per element. At the first element it turns on the AMP line and
the PA line (which also switches the T/R relay and the PA bias) and keeps them on through the
hang time. Each dot and dash is made by opening and closing **DIN**, the gate on the I2S TX data
to the PCM3060 (control bit 2 ANDed with the I2S data line in the schematic). That keeps the
relays from chattering.

## Change 1: SSB had no RF after using CW (bug fix)

DIN is open at power-up, but nothing opened it again after CW. Every CW element leaves it
closed at the end, and neither the end of the hang time nor the change from CW to another mode
reopened it. So, as far as I can tell from the code, after any CW sending, SSB and digital modes
would send no TX I/Q (no RF) until the radio was power-cycled.

Now DIN is opened again when the hang time ends and when the mode changes away from CW.

## Change 2: CW key delay for non-QSK amps

As on the MKII: the relays need time to close, and before, RF started at the same moment.

1. First key-down: AMP line and PA line on straight away, DIN closed.
2. The key is recorded every millisecond and played to DIN a short time later (the key delay).
   Every element is delayed the same amount, so element lengths and speed are unchanged, and
   the first dot is not cut short.
3. The hang time starts when the last delayed element has finished, then back to receive as
   before.

The key delay comes from the existing TX hold setting: TX hold / 16, kept between 20 and 40 ms
(about 31 ms at 500 ms hold).

| TX hold | Key delay (relays on → RF) |
|---|---|
| 320 ms or less | 20 ms |
| 500 ms | about 31 ms |
| 640 ms or more | 40 ms |

As on the MKII, nothing sets the QSK flag, so every amp gets the delay. If the flag is ever set,
the original Legacy keying is still there and runs unchanged.

## Change 2b: key-click shaping with the PCM3060 soft mute

DIN is a hard on/off gate, so each element started and stopped instantly. Now each element
ramps up and down using the PCM3060's own soft mute (register 0x44, bits MUT21 + MUT22):

- Per the TI datasheet (SLAS533B), the DAC level moves 0.5 dB every 8 samples. At the
  Proficio's 96 kHz rate that is 6 dB per ms: 16.7 ms from full to mute, the top 40 dB in
  about 6.7 ms. The chip does the ramp itself; one I2C write per edge. (Every DAC level change
  slews at this fixed rate, so a hand-made cosine table is not possible; this is the chip's
  own exponential, dB-per-ms ramp.)
- In CW the DAC is kept muted. For each element DIN opens and the DAC unmutes 14 ms before
  the delayed key down, so the half-level (-6 dB) point lands on the key edge; at the delayed
  key up the DAC mutes, and DIN closes 18 ms later when the ramp is done. Element lengths
  stay right at the half-level points.
- The key-delay interrupt only asks for the mute or unmute; the main loop does the I2C write
  (I2C can't run in an interrupt).
- On leaving CW the DAC is unmuted again for SSB.
- Limits: spaces shorter than 14 ms merge, so above about 80 WPM elements run together.
  At the shortest key delay (20 ms) the RF starts rising 6 ms after key-down, still about
  70 dB down when the amp relay closes.
- Note: the PCM3060 is rated for 400 kHz I2C; the Proficio bus runs at 1000 kHz. That works
  today and is unchanged; it just adds one short write per element edge.

## Change 3: SSB / PTT amp sequencing

Same as MKII 3.234 (Legacy's `tx.c` was identical to the old MKII one, so it is now the same
file): PTT on → AMP on → wait the key delay → PA on. PTT off → PA off → wait 10 ms → AMP off.

## Schematic (TopDesign)

- New `Key_Delay_Timer`: Fixed Function, 16-bit, period 10 on CW_Clock (1 ms), Run Mode
  Continuous, reset tied to Logic Low.
- New `Key_Delay_isr` on its tc output, priority 7 (below USB).

## Code

- `cw.c`: the 1 ms key-delay interrupt (drives DIN and asks for the DAC soft mute), the new CW
  states, DIN reopened at the end of the hang.
- `main.c`: starts the timer; works out the key delay from TX hold; reopens DIN on leaving CW.
- `tx.c`: amp-then-PA order (same file as MKII 3.234).
- `basic-plus.h`: limits and shared names; `PCM3060_SetRegister` prototype; version 1.235.
- 1.233-1.235: `cw.c` (DIN every tick), `main.c` (entering CW drops TX; calibration load and
  save), `calibration.c` (new), `usbvend.h` / `usbvend01.c` (0xC8), `basic-plus.h`.

## Post-build step

The Creator post-build command in your Legacy project points to
`C:\Users\Ron\.grok\worktrees\Release-Proficio-Legacy\copy-release.bat`, a folder that is gone
since the repo was reorganised, so the build reports "failed" at the very end (the hex and
cyacd are made fine). In the copy it now points to the copy's own `copy-release.bat`. Your
`Proficio-Legacy` still has the old path.

## Could you test on a Legacy rig?

1. Radio reports 1.235.
2. CW: keys normally, first dot complete, sounds right. On a receiver nearby or a scope:
   softer edges and less click than before; at higher speeds (30+ WPM) elements still clean.
3. **SSB after CW**: send some CW, switch to SSB, PTT: you should get RF (the bug fix).
4. Receive audio returns normally after CW and after SSB.
5. With a non-QSK amp and a scope, if you have time: AMP on to RF out about 31 ms at 500 ms
   hold, in CW and on SSB PTT; RF stopped before the AMP line drops.
6. CW at a few speeds: no chopped elements, no new clicks (1.233).
7. In SSB with PTT or TUNE on, switch to CW: the radio drops back to receive cleanly (1.234).

If it looks good you can move it into your Legacy tree, or tell me and I'll help. Nothing has
been committed.

Questions → Ron (W4MMP).
