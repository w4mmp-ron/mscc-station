# Phase 0 — Bring-up

## Goal

Blink **PC13** on the Black Pill; SYSCLK **84 MHz** (USB 48 MHz PLLQ ready; HSE 25 MHz).

## Hardware

- WeAct STM32F411CEU6 Black Pill  
- ST-Link (SWDIO/SWCLK/GND/3V3)  
- USB-C optional for power  

No Proficio wiring required yet.

---

## A — PlatformIO (generated & **builds**)

Project: `firmware/pio/`

```bash
cd worktrees/proficio-stm32f411-25MHz/firmware/pio
pio run              # already verified SUCCESS
pio run -t upload    # ST-Link connected
```

**VS Code:** open folder `firmware/pio` with PlatformIO extension → Build / Upload.

App: `src/main.c` — HAL blink PC13, HSE 25 MHz → 84 MHz SYSCLK / 48 MHz USB.

---

## B — STM32CubeIDE

1. Install STM32CubeIDE.  
2. Open `firmware/cubeide/Proficio_F411.ioc` (or *New project from ioc*).  
3. **Generate Code** (downloads CubeF4 pack if needed).  
4. Merge blink from `firmware/pio/src/main.c` if the generated `main.c` is empty of app logic.  
5. Build & run with ST-Link.

Details: `firmware/cubeide/README.md`.

---

## Verify

- [ ] `pio run` → SUCCESS  
- [ ] LED blinks ~1 Hz after upload  
- [ ] SWD stable  

## Exit → Phase 1

USB device enumerate + vendor control stub (TinyUSB or Cube USB).

Pin map: [`STEW-DAUGHTER-BOARD-PINOUT.md`](STEW-DAUGHTER-BOARD-PINOUT.md) · J5: [`J5-BLACK-PILL-PINMAP.md`](J5-BLACK-PILL-PINMAP.md)
