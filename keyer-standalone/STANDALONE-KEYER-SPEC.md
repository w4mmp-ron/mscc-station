# Stand-alone keyer — spec (draft)

From Ron, 2026-11-05. Draft for Ron and Stew to mark up before anything is drawn.
Based on the Rev 2 board (`Schematic_STM32F411CEU6-PCIe-CW-Rev-2_2026-11-05.pdf`, N8VET) and
the keyer-buffer firmware (`main.c`, in Ron's rig since 2026-11-05).

**Goal:** the PIC keyer from the Rev 2 board as a box of its own: runs on a standard 9 V
battery (Ron 2026-11-05: available everywhere), keys any
transmitter, set up from its own controls (no radio, no client). Keeps everything the keyer
already does: iambic A / B, straight key, buffered Farnsworth (STP / RES), 4 CQ memories,
sidetone with rig audio pass-through.

Items marked **(check)** are my reading of the schematic or datasheet figures, not measured.

---

## 1. Block diagram

```
 9 V battery (PP3), 9.5 V fresh .. ~6 V spent
   |
 [P-FET reverse protection] -> [power switch] -> [LDO 5 V, low Iq] -> 5 V rail
   |                                                     |
   +-> [divider] -> PIC analog in (battery check)        +-> PIC, LM386 (switched), photoMOS LEDs

 Paddle jack --[ESD + RC]--> PIC RA5 / RA4
 Speed pot ---------------> PIC RA2 (analog)
 FN button ---------------> PIC RA3 (input only)

 PIC RC5 --> photoMOS --> KEY OUT jack (tip = key, sleeve = common, isolated)
 PIC RC4 --> photoMOS --> KEY OUT ring = PTT / amp (second output, fitted)
 PIC RC2 (NCO tone) --> RC filter + L1 (as Rev 2) --> volume pot --> LM386 on 5 V --> \
 RIG AUDIO IN jack -------------------------------------------------------------------> audio
                               switch U6 / U7 (TX_CW / RX_CW, as Rev 2) --> PHONES jack
 PIC RC1 --> P-FET --> LM386 supply (off when idle)
 PIC RA1 --> status LED            PIC RA0 = RX_CW (as Rev 2)     ICSP header as J3
```

## 2. What changes from Rev 2

| Rev 2 | Stand-alone |
|---|---|
| STM32 pill (M1), PCIe edge connector (U2), thermistor, ATU lines, BOOT | **Removed** |
| 12 V from the radio, R1 22 R, MC7805 (U4, about 5 mA quiescent **(check)**) | 9 V battery (snap clip + holder), P-FET reverse protection, power switch, low-Iq 5 V LDO (e.g. MCP1703A-5002: 16 V max in, 250 mA, about 2 uA quiescent, dropout about 0.3–0.6 V at these currents **(check)**), so it runs down to about 5.6–6 V |
| LM386M-1 (U5) on 12 V (4–12 V operating range; 13.8 V station supply is already over it) | LM386 on the **5 V rail**, switched by a P-FET from the PIC (off when idle) |
| Paddles from the radio through U2 | 3.5 mm stereo paddle jack: tip = dit, ring = dah, sleeve = ground; ESD (PRTR5V0U2X like U9), about 1 k series + 1 nF to ground per line against RF |
| KEY_0A to the pill (logic) | **photoMOS** to a KEY OUT jack: isolated, either polarity. Rating to suit the rigs it keys **(decide: modern rigs only, or vintage tube rigs with high / negative key-line voltages)** |
| KEY_1A unused | Optional second photoMOS on the KEY OUT ring: PTT / amp keying |
| Settings over I2C from the client (SCL / SDA) | Speed pot + FN button + paddle menu (section 5); RC0 / RC1 reused |
| — | Battery divider into an analog pin; "LB" on the sidetone when low |
| — | Status LED |
| Sidetone filter R3–R5, C14–C16, L1, C18, C17, RP1; audio switch U6 / U7, R2 / R8, D1 / D2; jacks J5 / J6 | **Unchanged** |
| ICSP header J3 | Unchanged |

## 3. PIC16F18326 pin map

| Pin | Port | Rev 2 | Stand-alone |
|---|---|---|---|
| 1 | VDD | 5 V | 5 V |
| 2 | RA5 | KEY_0 paddle | Paddle (dit / dah by the reverse setting) |
| 3 | RA4 | KEY_1 paddle | Paddle |
| 4 | RA3 / MCLR | MCLR / VPP | **FN button** (MCLR off in config; still VPP for ICSP) **(check that the button cannot upset programming)** |
| 5 | RC5 | KEY_0A out | KEY OUT photoMOS |
| 6 | RC4 | KEY_1A out (unused) | PTT / amp photoMOS (fitted) |
| 7 | RC3 | TX_CW | TX_CW (sidetone to phones) |
| 8 | RC2 | SIDE_TONE (NCO1) | SIDE_TONE |
| 9 | RC1 | SDA | **LM386 power enable** |
| 10 | RC0 | SCL | **Battery sense** (analog) |
| 11 | RA2 | free **(check)** | **Speed pot** (analog) |
| 12 | RA1 / ICSPCLK | ICSPCLK | **Status LED** (through a resistor; shares ICSPCLK) |
| 13 | RA0 / ICSPDAT | RX_CW **(check)** | RX_CW (rig audio to phones) |
| 14 | VSS | GND | GND |

All pins used. If memory buttons are wanted later, the LED and battery sense could share,
or a bigger PIC (PIC16F18346, 20 pins) gives room.

## 4. Power budget (datasheet estimates, not measured)

| | Idle, awake | Keying | Sleeping |
|---|---|---|---|
| LDO quiescent | ~2 uA | ~2 uA | ~2 uA |
| PIC | 1–2 mA | 1–2 mA | a few uA |
| LM386 | 0 (switched off) | 4 mA + 20–50 mA into headphones | 0 |
| photoMOS LED | 0 | 5–10 mA | 0 |
| Battery divider (e.g. 1 M / 330 k) | ~10 uA | ~10 uA | ~10 uA (or switch it) |
| **Total** | **~2 mA** | **~30–65 mA** | **~20 uA** |

Alkaline 9 V (about 500 mAh; lithium 9 V about 1200 mAh):

| Use | Life (alkaline) |
|---|---|
| Asleep (switched on, not keying) | years: shelf life limits it |
| Awake, not keying | ~250 hours |
| Keying continuously | ~8–15 hours |
| Normal operating (sending part of the time) | roughly 20–40 hours |

Most of the keying current is the LM386 into the headphones and the photoMOS LED: lower
volume and a low-current photoMOS (or a higher LED resistor, within its spec) stretch it.
Low battery: warn ("LB" on the sidetone) at about 6.5 V, stop keying at about 5.8 V.
No charger: replace the battery.

## 5. Controls and menu

**Front:** power switch, SPEED knob, VOLUME knob (RP1), FN button, status LED.
**Back:** PADDLE, KEY OUT, PHONES, RIG AUDIO IN. **Inside:** 9 V battery holder (easy to reach).

- **SPEED knob:** letter speed, 5–60 wpm (range settable with S). Read every 50 ms with hysteresis.
- **FN short press:** play CQ memory 1. A paddle press stops it (as now).
- **FN long press (about 1 s):** menu. The keyer answers "?" on the sidetone; send a
  command letter with the paddles; the keyer answers in Morse; FN or 10 s idle leaves.

| Command | Does | Answer |
|---|---|---|
| F | Buffered Farnsworth: then send the text speed in digits (e.g. 15), or 0 = off | "F 15" / "F OFF" |
| A / B | Iambic mode A / B | "A" / "B" |
| K | Straight key | "K" |
| R | Reverse paddles | "R" |
| T | Sidetone 400 / 600 / 800 / 1000 Hz, steps each time | "4" / "6" / "8" / "1" |
| W | Weight: then digits (e.g. 50) | "W 50" |
| M | Record a memory: send the slot 1–4, then key the message, FN to end | "M 1" ... "R" (recorded) |
| 1–4 | Play memory 1–4 | — |
| V | Battery voltage | e.g. "11.8" |
| S | Speed knob range: then two numbers (e.g. 10 30); default 5 60 | "S 10 30" |

All settings saved in EEPROM. The menu uses the existing announcer (sidetone only, never on
the air) and the letter decoding from the buffer.

## 6. Firmware plan (from keyer-buffer)

1. **Build switch** `STANDALONE`: no I2C; pins as section 3. The radio build stays as it is.
2. **New EEPROM layout** with a version byte and defaults; settings including Farnsworth saved
   (in the radio build 0x9D stays RAM only). A new unit starts with empty CQ memories.
3. **Speed pot** (ADC, 50 ms, hysteresis), **battery check** (ADC, every few seconds, LB / cut-off).
4. **FN button:** short / long press, debounced, in the 1 ms tick.
5. **Menu** state machine: commands above; answers through the announcer.
6. **Memory record from the paddles:** decode each buffered letter (element bits -> character
   with the Morse table already in main.c) into the slot; word gaps -> spaces; FN ends.
7. **Sleep:** after about 30 s idle, LM386 off, tick off, PIC sleeps; paddle or FN wakes it
   (interrupt on change).
8. **PC test bench** (as for keyer-buffer): menu, memory record, speed pot, sleep / wake, before
   any hardware.

## 7. How to get there

1. Firmware on the PC bench.
2. Try it on Ron's Rev 2 board where it fits: paddles, sidetone, key out and the buffer are the
   same pins; pot and FN would need temporary wires **(check)**.
3. Prototype on perfboard (PIC, sidetone, audio switch, power).
4. PCB (EasyEDA), then an enclosure.

## 8. Decisions (Ron, 2026-11-05)

| Question | Decision |
|---|---|
| Battery | Standard 9 V (PP3) only; no charger |
| Key output | Modern rigs only: VO1400 photoMOS, 60 V / 100 mA, isolated |
| Second output | **Fitted:** PTT / amp on the KEY OUT ring (second VO1400) |
| Rig audio pass-through | Kept; RX path normally closed (CPC1117) so it costs no current |
| LM386 | On the 5 V rail, switched off when idle |
| Speed knob | **5–60 wpm** (the keyer's full range) |
| Enclosure | **Small aluminium box**, Hammond 1590B size (about 112 x 60 x 31 mm) or similar |
| CQ memories | **FN + menu:** FN short press = memory 1; menu then 1–4 = any memory. 14-pin PIC kept |
| Schematic | Route 1: netlist + BOM + reference drawing (`hardware/`), drawn in EasyEDA by Ron / Stew |
