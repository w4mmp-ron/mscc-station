# BOOT configuration (daughter board)

**Date:** 2026-08-29  

BOOT is **not** a MKII mother-board net and is **not** on J5.  
On the PSoC design it lived only on the MCU daughter; the STM32 daughter keeps the same arrangement.

## Hardware

| Item | Spec |
|------|------|
| MCU pin | **PA8** |
| Location | **STM32 daughter board only** (header / pad / jumper) |
| Idle | Input with **internal pull-up** → pin high → **run application** |
| Bootloader | Short **PA8 → GND**, then **reset or power-cycle** → firmware jumps to STM32 **ROM DFU** |
| After flash | Remove jumper, reset → application runs |

Do **not** confuse this with the Black Pill module **BOOT0** pad (dev-only hardware entry). Production BOOT sense is **PA8** on the daughter.

## Why it exists

| Path | When to use |
|------|-------------|
| **PA8 BOOT jumper** | Recovery when the app will not enumerate USB |
| **USB `0xFE`** (`CMD_ENTER_BOOTLOADER`) | Normal field update while the app is running |
| Flash transport | **MKII mother-board USB jack** → J5 → PA11/PA12 (DFU via STM32CubeProgrammer) |

## Firmware hooks

- Pin macros: `BOARD_BOOT_*` → **PA8** (`firmware/pio/include/board_pins.h`)
- Early check: `system_boot_check_and_enter()` — if PA8 low, jump to ROM bootloader
- Host command: vendor **`0xFE`** → deferred jump to same ROM bootloader

## PCB note for Stew

Provide a **2-pin BOOT jumper** (or equivalent pad pair) on the daughter:

- Pin 1: **PA8**
- Pin 2: **GND**

Open = normal. Shorted at reset = DFU. No mother-board routing required.
