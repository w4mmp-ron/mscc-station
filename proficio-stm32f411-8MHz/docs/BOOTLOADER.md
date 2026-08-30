# Firmware update (STM32 ROM bootloader)

Same **concept** as PSoC BOOT jumper + bootload tool.  
**Tools:** ST **STM32CubeProgrammer** (free, off the shelf) — not the PSoC bootload app.

## Ways to enter bootloader

### A — Black Pill BOOT0 (hardware, no app needed)
1. Set **BOOT0** = 1 (jumper/pad on Black Pill).  
2. Press **NRST** (or power-cycle).  
3. Connect USB (DFU) or ST-Link.  
4. Flash with **STM32CubeProgrammer**.  
5. Set BOOT0 = 0, reset → runs app.

### B — Mother-board BOOT pin (PSoC-style, firmware)
- Net **BOOT** → MCU **PA8** (pull-up).  
- Hold **BOOT low** (jumper), power-up/reset → app jumps to ROM bootloader.  
- Then CubeProgrammer as above.

### C — USB command (host-initiated)
- Vendor request **`CMD_ENTER_BOOTLOADER` = `0xFE`** (host-to-device).  
- Device re-enters ROM bootloader; host uses CubeProgrammer DFU.  
- Requires app already running and USB enumerated.

### D — ST-Link SWD (development)
- No BOOT needed: `pio run -t upload` or CubeProgrammer via ST-Link.

## CubeProgrammer
Download: [STM32CubeProgrammer](https://www.st.com/en/development-tools/stm32cubeprog.html)

Connect mode: **USB** (DFU) after A/B/C, or **ST-Link** for D.
