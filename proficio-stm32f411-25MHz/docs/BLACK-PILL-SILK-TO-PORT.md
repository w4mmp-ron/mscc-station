# WeAct Black Pill V2.0 — silk to STM32 port

Board: **STM32F411CEU6 WeAct Black Pill V2.0**  
Orientation: **USB-C at top**, component side facing you.

Silk short form to full port:

| Silk | Port |
|------|------|
| `A#` | **PA#** |
| `B#` | **PB#** |
| `C#` | **PC#** |
| `R` | **NRST** |
| `G` | **GND** |
| `5V` / `3.3` | power |

---

## Left header (USB-C at top to bottom)

| # | Silk | Port |
|---|------|------|
| 1 | B12 | PB12 |
| 2 | B13 | PB13 |
| 3 | B14 | PB14 |
| 4 | B15 | PB15 |
| 5 | A8 | PA8 |
| 6 | A9 | PA9 |
| 7 | A10 | PA10 |
| 8 | A11 | PA11 |
| 9 | A12 | PA12 |
| 10 | A15 | PA15 |
| 11 | B3 | PB3 |
| 12 | B4 | PB4 |
| 13 | B5 | PB5 |
| 14 | B6 | PB6 |
| 15 | B7 | PB7 |
| 16 | B8 | PB8 |
| 17 | B9 | PB9 |
| 18 | 5V | 5V |
| 19 | G | GND |
| 20 | 3.3 | 3.3V |

## Right header (USB-C at top to bottom)

| # | Silk | Port |
|---|------|------|
| 1 | C13 | PC13 |
| 2 | C14 | PC14 |
| 3 | C15 | PC15 |
| 4 | A0 | PA0 |
| 5 | A1 | PA1 |
| 6 | A2 | PA2 |
| 7 | A3 | PA3 |
| 8 | A4 | PA4 |
| 9 | A5 | PA5 |
| 10 | A6 | PA6 |
| 11 | A7 | PA7 |
| 12 | B0 | PB0 |
| 13 | B1 | PB1 |
| 14 | B2 | PB2 |
| 15 | B10 | PB10 |
| 16 | 3.3 | 3.3V |
| 17 | G | GND |
| 18 | 5V | 5V |

---

## Multus triad

| Function | Silk | Port |
|----------|------|------|
| Codec **RESET** | A2 | **PA2** |
| **BOOT** (short to GND for ROM DFU sense) | A8 | **PA8** |
| **USBV+** sense | A9 | **PA9** |

## Other useful pins

| Silk | Port | Notes |
|------|------|--------|
| A0 | PA0 | onboard KEY button |
| C13 | PC13 | onboard LED |
| A11 / A12 | PA11 / PA12 | USB D- / D+ (also on USB-C) |
| R | NRST | MCU reset |

**Not on the dual headers:** onboard **BOOT0** pad (separate from silk **A8** / firmware BOOT).  
SWD (4-pin end): 3V3, SWDIO (PA13), SWCLK (PA14), GND.

Source photo: `STM32F411CEU6_WeAct_Black_Pill_V2.0-2.jpg`
