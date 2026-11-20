# Note for Stew: flashing a new Black Pill (2026-11-19)

A brand-new pill has no bootloader, so the USB flasher (stm32-app-flash) cannot work on it yet.
The first burn must be done with the ST-Link, the same one Ron uses. Do not use the ST built-in
loader (KEY + NRST, or BOOT0). It does not work for this.

## What you need

- A Windows PC with STM32CubeProgrammer installed. It is free from ST, and it includes the
  ST-Link driver.
- The ST-Link, wired to the pill's SWD header: SWDIO, SWCLK, GND and 3.3 V.
- The BOOT0 and PA8 jumpers off.

## Steps

1. Close the MSCC servers.
2. Open PowerShell in the repo, in `firmware-programmers\windows\stm32-flash\`.
3. Run:
   ```powershell
   .\stm32-bootloader-flash.ps1 -Image combined
   ```
4. When it finishes, unplug the ST-Link and plug in the rig USB. The radio should show
   firmware 3.24.

After this first burn, updates go over USB with the normal flasher, and Windows finds the driver
by itself. Zadig is not needed.

## For Grok

- Use the ST-Link and the script above only. Never use the ST built-in ROM loader (KEY + NRST,
  BOOT0, or dfu-util at 0x08000000). It does not work for this.
- Never use stm32-app-flash or dfu-util on a new pill. They write only the application (at
  0x08008000), which needs our bootloader, and a new pill does not have it.
- `-Image combined` burns the newest `BlackPill-25MHz-<version>-boot-app.bin` from
  `psoc-replacement-stm32\proficio-stm32f411-25MHz\release\` at 0x08000000. That file holds the
  bootloader and the application together. Today the newest is 3.24.
- The script finds `STM32_Programmer_CLI.exe` in the normal STM32CubeProgrammer install folder.
  If it is installed somewhere else, add `-CubeCli <path to STM32_Programmer_CLI.exe>`.
- It connects over SWD under reset, writes the file, verifies it and resets the pill. Success is
  "Download verified successfully."
- If it says "No STM32 target found", check the four wires and the 3.3 V supply, then run it again.
  A slower link can help: add `-FreqKHz 1000`.
- The script only looks in the 25 MHz release folder. For an 8 MHz pill, give the file by hand:
  `.\stm32-bootloader-flash.ps1 -BinPath <path to the 8 MHz boot-app .bin>`, taken from
  `psoc-replacement-stm32\proficio-stm32f411-8MHz\release\`. Do not mix the two.
