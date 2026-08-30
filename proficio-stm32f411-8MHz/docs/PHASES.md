# Implementation phases

## Product scope (closed)

Firmware feature set is **complete** for MKII STM32 migration:

USB vendor + audio, PCM3060/I2S/SOF lock, SI5351, CW/keyer, band/PTT/GPIO, die temp.

**Not in scope:** LCD, bias, potentia, external temp sensors, EEPROM settings, PSoC bootloader (legacy).

---

## Phase 0 — Scaffold & bring-up

- [x] Tree, docs, pin map, PlatformIO  
- [x] `pio run` SUCCESS  
- [ ] Flash to hardware + LED  

## Phase 1–4 — Control path

- [x] USB vendor, LO, CW/keyer, band/PTT  
- [ ] Hardware verify  

## Phase 5 — Temperature

- [x] STM32 die temp → `CMD_GET_TRANSCEIVER_TEMP` (0xBF)  
- [x] ~~Display / bias / potentia~~ — **dropped (legacy)**  

## Phase 6 — Audio

- [x] PCM3060, I2S DMA, UAC1, SyncSOF-style PLL trim  
- [ ] Hardware IQ path with ms-sdr  

---

## Remaining work = hardware bring-up only
