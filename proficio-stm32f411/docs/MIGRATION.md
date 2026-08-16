# Migration guide: PSoC 3 Proficio → STM32F411

## System context (unchanged host)

```text
MSCC client  ←UDP→  ms-sdr  ←USB→  [ radio MCU ]  ←I2C→  PIC keyer
                                      ↑
                               PSoC 3 today
                               STM32F411 target
```

ms-sdr expects:

- USB **vendor** control transfers (`usbvend` opcodes: freq, CW, keyer mem `0x9C`, etc.)
- USB **audio** endpoints (IQ RX/TX via PCM3060 path on PSoC)
- Stable VID/PID behavior if possible (today Multus Proficio IDs)

## What the PSoC firmware does

| Domain | Role |
|--------|------|
| **USBFS** | Device: vendor + audio interfaces, DMA to/from I2S |
| **I2S + PCM3060** | Codec IQ path |
| **SI5351** | LO / clocks |
| **I2C** | Keyer PIC, display, bias, sensors |
| **GPIO / timers** | Band, PTT, CW hold, QSK pop |
| **App logic** | `main` loop: audio, band, CW config, USB vendor handlers |

Application C (port these *logically*):  
`main.c`, `usbvend01.c`, `cw.c`, `audio.c`, `pcm3060.c`, `si5351a.c`, `band.c`, `tx.c`, `display.c`, …

## Mapping strategy

### Keep

- Opcode map (`usbvend.h` / ms-sdr `usbavrcmd.h`) — host stays stable.
- Keyer I2C framing: one-byte writes, cmd then param; memory `0x9C` 2-byte USB `[param,seq]`.
- SI5351 programming sequences (after I2C works).

### Replace

| PSoC | STM32F411 |
|------|-----------|
| USBFS component + DMA | TinyUSB or Cube USB Device + DMA |
| I2S component + DMA TDs | SPI/I2S peripheral + DMA |
| I2C_DISPLAY master | I2C1/I2C2 HAL |
| CW_Hold_Timer / isr | TIM + IRQ |
| FracN / SyncSOF Verilog | PLL/timer + USB SOF feedback (later) |
| Keil 8051 build | arm-none-eabi + CMake/CubeIDE |

### Hard problems (plan explicitly)

1. **USB Audio + isochronous + SOF sync** — largest risk.  
2. **Pin/electrical adapter** from Black Pill to Proficio RF board (levels, connectors).  
3. **Real-time audio** CPU budget vs control loop.  
4. **Bootloader** — PSoC bootloadable vs DFU/OpenBootloader on STM32.

## Compatibility options

| Mode | Description |
|------|-------------|
| **A. Protocol clone** | Same USB vendor requests + audio layout → ms-sdr works unmodified. |
| **B. Staged** | Vendor-only first; audio still on PSoC or muted until Phase 6. |

Recommend **B** for first hardware bring-up.

## Reference tree

Do not copy `Generated_Source` into this project. Cite paths under:

`../Release-Proficio-MKII-PTT/Proficio-MKII-PTT.cydsn/`
