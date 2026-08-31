# Codec RESET pin (U2 / J5 → Black Pill → PCM3060)

**Date:** 2026-08-31  
**Source:** Stew schematic *STM32F411CEU6-PCIe-CW* rev **6.0**

## Mapping

| Item | Value |
|------|--------|
| Daughter connector | **U2** pin **A28** |
| Mother connector | **J5** pin **A28** (mates with U2) |
| Mother-board net | **RESET** → **PCM3060** |
| Black Pill | **PA9** (MCU output) |
| Polarity | Active-low |

## Notes

- Daughter: **U2 A28 → PA9**.  
- Mother: **J5 A28 → PCM3060 RESET**.  
- Do **not** tie to Black Pill **NRST**.  
- Firmware: `BOARD_CODEC_RESET_*` → **PA9**; pulsed in `PCM3060_Init()`.
