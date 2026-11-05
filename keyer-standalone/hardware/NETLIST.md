# Stand-alone keyer — netlist (draft 1, 2026-11-05)

For drawing the schematic in EasyEDA. Parts list with LCSC numbers: `BOM.csv`.
Spec: `../STANDALONE-KEYER-SPEC.md`. Based on the Rev 2 board (N8VET) sheet 1 (PIC) and sheet 2
(sidetone, audio switch); changes from Rev 2 are marked **NEW** or **CHANGED**.

Ron's choices (2026-11-05): standard 9 V battery, modern rigs only, keep rig audio pass-through,
LM386 on 5 V, PTT output fitted, speed knob 5–60 wpm, small aluminium box (1590B size),
CQ memories through FN + menu.

Items marked **(check)** need the datasheet / EasyEDA symbol confirmed when drawing:
pin numbers of the 3.5 mm jacks are given as Tip / Ring / Sleeve, map them to the SJ1 symbol pins.

---

## Sheet 1 — power, PIC, paddle, key out

### Power (NEW)

| Ref | Part | Pin | Net |
|---|---|---|---|
| J1 | 2-pin header to the 9 V snap clip | 1 | BATT+ |
| | | 2 | GND |
| J2 | 2-pin header to the panel power switch | 1 | BATT+ |
| | | 2 | SW_OUT |
| Q1 | AO3401A P-FET (reverse-battery protection) | D (3) | SW_OUT |
| | | S (2) | VBAT |
| | | G (1) | Q1_G |
| R1 | 100k | | Q1_G – GND |
| U1 | MCP1703T-5002E/CB, 5 V LDO, SOT-23A | 3 VIN | VBAT |
| | | 2 VOUT | +5V |
| | | 1 GND | GND |
| C1 | 1 uF | | VBAT – GND (at U1 input) |
| C2 | 1 uF | | +5V – GND (at U1 output, datasheet minimum) |
| C3 | 10 uF | | +5V – GND (bulk) |

How Q1 works: battery the right way round, the body diode conducts, gate is at 0 V, the FET
turns fully on (no diode drop). Battery reversed: blocked. Vgs at 9.5 V is inside the AO3401A's
±12 V gate rating. AO3401A pin numbers (SOT-23: 1 G, 2 S, 3 D) **(check)**.

### Battery check (NEW)

| Ref | Part | Net |
|---|---|---|
| R2 | 1M | VBAT – VSENSE |
| R3 | 470k | VSENSE – GND |
| C4 | 100 nF | VSENSE – GND |

VSENSE = VBAT × 0.32: 9.5 V -> 3.04 V, 6.5 V (LB) -> 2.08 V, 5.8 V (stop) -> 1.86 V.
Divider draws about 6 uA.

### PIC (CHANGED pin use)

| U2 pin | Port | Net | Notes |
|---|---|---|---|
| 1 | VDD | +5V | C5 100 nF to GND, close to the pin |
| 2 | RA5 | PAD_A | paddle (dit/dah set by reverse) |
| 3 | RA4 | PAD_B | paddle |
| 4 | RA3 / MCLR | FN | FN button; also VPP on J8 |
| 5 | RC5 | KEY_K | key-out photoMOS LED cathode |
| 6 | RC4 | PTT_K | PTT photoMOS LED cathode |
| 7 | RC3 | TX_CW | sidetone switch (sheet 2), as Rev 2 |
| 8 | RC2 | SIDE_TONE | NCO tone to sheet 2, as Rev 2 |
| 9 | RC1 | AMP_EN | LM386 power switch (low = on) **NEW** |
| 10 | RC0 | VSENSE | battery check (analog) **NEW** |
| 11 | RA2 | SPEED | speed pot wiper (analog) **NEW** |
| 12 | RA1 / ICSPCLK | LED / PGC | status LED; ICSP clock |
| 13 | RA0 / ICSPDAT | RX_CW / PGD | RX audio switch (sheet 2); ICSP data |
| 14 | VSS | GND | |

U2 = PIC16F18326-I/P (DIP-14, in a socket, as Rev 2).

### FN button (NEW)

| Ref | Part | Net |
|---|---|---|
| R4 | 10k | +5V – FN (pull-up) |
| R5 | 1k | FN – FN_SW |
| J5 | 2-pin header to the panel push button | 1 FN_SW, 2 GND |

R5 keeps a pressed button from shorting VPP while programming (about 9 mA from the PICkit
through R5). No capacitor on FN: Microchip advises none on MCLR/VPP; debounce in firmware.

### Speed pot (NEW)

| Ref | Part | Net |
|---|---|---|
| J6 | 3-pin header to the panel SPEED pot (100k linear) | 1 +5V, 2 SPEED, 3 GND |
| C6 | 100 nF | SPEED – GND (at U2 pin 11) |

100k so the pot draws only 50 uA; C6 gives the ADC a low source impedance.

### Status LED (NEW)

| Ref | Part | Net |
|---|---|---|
| R6 | 1k | LED_PGC – LED_A |
| J7 | 2-pin header to the panel LED | 1 LED_A (anode), 2 GND (cathode) |

### LM386 power switch (NEW)

| Ref | Part | Pin | Net |
|---|---|---|---|
| Q2 | AO3401A P-FET | S (2) | +5V |
| | | D (3) | AMP_VCC |
| | | G (1) | Q2_G |
| R7 | 100 R | | AMP_EN – Q2_G |
| R8 | 100k | | Q2_G – +5V (amp off while the PIC starts) |

### Paddle input (NEW)

| Ref | Part | Pin | Net |
|---|---|---|---|
| J3 | SJ1-3535NG stereo jack, PADDLE | Tip | PAD_A_RAW |
| | | Ring | PAD_B_RAW |
| | | Sleeve | GND |
| U3 | PRTR5V0U2X ESD | 1 GND | GND |
| | | 2 I/O1 | PAD_A_RAW |
| | | 3 I/O2 | PAD_B_RAW |
| | | 4 VCC | +5V |
| R9 | 1k | | PAD_A_RAW – PAD_A |
| R10 | 1k | | PAD_B_RAW – PAD_B |
| R11 | 10k | | +5V – PAD_A (pull-up) |
| R12 | 10k | | +5V – PAD_B (pull-up) |
| C7 | 1 nF | | PAD_A – GND |
| C8 | 1 nF | | PAD_B – GND |

U3 sits at the jack. 1k + 1 nF filter RF on the paddle cable; 10k pull-ups (0.5 mA only while
a paddle is closed). Tip = dit, ring = dah by default; the reverse setting swaps them.

### Key output (NEW, isolated)

| Ref | Part | Pin | Net |
|---|---|---|---|
| U4 | VO1400AEFTR photoMOS (Form A), KEY | 1 A | U4_A |
| | | 2 C | KEY_K |
| | | 3 S | KEY_COM |
| | | 4 S | KEY_TIP |
| R13 | 560 R | | +5V – U4_A |
| U5 | VO1400AEFTR photoMOS (Form A), PTT | 1 A | U5_A |
| | | 2 C | PTT_K |
| | | 3 S | KEY_COM |
| | | 4 S | PTT_RING |
| R14 | 560 R | | +5V – U5_A |
| J4 | SJ1-3535NG stereo jack, KEY OUT | Tip | KEY_TIP |
| | | Ring | PTT_RING |
| | | Sleeve | KEY_COM |

KEY_COM is **not** connected to the keyer's GND (isolated output, either polarity).
LED current (5 V − 1.2 V) / 560 R ≈ 6.8 mA; the VO1400 needs 5 mA max to turn on, 50 mA abs max.
Output: 60 V, 100 mA, about 2.5 R max on (datasheet), so fine for modern rigs. Turn-on about
0.65 ms typ, turn-off 0.3 ms typ: a dit is about 0.35 ms shorter, nothing at CW speeds.
U4 / U5 pins 1–4 as on the Rev 2 VO1400 symbol **(check)**.

### ICSP (as Rev 2 J3)

| J8 pin | Net |
|---|---|
| 1 | FN (MCLR / VPP) |
| 2 | +5V |
| 3 | GND |
| 4 | RX_CW / PGD (RA0) |
| 5 | LED / PGC (RA1) |
| 6 | not connected |

---

## Sheet 2 — sidetone and audio switch (from Rev 2 sheet 2)

### Sidetone filter (as Rev 2)

| Ref (Rev 2) | Part | Net |
|---|---|---|
| R15 (R3) | 1k5 | SIDE_TONE – ST1 |
| C9 (C14) | 100 nF | ST1 – GND |
| R16 (R4) | 1k5 | ST1 – ST2 |
| C10 (C15) | 100 nF | ST2 – GND |
| R17 (R5) | 1k5 | ST2 – ST3 |
| C11 (C16) | 100 nF | ST3 – GND |
| L1 (L1) | 1 mH | ST3 – ST4 |
| C12 (C18) | 470 nF | ST4 – GND |
| C13 (C17) | 10 nF | ST3 – VOL_TOP (coupling) |

**(check)** with the Rev 2 drawing: as I read sheet 2, C17 couples from the L1 top (ST3) to the
volume pot, L1 + C18 go from ST3 to ground. Copy Rev 2 exactly here.

### Volume pot (CHANGED: panel pot on a header; Rev 2 RP1 10k)

| Ref | Part | Net |
|---|---|---|
| J9 | 3-pin header to the panel VOLUME pot (10k audio) | 1 VOL_TOP, 2 VOL_W, 3 GND |
| C14 (C20) | 10 nF | VOL_W – GND |

### LM386 (CHANGED: on AMP_VCC = switched 5 V)

| U6 pin | Net | Rev 2 |
|---|---|---|
| 1 GAIN | GAIN1 | |
| 2 IN− | GND | |
| 3 IN+ | VOL_W | |
| 4 GND | GND | |
| 5 VOUT | AMP_OUT | |
| 6 VS | AMP_VCC | was 12 V |
| 7 BYPASS | BYP | |
| 8 GAIN | GAIN8 | |

| Ref (Rev 2) | Part | Net |
|---|---|---|
| R18 (R6) | 680 R | GAIN1 – G_MID |
| C15 (C12) | 10 uF | G_MID – GAIN8 |
| C16 (C21) | 100 nF | BYP – GND |
| C17 (C13) | 100 nF | AMP_VCC – GND |
| C18 **NEW** | 47 uF | AMP_VCC – GND (switched rail bulk) |
| C19 (C22) | 100 nF | AMP_OUT – ZOB |
| R19 (R7) | 10 R | ZOB – GND |
| C20 (C19) | 220 uF | AMP_OUT (+) – SIDE_TONE_OUT (−) |
| C21 (C23) | 10 nF | SIDE_TONE_OUT – GND |

U6 = LM386MX-1/NOPB (SOIC-8, LCSC C23573; the M-1 in reel packaging).

### Audio switch (CHANGED: RX path now normally closed)

| Ref | Part | Pin | Net |
|---|---|---|---|
| U7 (U6) | VO1400AEFTR (Form A), sidetone path | 1 A | U7_A |
| | | 2 C | TX_CW |
| | | 4 S | SIDE_TONE_OUT |
| | | 3 S | AUDIO_OUT |
| R20 (R2) | 1k2 | | +5V – U7_A |
| D1 (D1) | DDZ9678-7, 1.8 V zener | | U7_A – GND (as Rev 2) |
| U8 **CHANGED** | CPC1117NTR (Form B, normally closed), RX path | 1 A | U8_A |
| | | 2 C | RX_CW |
| | | 4 S | RX_AUDIO |
| | | 3 S | AUDIO_OUT |
| R21 (R8) | 1k2 | | +5V – U8_A |
| D2 (D2) | DDZ9678-7, 1.8 V zener | | U8_A – GND (as Rev 2) |

**Why U8 changed:** with Rev 2's Form A part the RX path needs its LED lit the whole time you are
not keying (about 3 mA, the biggest drain on a 9 V battery, and it would stop the PIC sleeping
with audio passing). The CPC1117 is closed with the LED off (60 V, 150 mA, 16 R max on; opens at
1 mA LED current), so rig audio passes with no current. **Firmware:** in the stand-alone build
RX_CW is inverted: high = rig audio on (LED off), low = rig audio muted (while sending).
CPC1117 pin numbering **(check)** against its symbol.

### Jacks (as Rev 2)

| Ref (Rev 2) | Part | Pin | Net |
|---|---|---|---|
| J10 (J5) | SJ1-3533NG, PHONES | Tip | AUDIO_OUT |
| | | Sleeve | GND |
| J11 (J6) | SJ1-3533NG, RIG AUDIO IN | Tip | RX_AUDIO |
| | | Sleeve | GND |
| J12 (AUDIO_OUT) | 2-pin header (optional, as Rev 2) | 1 AUDIO_OUT, 2 GND | |

Rig audio ground is the keyer's GND (the key output stays isolated).

---

## Nets summary

+5V, GND, BATT+, SW_OUT, VBAT, Q1_G, VSENSE, FN, FN_SW, SPEED, LED_A, LED_PGC, RX_CW_PGD,
AMP_EN, Q2_G, AMP_VCC, PAD_A_RAW, PAD_B_RAW, PAD_A, PAD_B, KEY_K, PTT_K, U4_A, U5_A,
KEY_TIP, PTT_RING, KEY_COM, TX_CW, SIDE_TONE, ST1–ST4, VOL_TOP, VOL_W, GAIN1, G_MID, GAIN8,
BYP, AMP_OUT, ZOB, SIDE_TONE_OUT, U7_A, U8_A, RX_AUDIO, AUDIO_OUT.

## Firmware impact (stand-alone build)

- RX_CW inverted (above). Asleep: RX_CW high (rig audio passes, no current), TX_CW high,
  AMP_EN high (amp off), KEY_K / PTT_K high (outputs open).
- AMP_EN low at the first paddle press, high again when the keyer goes idle.
- ADC: SPEED (RA2), VSENSE (RC0).
- FN on RA3: MCLR disabled in the config bits (MCLRE = 0); VPP still works for programming.
