# Pinout — Proficio PSoC 3 → STM32F411 Black Pill

**For Stew:** use **`docs/STEW-DAUGHTER-BOARD-PINOUT.md`** + **`STEW-BlackPill-pinout-diagram.jpg`**  
(Those are the daughter-board deliverable. This file is a short engineer map.)

**Version:** 2026-08-17  

**Mother-board nets** = MKII radio/codec/control signals that leave the MCU daughter for the rest of the radio (same list the old PSoC used).

**PSoC:** CY8C3246PVI-147 48-SSOP  
**STM32:** WeAct STM32F411CEU6 Black Pill  
**Logic:** 3.3 V only on STM32 I/O (+3.3V and common GND required)

---

## Mother-board nets → STM32

| MKII mother-board net | Dir | STM32 |
|-----------------------|-----|-------|
| DOUT (from PCM3060) | in | **PB14** |
| DIN (to PCM3060) | out | **PB15** |
| BCK1 / BCK2 | out | **PB13** (tie both nets) |
| LRCK1 / LRCK2 | out | **PB12** (tie both nets; **not** GND) |
| SCK1 / SCK2 | out | **PC6** (tie both nets) |
| SDA / SCL | OD | **PB9 / PB8** |
| BS0 / BS1 / BS2 | out | **PA7 / PB5 / PB3** |
| LED1 | out | **PC13** (module) |
| RX / AMP | out | **PA1 / PB4** (active low) |
| **BOOT** | in | **PA8** |
| KEY_0 / KEY_1 | in | **PB0 / PB1** (no KEY_7) |
| **PTT** | **in** | **PA6** (sense) |

USB host: Black Pill PA11/PA12 (USB-C).

**Power:** +3.3V and GND to daughter; common GND with mother board.

---

## Reference (WeAct module)

| File | Role |
|------|------|
| `WeAct-BlackPill-F411-Pinout.png` | Vendor-style Black Pill pinout |
| `WeAct-STM32F4x1-Pin-Layout.pdf` | Official WeAct pin layout PDF |
| `WeAct-BlackPill-F411-board.jpg` | Board photo |
| `stm32-base-top.jpg` | Top-view photo |

---

## Firmware

`firmware/pio/include/board_pins.h`
