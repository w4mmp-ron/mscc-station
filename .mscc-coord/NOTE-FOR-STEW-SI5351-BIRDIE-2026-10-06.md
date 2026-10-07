# For Stew — birdie pair on the spectrum (Si5351 against its 25 MHz crystal)

From Ron, 2026-10-06. Seen on Ron's Proficio MKII PTT, PSoC board, FW 3.232, Pi host,
WPF 10.4.0. It is made in the rig: it is there with the signal generator off and a dummy
load on the antenna. It is not a display or server fault. Ron would like your view on what
to do about it; three options are at the end.

## What Ron sees

On 20 m, with the dial within about 1.5 kHz of 14.0745 USB, two steady lines appear on the
spectrum, the same distance either side of the LO point (dial - 12 kHz), about 10 dB above
the noise floor (around -110 on the client scale). Nowhere else on 20 m.

Measured from Ron's screen recordings (1 kHz and 100 Hz dial steps):

| Dial (USB) | Lines, from the LO point |
|---|---|
| 14.073 | one, +47.25 kHz |
| 14.074 | pair, -15.3 / +15.3 kHz |
| 14.075 | pair, -16.8 / +16.6 kHz |
| 14.076 | one, +47.25 kHz |

All of them fit one line: **offset = 32 x (dial - 14.074478 MHz)**, shown only while it is
under 48 kHz. So the lines move 32 kHz for every 1 kHz of dial. In CW the zero beat is at
dial 14.0751 (the LO point is 12.6 kHz below the dial there).

At dial 14.074 (20 m FT8) the upper line is at 14.0773. With Hi at 4.0 kHz that is inside
the receive passband: a steady tone at about 3.3 kHz audio.

## What it looks like (by fit, not measured on the board)

- Zero beat is at LO = 14.0625 MHz = 25 MHz x 9/16. The Si5351 output there (4 x LO) is
  56.25 MHz, and 8 x 56.25 = 18 x 25 = 450 MHz. So a harmonic of the Si5351 output meets a
  harmonic of its 25 MHz crystal. The slope of 32 (8 x 4) agrees with that.
- The pair is symmetric about the LO point. That means a real signal, the same in I and Q,
  not a quadrature one. So it does not come in as RF through the mixer; it gets into the
  audio / codec side after it.
- The general rule would be 32 x LO = 25 MHz x k, LO = 0.78125 MHz x k. USB dial settings
  inside ham bands (LO + 12 kHz):

| Band | Dial (USB) | k |
|---|---|---|
| 40 m | 7.04325 | 9 |
| 20 m | 14.0745 | 18 |
| 15 m | 21.10575 | 27 |
| 10 m | 28.137, 28.918, 29.6995 | 36, 37, 38 |

- **Tested by Ron 2026-10-06: 40 m USB, dial 7.043, the pair is there too.** Position not
  measured. The 15 m and 10 m settings are not tested.

Not known: how it gets into the audio, and whether your boards and the other radios show it.

## Options

1. **Work around it, no changes.** On 20 m FT8 set Hi to 3.0 kHz; the 3.3 kHz tone is then
   outside the filter. The line stays on the spectrum.
2. **Software.** The lines only show while the LO is within about 1.5 kHz of a birdie
   point. Today the LO is always dial - 12 kHz. If that offset were moved by about 3 kHz
   whenever the LO would land in such a window, the beat would be above 48 kHz and the codec
   would filter it out. It touches ms-sdr, sdrcore-recv, sdrcore-trans and the client's
   spectrum scale, on the Pi, Windows and Ubuntu. Not worked out in detail, nothing changed.
3. **Hardware.** Decoupling, layout or shielding between the Si5351 / crystal and the audio
   / codec side, or a crystal frequency that puts the birdie points outside the ham bands.
   New or reworked boards only.

## Option 3 in numbers — a different reference for new rigs (Ron's suggestion)

Ron: a different TCXO is only a part swap plus the crystal value in the firmware, so it
could go into new rigs. Where the same birdie (32 x LO = reference x k) would land, USB
dial, inside the ham bands:

| Reference | In-band birdie points (dial, MHz) |
|---|---|
| 25 MHz (now) | 3.918, 7.043, 14.0745, 21.106, 28.137, 28.918, 29.700 |
| 26 MHz | 21.137, 28.450, 29.262 |
| 27 MHz | 10.137, 21.106, 28.700, 29.543 |
| 27.12 MHz | 21.200, 28.827, 29.675 |
| 24 MHz | 3.762, 14.262, 21.012, 28.512, 29.262 |

27 MHz puts one on 10.137, next to 30 m FT8 (10.136). 26 MHz looks best: nothing on 160 m
through 17 m, and its 15 m and 10 m points are away from the FT8 frequencies.

Limits: calculated, not measured. It covers only the one birdie family seen so far; other
harmonic combinations may exist. It assumes the LO is 12 kHz below the dial (LSB and CW not
checked). Whether the Si5351 on this board is happy at 26 MHz is your call.

## Read from the schematic and firmware (2026-10-06)

Source: `Schematic_Proficio-Mark-II-Rev-7_2026-10-06.pdf` and its netlist (title blocks say
REV 6, 2024-02-10). Read from the drawing only; nothing measured on a board.

- **X1 is the 25 MHz TCXO** (ECS-TXO-3225 or I538), through C147 into XA. Where this note
  says "crystal" above, read TCXO.
- **CLK0 (U13 pin 10) goes straight to the two clock pins of the 74ACT74 (U12 pins 3 and 11)**
  and nowhere else. No series resistor. So the load is logic input plus trace, and the drive
  setting only changes the edge speed.
- **The firmware runs CLK0 at 8 mA, the maximum.** `si5351.c:267` sets 2 mA at startup, but
  every tune ends in `si5351a.c:408`, which writes `0x4F` to the CLK0 control register (low
  bits 11 = 8 mA). Same in all the radios' firmware.
- **3.3 V rail (U3):** feeds the Si5351 VDD and VDDO, the TCXO, the PCM3060 digital VDD and
  the PSoC connector. One 0.1 uF (C24) at the Si5351 / TCXO, no bead between them and the
  codec.
- **5 V rail (U2):** feeds the 74ACT74, the PCM3060 analog VCC, the receive op-amp U8, both
  mixers and U16. A 0.1 uF each, no bead.

So on paper there are two ways the beat could reach the audio, the 3.3 V rail and the 5 V
rail. Which one is real, the drawing cannot say.

**Cheap test, planned, not built:** a new opcode so the drive level (2 / 4 / 6 / 8 mA) can be
set from `mscc.ini` with no client change. One firmware flash, then try each level and watch
the pair at dial 14.074. Plan: `rpi/si5351-drive.md`. Rough estimate for about 10 pF of load:
4 mA should be enough on all bands; 2 mA is fine on the low bands and doubtful on 10 m (clock
about 115 MHz). Your view on the lowest safe level is welcome.

Candidates for new boards, for you to judge: a bead plus capacitor to isolate the Si5351 /
TCXO supply, a series resistor at CLK0, a bead on the 74ACT74 supply.

## Asked of you

- Do you see the pair on your radios at dial 14.074 USB (dummy load is fine)?
- Which of the three do you think is right, and do you know where it gets in?

Also seen, weaker and not looked into: fixed lines about 7.0, 4.0 and 1 kHz below the LO
point and one about 29.4 kHz above it (dial 14.074, same recording).
