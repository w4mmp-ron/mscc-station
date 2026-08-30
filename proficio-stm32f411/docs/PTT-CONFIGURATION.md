# PTT configuration (daughter board)

**Date:** 2026-08-29  

PTT is **not** a MKII mother-board / J5 net for this design.  
It lives on the **STM32 daughter board** only (same idea as the PSoC daughter PTT sense).

## Mapping

| Item | Value |
|------|--------|
| Net | **PTT** (footswitch / PTT sense) |
| Location | **Daughter board only** — header, jack, or pad as Stew designs |
| Black Pill | **PA6** |
| Direction | MCU **input** |
| Idle | **Pull-up** → pin high = not keyed |
| Active | Pin **low** = PTT asserted (active-low assumed; confirm vs hardware) |

## Notes for Stew

- Provide a daughter-board connector or pad pair for PTT (e.g. tip/sleeve jack or 2-pin header).
- Route **PTT sense → PA6**. Do **not** put PTT on J5 unless the radio design changes.
- This is **sense only** — the MCU does not drive PTT out on this pin.
- Do **not** confuse with band/TX control nets (**RX**, **AMP**) which *are* mother-board outputs on J5.

## Firmware

- Pin macros: `BOARD_PTT_*` → **PA6** (`firmware/pio/include/board_pins.h`)
- Status bit: `STATUS_PTT` / `E_PTT` (see `control.c`)

## Related

- BOOT (also daughter-local): `docs/BOOT-CONFIGURATION.md`
- J5 map (PTT listed as “not on crop” — correct; it is not a J5 net): `docs/J5-BLACK-PILL-PINMAP.md`
