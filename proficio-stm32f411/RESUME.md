# Proficio STM32F411 — resume / handoff

**Updated:** 2026-08-15  
**Tree:** `worktrees/proficio-stm32f411`  
**Reference:** `worktrees/Release-Proficio-MKII-PTT` (logic only; no display/bias/legacy sensors)

---

## Status — firmware **complete** (product scope)

| Item | State |
|------|--------|
| `pio run` | **SUCCESS** (~24 KB flash) |
| USB vendor + UAC1 audio | Done |
| PCM3060 + I2S + SOF clock lock | Done |
| SI5351 LO, CW/keyer, band, PTT/GPIO | Done |
| Die temperature (`0xBF`) | Done (STM32 internal sensor) |
| ROM bootloader entry | Done — BOOT pin / `0xFE` / BOOT0; see `docs/BOOTLOADER.md` |
| Stew pinout | `docs/STEW-DAUGHTER-BOARD-PINOUT.md` |
| Hardware test | Pending Black Pill + mother board |

**Out of scope (old PSoC legacy — not required):** LCD, bias, potentia, external PA sensors, EEPROM cal store, PSoC bootloader.

---

## Build

```powershell
cd C:\Users\Ron\.grok\worktrees\proficio-stm32f411\firmware\pio
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run
```

---

## Next (hardware only)

1. Flash Black Pill  
2. USB enum + `CMD_GET_VERSION`  
3. I2C LO / keyer / codec  
4. Full IQ with ms-sdr  

Bring-up bugs expected; feature list is closed for this migration.
