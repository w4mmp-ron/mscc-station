# Proficio → STM32F411 Black Pill

**New project:** migrate Multus **Proficio MKII-PTT** firmware from **PSoC 3** (8051 + USBFS + I2S DMA) to **STM32F411CEU6 Black Pill** (Cortex-M4F).

| | Path |
|--|------|
| **This tree** | `worktrees/proficio-stm32f411-8MHz` |
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
- **Absorbing the PIC keyer into STM firmware on the first daughter board** — see [Keyer (PIC vs STM)](#keyer-pic-vs-stm) below and `RESUME.md`.

## Toolchain (planned)

- **STM32F411CEU6** @ 96–100 MHz (USB-friendly clock tree)
- **CMake** + **arm-none-eabi-gcc** (and/or STM32CubeIDE)
- **STM32 HAL/LL** or bare register (choose in Phase 0)
- Debug: SWD (ST-Link)

```text
proficio-stm32f411-8MHz/
  README.md / RESUME.md
  docs/           # migration architecture, phases, pinout
  firmware/       # STM32 application skeleton
  reference/      # pointers to PSoC modules to port
```

## Status

**Product-scope firmware complete** (PlatformIO) — USB vendor + UAC1, PCM3060/I2S/SyncSOF, SI5351, CW **bridge** to PIC keyer, band/PTT, die temp, ROM bootloader.  
**Hardware bring-up** still pending (Black Pill + mother board).  
**Verified:** `pio run` succeeds (`firmware.elf` / `.bin`).

| How to work | Path |
|-------------|------|
| **Build/flash (recommended)** | `firmware/pio/` — VS Code + PlatformIO, or CLI `pio run` |
| **STM32CubeIDE** | Open/generate from `firmware/cubeide/Proficio_F411.ioc` |

See `RESUME.md` and `docs/PHASE0-BRINGUP.md`.

---

## Keyer (PIC vs STM)

**Current architecture (keep for first spin):**

```text
Host USB opcodes  →  STM32 (Configure_CW / keyer_write)  →  I²C  →  PIC16F18326
```

The PIC owns paddle feel, element timing, NCO sidetone, CQ memory (`0x9C`), Farnsworth (`0x76`), and EEPROM. STM does **not** replace that logic in product-scope firmware.

**Could the keyer move into STM later?** Yes, technically — but it is a real port (timer-ISR timing, sidetone without NCO, flash for memories, direct key/paddle drive), not a copy-paste. Do **not** attempt during first radio bring-up.

**Board spin:** A phased firmware plan (PIC now, onboard later) **without designing for both ⇒ second PCB spin** when the PIC is deleted.

**One-spin option:** make the PIC footprint **optional (DNP)** on the first daughter — populate for phase 1; leave off and run onboard keyer for phase 2. Same PCB, two BOMs.

Detail: [`RESUME.md`](RESUME.md) (section *PIC keyer vs STM*). PIC source: `worktrees/keyer`.
