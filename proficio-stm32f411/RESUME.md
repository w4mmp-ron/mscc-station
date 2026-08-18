# Proficio STM32F411 — resume / handoff

**Updated:** 2026-08-17  
**Tree:** `worktrees/proficio-stm32f411`  
**Reference:** `worktrees/Release-Proficio-MKII-PTT` (logic only; no display/bias/legacy sensors)  
**PIC keyer:** `worktrees/keyer` (stays external for this migration)

---

## Status — firmware **complete** (product scope)

| Item | State |
|------|--------|
| `pio run` | **SUCCESS** (~24 KB flash) |
| USB vendor + UAC1 audio | Done |
| PCM3060 + I2S + SOF clock lock | Done |
| SI5351 LO, CW/keyer **bridge**, band, PTT/GPIO | Done (keyer = I²C to PIC) |
| Die temperature (`0xBF`) | Done (STM32 internal sensor) |
| ROM bootloader entry | Done — BOOT pin / `0xFE` / BOOT0; see `docs/BOOTLOADER.md` |
| Stew pinout | `docs/STEW-DAUGHTER-BOARD-PINOUT.md` |
| Hardware test | Pending Black Pill + mother board |
| Absorb PIC keyer into STM | **Not planned for first board** — see below |

**Out of scope (old PSoC legacy — not required):** LCD, bias, potentia, external PA sensors, EEPROM cal store, PSoC bootloader.

---

## PIC keyer vs STM — absorb feasibility

**Today:** STM is a USB → I²C bridge (`Configure_CW` / `keyer_write` @ `KEYER_I2C_ADDR`). The **PIC16F18326** owns iambic A/B, straight, weight/spacing, NCO sidetone, paddle feel, CQ memory (`0x9C`), Farnsworth (`0x76`), and EEPROM.

**Absorb into STM?** Technically **yes** (F411 has flash/CPU). Not a drop-in:

| Concern | Why it matters |
|---------|----------------|
| Hard real-time | Element timing must use a **timer ISR**; USB audio / SyncSOF / LO already load the core. PIC is dedicated. |
| Sidetone | No PIC NCO → TIM PWM / DAC / codec path; feel must be retuned. |
| Paddles / key | STM would drive key directly; I²C slave protocol goes away. |
| Storage | CQ slots → F411 flash pages or external EEPROM. |
| Host opcodes | USB `0x71`…`0x7F`, `0x9C`, `0x76` can stay; only the I²C hop disappears. |

**Bring-up policy:** keep the **PIC + I²C bridge** until radio/USB/audio are proven. Onboard keyer is a later deliberate phase, not a free merge.

### Board spin implication

A **firmware-only** “phase 1 = PIC / phase 2 = onboard” plan **without hardware foresight ⇒ second board spin** when the PIC is removed (BOM, no I²C keyer, sidetone/keying on STM).

**One-spin escape hatch:** first daughter designs the PIC as **optional** (footprint + DNP):

- Populate PIC → phase 1 (current bridge)  
- Leave PIC off; STM owns paddles/key/sidetone → phase 2  

Same PCB, two population options. If spin 1 assumes “PIC always present,” absorbing the keyer later means another board.

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
