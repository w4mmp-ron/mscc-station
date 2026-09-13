# Burn bootloader + Proficio app (25 MHz Black Pill)

## Files (this folder)

| File | Address | Size role |
|------|---------|-----------|
| `bootloader.bin` | **`0x08000000`** | 32 KB DFU bootloader alone |
| `proficio-stm32f411-25MHz-YYYY-MM-DD.bin` | **`0x08008000`** | App only (field update) |
| `proficio-stm32f411-25MHz-YYYY-MM-DD-full.bin` | **`0x08000000`** | **Virgin / combined** (BL + app) |

Every `pio run` builds the **app** and **full** dated images (needs `bootloader.bin` in this folder).

Scripts: `flash-bootloader.sh` · `flash-proficio.sh`

## Order (ST-Link / CubeProgrammer / OpenOCD)

1. Erase chip (recommended once).
2. Flash **bootloader** at `0x08000000`.
3. Flash **app** `proficio-stm32f411-25MHz-YYYY-MM-DD.bin` at `0x08008000` (not at 0x08000000).
4. Reset (no KEY held) → should run Multus / fast LED when USB up.

## Enter flash DFU later (updates)

Hold **KEY (PA0)** + NRST → release KEY → PC13 fast blink → `dfu-util`  
Then download **app only** to `0x08008000`.

## Notes

- This is **Cube USB-DFU IAP**, not ST OpenBL.
- Do **not** use A8/PA8 / ST ROM soft-jump for normal updates.
- App naming stays **`proficio-stm32f411-25MHz-YYYY-MM-DD.bin`** (still linked at `0x08008000`).
