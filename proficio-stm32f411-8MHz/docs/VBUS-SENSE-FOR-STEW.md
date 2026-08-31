# USBV+ / VBUS sense — note for Stew (STM32 daughter)

**Date:** 2026-08-31  
**MCU module:** WeAct Black Pill STM32F411.  
**Schematic:** Stew *STM32F411CEU6-PCIe-CW* rev **6.0** — **USBV+ not yet routed to an MCU pin.**

---

## Mother / connector

| Connector net | J5 / U2 | Role |
|---------------|---------|------|
| **USBV+** | **B8** | Host USB VBUS to daughter (sense only) |
| **USB+** | (data) | D+ → **PA12** |
| **USB−** | (data) | D− → **PA11** |

**USBV+ is not the radio 5 V rail.** Pill **5V** comes from the daughter regulator.

---

## Status on rev 6.0

- **USB±** → PA12/PA11: done on schematic.  
- **USBV+** arrives on **U2 B8** from the mother.  
- **Still needed:** route **U2 B8 → resistor divider → free GPIO** (3.3 V max at the MCU).  
  **Do not use PA9** — that is **PCM3060 RESET**.  
  FW placeholder: **PB10** (`BOARD_VBUS_SENSE_*` / `vbus_sense_present()`).  
  Sense is **optional** — bare Black Pill USB-C still enumerates without it.

---

## Why it matters

Pill stays powered from the board regulator across a **PC reboot**. Sensing USBV+ lets firmware drop/re-init USB without unplugging.

---

## Stew checklist

- [ ] **USBV+** (U2/J5 **B8**) ≠ short to regulator **5V** / pill 5V  
- [ ] **USBV+** → divider → **chosen GPIO** (not PA9)  
- [ ] Tell firmware which GPIO was chosen  

**One-liner:** *USBV+ on U2 B8; still needs divider → free GPIO for sense (PA9 is RESET).*
