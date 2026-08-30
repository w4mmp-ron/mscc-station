# Codec RESET pin (J5 → Black Pill)

**Date:** 2026-08-29  

Assignment for Stew / daughter layout. This note supersedes the **TBD** on J5 **A28** in older pin tables until those are revised.

## Mapping

| Item | Value |
|------|--------|
| J5 pin | **A28** |
| Mother-board net | **RESET** |
| Meaning | **PCM3060 codec reset** (not MCU **NRST**) |
| Black Pill | **PA2** |
| Direction | MCU **output** |
| Polarity | Confirm vs PSoC / PCM3060 schematic (typically active-low hold-in-reset) |

## Notes

- Wire **J5 A28 → PA2** on the daughter.
- Do **not** connect this net to the Black Pill **NRST** pin.
- Firmware: `BOARD_CODEC_RESET_*` → **PA2** in `board_pins.h`; GPIO init in `control.c`; pulse in `PCM3060_Init()`.

## Related

- Full connector map: `docs/J5-BLACK-PILL-PINMAP.md` (A28 still listed TBD until that file is edited)
- Net tables: `docs/STEW-DAUGHTER-BOARD-PINOUT.md`
