# si5351-drive — plan: Si5351 drive level set from mscc.ini (new opcode)

Ron's idea, 2026-10-06. **Plan only: nothing is changed, nothing is built.**

## Why

The Si5351 output beats with its 25 MHz crystal and puts a birdie pair on the spectrum (dial
14.0745 and 7.04325 USB; see `rpi/CLAUDE.md` Open list item 18 and
`.mscc-coord/NOTE-FOR-STEW-SI5351-BIRDIE-2026-10-06.md`). A lower output drive level gives
weaker harmonics, so it may weaken the birdie. This plan makes the level settable from
`mscc.ini`, so all four levels can be tried with one firmware flash and no client change.

## What the firmware does today

The Si5351 has four drive levels: 2, 4, 6 and 8 mA. The output runs at **8 mA**, the maximum.

- `si5351.c:267` sets CLK0 to 2 mA at startup.
- That does not last: every frequency change ends in `si5351a.c:408`, which writes
  `SI5351_CLK0_ON` = `0x4F` (`si5351a.c:203`) to the CLK0 control register. The low two bits
  `11` mean 8 mA. `main.c:98` sets the first frequency right after init.
- Same lines in `Proficio-MKII-PTT` (Ron's rig) and `Proficio-MKII-ATU`. The ATU, Geminus and
  Ultimus firmware also have `si5351a-cw.c` with the same `0x4F`.

## What the schematic shows (read 2026-10-06)

Source: `Schematic_Proficio-Mark-II-Rev-7_2026-10-06.pdf` and its netlist (title blocks say
REV 6, 2024-02-10). The netlist was checked against the drawing for these parts and agrees.
Read from the drawing only; nothing measured on a board.

- X1 is a 25 MHz TCXO (ECS-TXO-3225 or I538), through C147 into XA; not a bare crystal.
- CLK0 (U13 pin 10) goes straight to the two clock pins of the 74ACT74 (U12 pins 3 and 11)
  and nowhere else. No series resistor. The load is about 10 pF of logic input plus trace, so
  the drive setting only changes the edge speed.
- 3.3 V rail (U3): Si5351 VDD and VDDO, the TCXO, the PCM3060 digital VDD and the PSoC
  connector. One 0.1 uF (C24) at the Si5351 / TCXO, no bead.
- 5 V rail (U2): the 74ACT74, the PCM3060 analog VCC, the receive op-amp U8, both mixers and
  U16. A 0.1 uF each, no bead.

Two leak paths exist on paper (the 3.3 V rail and the 5 V rail); the drawing cannot say which
is real. A lower drive level also cuts the current pulses the Si5351 draws from the 3.3 V
rail, so this test can help on that path too.

**How much drive is needed (estimate, not measured):** the output is about 50 ohms at 8 mA
and about 200 ohms at 2 mA. Into about 10 pF, 2 mA gives edges of about 4 ns. On 20 m the
clock is 56 MHz (18 ns period): no problem. On 10 m it is about 115 MHz (9 ns period): 2 mA
is doubtful, 4 mA should be fine. If 2 mA works on 20 m but not on 10 m, the opcode could be
sent per band.

## Opcode

`CMD_SET_SI5351_DRIVE 0x9D`, host to rig, one byte: 0 = 2 mA, 1 = 4 mA, 2 = 6 mA, 3 = 8 mA.

0x9D is not used in the firmware's USB handler and is commented out as retired in ms-sdr's
`usbavrcmd.h` (old `GET_CW_DEFAULTS`; sdrcore-trans `commands.h` still carries that old name,
on a different link). **The number is not approved yet (Ron / Stew).**

## Firmware changes (`radio-psoc-firmware/Proficio-MKII-PTT/Proficio-MKII-PTT.cydsn`)

1. `usbvend.h`: add `#define CMD_SET_SI5351_DRIVE 0x9D`.
2. `main.c` (next to `E_Amplifier`, line 60): `uint8 E_si5351_drive = 3;` so the default stays
   8 mA. Add the `extern` in `basic-plus.h`.
3. `usbvend01.c`, host-to-device switch, after `CMD_SET_PA_BYPASS` (line 255): a new case that
   receives one byte into `E_si5351_drive`, same pattern as `CMD_SET_PA_BYPASS`.
4. `si5351a.c:203`: change the define to
   `#define SI5351_CLK0_ON (0x4Cu | SI_CLK_SRC_PLL_A | (E_si5351_drive & 0x03u))`.
5. `si5351a.c`, state 0 of `si5351aSetFrequency`: if the level differs from the one last
   written, write the CLK0 control register once. Needed because the level is otherwise only
   written at a frequency change, and ms-sdr restores the same frequency at startup.
6. Bump the firmware version (3.232 -> 3.233).

New firmware with an old ms-sdr behaves as today (8 mA).

## ms-sdr changes (`rpi/ms-sdr-linux/source`)

1. `usbavrcmd.h`: the same define.
2. `main.c` (line 80) and `extern.h`: `int G_si5351_drive = -1;` (-1 = key absent, send nothing).
3. `main-controller.c`, `Parse_mscc_record`: read `SI5351_DRIVE=2|4|6|8;`, convert to 0-3, log
   it. Any other value is logged and ignored.
4. `main-controller.c`: a small new function `Radio_send_si5351_drive()` that calls
   `usbControlMsgOUT` directly and only logs a failure.
5. `main.c`, right after `initialize_mscc()` (line 1370): call it if `G_si5351_drive >= 0`.

No storage in the rig: ms-sdr sends the level on every start.

## Trap

Step 4 must NOT use `Radio_send_parameters()`. Old firmware has no case for the opcode and
rejects it, and `Radio_send_parameters()` answers any failure with `Stop_all()`, which would
shut ms-sdr down. With the direct call, new ms-sdr on old firmware logs "not supported" and
carries on.

## Not covered by this plan

- `Proficio-MKII-ATU`, Geminus and Ultimus firmware (same change, plus `si5351a-cw.c`).
- The pill firmware (blackpill): needs the same opcode to keep the baseline comparable.
- Windows and Ubuntu ms-sdr (Stew).

## Test

Flash 3.233, build ms-sdr on the Pi, set `SI5351_DRIVE=2;` in `mscc.ini`, restart. Look at the
birdie pair at dial 14.074 USB (generator off, dummy load). Also check receive sensitivity and
transmit power: a lower drive level may be too weak for the mixer's clock input. Repeat for 4
and 6.
