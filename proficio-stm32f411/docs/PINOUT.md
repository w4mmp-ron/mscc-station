# Pinout — Proficio PSoC 3 → STM32F411 Black Pill

**For Stew:** use **`docs/STEW-DAUGHTER-BOARD-PINOUT.md`** + **`STEW-BlackPill-pinout-diagram.jpg`**  
(Those are the daughter-board deliverable. This file is a short engineer map.)

**Version:** 2026-08-15d  

**“Yellow nets”** = MKII interface signals taken from the old PSoC pin list (highlighted yellow in Creator screenshots). Not a PCB color code. Full explanation is in the Stew doc.

**PSoC:** CY8C3246PVI-147 48-SSOP  
**STM32:** WeAct STM32F411CEU6 Black Pill  
**Logic:** 3.3 V only on STM32 I/O

---

## Mother-board yellow nets → STM32

| Yellow net | Dir | STM32 |
|------------|-----|-------|
| DOUT (from PCM3060) | in | **PB14** |
| DIN (to PCM3060) | out | **PB15** |
| BCK1 / BCK2 | out | **PB13** (tie both nets) |
| LRCK1 / LRCK2 | out | **PB12** (tie both nets) |
| SCK1 / SCK2 | out | **PC6** (tie both nets) |
| SDA / SCL | OD | **PB9 / PB8** |
| BS0 / BS1 / BS2 | out | **PA7 / PB5 / PB3** |
| LED1 | out | **PC13** (module) |
| RX / AMP | out | **PA1 / PB4** (active low) |
| **BOOT** | in | **PA8** |
| KEY_0 / KEY_1 | in | **PB0 / PB1** |
| **PTT** | **in** | **PA6** (sense) |

USB host: Black Pill PA11/PA12 (USB-C).

---

## Firmware

`firmware/pio/include/board_pins.h`
