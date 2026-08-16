# Proficio → STM32F411 Black Pill

**New project:** migrate Multus **Proficio MKII-PTT** firmware from **PSoC 3** (8051 + USBFS + I2S DMA) to **STM32F411CEU6 Black Pill** (Cortex-M4F).

| | Path |
|--|------|
| **This tree** | `worktrees/proficio-stm32f411` |
| **PSoC reference (read-only source of truth)** | `worktrees/Release-Proficio-MKII-PTT/Proficio-MKII-PTT.cydsn` |
| **Host (unchanged goal)** | ms-sdr USB vendor protocol + audio path |

**Start here:** [`RESUME.md`](RESUME.md) → [`docs/MIGRATION.md`](docs/MIGRATION.md) → [`docs/PHASES.md`](docs/PHASES.md)

---

## Goals

1. Replace PSoC 3 application firmware with STM32F411 while keeping **ms-sdr / client protocol** stable where possible.
2. Port subsystems in **phases** (bring-up → USB vendor → LO → I2C/keyer → audio last).
3. Document pin map, clocks, and USB so hardware bring-up is deliberate (Black Pill is a dev board; radio I/O is an adapter problem).

## Non-goals (initial)

- Bit-identical PSoC Verilog (FracN, SyncSOF) — re-implement in STM32 timers/PLL/USB SOF sync as needed.
- Day-one full IQ USB audio — hard; schedule after control path works.
- Changing ms-sdr or PIC keyer unless a protocol gap forces it.

## Toolchain (planned)

- **STM32F411CEU6** @ 96–100 MHz (USB-friendly clock tree)
- **CMake** + **arm-none-eabi-gcc** (and/or STM32CubeIDE)
- **STM32 HAL/LL** or bare register (choose in Phase 0)
- Debug: SWD (ST-Link)

```text
proficio-stm32f411/
  README.md / RESUME.md
  docs/           # migration architecture, phases, pinout
  firmware/       # STM32 application skeleton
  reference/      # pointers to PSoC modules to port
```

## Status

**Buildable Phase 0 project** (PlatformIO + STM32Cube HAL) — LED blink on Black Pill.  
**Verified:** `pio run` succeeds (`firmware.elf` / `.bin`).

| How to work | Path |
|-------------|------|
| **Build/flash (recommended)** | `firmware/pio/` — VS Code + PlatformIO, or CLI `pio run` |
| **STM32CubeIDE** | Open/generate from `firmware/cubeide/Proficio_F411.ioc` |

See `RESUME.md` and `docs/PHASE0-BRINGUP.md`.
