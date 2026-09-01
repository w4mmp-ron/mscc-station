# USBV+ / VBUS sense — STM32 daughter (locked)

**Date:** 2026-08-31  
**MCU module:** WeAct Black Pill STM32F411.  
**Locked PCB:** Stew *STM32F411CEU6-PCIe-CW* — **USBV+ → divider → PA9**.

---

## Mother / connector

| Connector net | J5 / U2 | Role |
|---------------|---------|------|
| **USBV+** | **B8** | Host USB VBUS to daughter (**Proficio name** for host USB 5 V) |
| **USB+** | (data) | D+ → **PA12** |
| **USB−** | (data) | D− → **PA11** |

**USBV+ is not the radio 5 V rail.** Pill **5V** comes from the daughter regulator.  
**USBV+** must be dropped through a **resistor divider to ≤ 3.3 V** at the MCU pin.

---

## Locked MCU pin

| Item | Value |
|------|--------|
| Connector | **U2 / J5 B8** |
| Sense GPIO | **PA9** (MCU **input**) |
| Conditioning | Resistor divider → 3.3 V logic at PA9 |

**Do not use PA9 for PCM3060 RESET** — RESET is **PA2** (`CODEC-RESET-PIN.md`).

Firmware: `BOARD_VBUS_SENSE_*` should track **PA9** (Ron — code update separate from this doc lock). Older **PB10** placeholder is obsolete for this board.

---

## Why it matters

Pill stays powered from the board regulator across a **PC reboot**. Sensing USBV+ lets firmware drop/re-init USB without unplugging. Sense is **optional** for first bring-up (USB still enumerates) but **routed on this PCB**.

---

## Checklist

- [x] **USBV+** (U2/J5 **B8**) routed to divider → **PA9** (schematic + PCB)  
- [ ] Confirm divider ratios: PA9 never exceeds **3.3 V**  
- [ ] **USBV+** ≠ short to regulator **5V** / pill 5V  
- [ ] FW macros match **PA9** (Ron)

**One-liner:** *USBV+ on U2 B8 → divider → **PA9**; RESET is **PA2**; BOOT is **PA8**.*

## Related locked triad

| Net | STM32 |
|-----|--------|
| **RESET** (codec) | **PA2** |
| **BOOT** | **PA8** |
| **USBV+** (sense) | **PA9** |
