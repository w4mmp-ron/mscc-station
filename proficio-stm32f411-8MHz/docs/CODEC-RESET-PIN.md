# Codec RESET pin (U2 / J5 → Black Pill → PCM3060)

**Date:** 2026-08-31  
**Locked PCB map:** Stew *STM32F411CEU6-PCIe-CW* (schematic + PCB)

## Mapping

| Item | Value |
|------|--------|
| Daughter connector | **U2** pin **A28** |
| Mother connector | **J5** pin **A28** (mates with U2) |
| Mother-board net | **RESET** → **PCM3060** |
| Black Pill | **PA2** (MCU output) |
| Polarity | Active-low |

## Notes

- Daughter: **U2 A28 → PA2**.  
- Mother: **J5 A28 → PCM3060 RESET**.  
- Do **not** tie to Black Pill **NRST**.  
- Do **not** confuse with **PA9** — that is **USBV+** sense (see `VBUS-SENSE-FOR-STEW.md`).  
- Firmware: `BOARD_CODEC_RESET_*` must match **PA2** (Ron — code update separate from this doc lock).

## Related locked triad

| Net | STM32 |
|-----|--------|
| **RESET** (codec) | **PA2** |
| **BOOT** | **PA8** |
| **USBV+** (sense) | **PA9** |
