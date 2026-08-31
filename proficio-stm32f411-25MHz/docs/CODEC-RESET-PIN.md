# Codec RESET pin (U2 / J5 → Black Pill → PCM3060)

**Date:** 2026-08-31  

**Assigned.** Also in `STEW-DAUGHTER-BOARD-PINOUT.md` and `J5-BLACK-PILL-PINMAP.md`.

## Mapping

| Item | Value |
|------|--------|
| Daughter connector | **U2** pin **A28** |
| Mother connector | **J5** pin **A28** (mates with U2) |
| Mother-board net | **RESET** → **PCM3060** |
| Black Pill | **PA2** (MCU output) |
| Polarity | Confirm vs schematic (typically active-low) |

## Notes

- Daughter: **U2 A28 → PA2**.  
- Mother: **J5 A28 → PCM3060 RESET**.  
- Do **not** tie to Black Pill **NRST**.  
- Firmware: `BOARD_CODEC_RESET_*` → **PA2**; pulsed in `PCM3060_Init()`.
