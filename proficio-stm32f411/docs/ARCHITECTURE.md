# Target architecture (STM32F411)

```text
                    ┌─────────────────────────────────────┐
                    │           STM32F411CEU6             │
  USB FS ──────────►│  USB device (vendor + later audio)  │
                    │           │                         │
                    │     App task / main loop            │
                    │      ├─ vendor opcode dispatch      │
                    │      ├─ LO control                  │
                    │      ├─ CW / keyer I2C              │
                    │      └─ band / PTT GPIO             │
                    │           │                         │
                    │  I2C ─────┼──► PIC keyer            │
                    │           ├──► SI5351               │
                    │           ├──► PCM3060 (ctrl)       │
                    │           └──► display / bias       │
                    │  I2S+DMA ────► PCM3060 data         │
                    └─────────────────────────────────────┘
```

## Suggested layering

```text
firmware/
  platform/     # clocks, USB, I2C, I2S, GPIO (STM32-specific)
  src/board/    # Black Pill pin defs, board_init
  src/app/      # radio logic ported from Proficio app C
  include/      # shared config, opcode headers (align with usbvend.h)
```

## Main loop (mirrors PSoC style)

1. Poll USB (or USB IRQs fill queues).
2. Service vendor command queue.
3. Configure_CW / keyer I2C drain.
4. Band / PTT / housekeeping.
5. Audio callbacks driven by DMA/USB (Phase 6).

## Clock (planning)

- HSE 25 MHz on many Black Pills (verify silk/crystal).
- PLL → **96 MHz** SYSCLK common for USB 48 MHz.
- Confirm your module’s HSE before locking Cube clock config.
