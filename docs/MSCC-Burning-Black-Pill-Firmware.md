# Burning Black Pill Firmware

2026-11-24

For radios fitted with a Black Pill (the 25 MHz STM32 board that replaces the PSoC). A radio with the original PSoC uses **MSCC Burning PSoC Firmware** instead.

## The two kinds of burn

| Burn | When | Tool | File |
| --- | --- | --- | --- |
| **Update** | The pill already runs MSCC firmware | USB cable, no extra hardware | `BlackPill-25MHz-<version>-app.bin` |
| **Virgin pill** | A brand-new pill, or one whose bootloader is gone | ST-Link programmer | `BlackPill-25MHz-<version>-boot-app.bin` |

- The `-app` file is the firmware only. It needs our bootloader, which a new pill doesn't have.
- The `-boot-app` file is the bootloader and the firmware together. Burn it once on a new pill. After that, every update goes over USB.
- `bootload-PA8.bin` is the bootloader alone. Normally you don't need it.
- **Use an ST-Link for a virgin pill. Don't use ST's built-in loader** (BOOT0 or KEY + NRST, or dfu-util at 0x08000000). It doesn't work for this.

Each MSCC installer carries the newest firmware, so you never download it separately.

## Before you start

1. **Stop the MSCC servers.** The tools need the radio's USB connection to themselves.
2. Don't unplug or power off during a burn. It takes under a minute.
3. Calibration and settings are kept.

### ST-Link wiring (virgin pill only)

- Connect SWDIO, SWCLK, GND and 3.3 V to the pill's SWD header.
- Power the pill from one source only. If it is in a powered radio, or has its own USB cable plugged in, leave the 3.3 V wire off.
- Take the BOOT0 and PA8 (BOOT) jumpers off.

### After a virgin burn

Unplug the ST-Link and plug in the radio's USB cable.

- **In a radio:** the LED is solid on, and MSCC shows the new firmware version.
- **Bare pill, not in a radio:** the LED blinks the letter **C** in Morse. That is normal.

## Windows

The installer puts the tools and the firmware in **`C:\mscc-net9\Black Pill Firmware\`**:

| File | What it is |
| --- | --- |
| `stm32-app-flash.exe` | USB updater (window **STM32 App Flash**) |
| `stm32-bootloader-flash.cmd` / `.ps1` | ST-Link burner (virgin pill) |
| `BlackPill-25MHz-<version>-app.bin` | Firmware for an update |
| `BlackPill-25MHz-<version>-boot-app.bin` | Bootloader and firmware for a virgin pill |
| `bootload-PA8.bin` | Bootloader alone |

Close the MSCC client and the servers first.

### Update over USB

1. Fit the BOOT jumper and press **NRST**. The LED blinks **B** (flash mode).
2. Run `stm32-app-flash.exe` and click **List DFU devices**.
   - If it shows "No Black Pill in flash mode found", check that the LED blinks **B** (step 1).
3. Click **Browse…** and pick `BlackPill-25MHz-<version>-app.bin` (not the `-boot-app` file).
4. Click **Burn application**.
5. Remove the BOOT jumper and press **NRST**. No power cycle is needed.

"Error during download get_status" right after the burn is normal: the pill resets at once.

### Virgin pill (ST-Link)

You also need **STM32CubeProgrammer**. It is free from ST and includes the ST-Link driver.

1. Wire the ST-Link (see **Before you start**).
2. Open a Command Prompt in `C:\mscc-net9\Black Pill Firmware\`.
3. Run:
   ```
   stm32-bootloader-flash.cmd -Image combined
   ```
   **`-Image combined` is required.** Without it, the script burns the bootloader alone.
4. Success ends with "OK: bootloader programmed at 0x08000000 and verified."
5. Unplug the ST-Link and plug in the radio's USB cable.

Windows sets up the pill's driver by itself. Zadig is not needed.

If it says "No STM32 target found", check the four wires and the 3.3 V supply. A slower link can help: add `-FreqKHz 1000`.

## Raspberry Pi

The `mscc` package installs the tools, the firmware and the USB rules. It also installs `dfu-util` and `stlink-tools`, which the tools use.

| What | Where |
| --- | --- |
| Firmware files | **`/usr/share/mscc/firmware/black-pill/`** |
| USB updater | Menu **MSCC → Black Pill Firmware Update**, or `stm32-app-flash` |
| ST-Link burner | `stm32-bootloader-flash` |

The commands are in `/usr/bin`. The scripts behind them are in `/usr/share/mscc/flash-tools/`.

Stop the servers first: menu **MSCC Stop**, or `mscc stop`.

### Update over USB

No jumper is needed. The tool puts the pill into flash mode by itself.

1. Menu **MSCC → Black Pill Firmware Update**.
2. Pick `BlackPill-25MHz-<version>-app.bin` and start the update.
3. At the end it waits for the radio and shows the new firmware version.

Command line:
```
stm32-app-flash /usr/share/mscc/firmware/black-pill/BlackPill-25MHz-<version>-app.bin
```

If it ends with the LED still blinking **B**, remove the BOOT jumper and press **NRST**.

### Virgin pill (ST-Link)

1. Wire the ST-Link (see **Before you start**) and plug it into the Pi.
2. Run:
   ```
   sudo stm32-bootloader-flash /usr/share/mscc/firmware/black-pill/BlackPill-25MHz-<version>-boot-app.bin
   ```
3. Success ends with "OK: programmed at 0x08000000 and verified."
4. Unplug the ST-Link and plug in the radio's USB cable.

If it says "no ST-Link found" or "no STM32F4 answering", check the USB cable, the four wires and the 3.3 V supply.

## Ubuntu

The Ubuntu `mscc` package (amd64) installs the same tools, firmware and USB rules as the Pi, in the same places:

| What | Where |
| --- | --- |
| Firmware files | **`/usr/share/mscc/firmware/black-pill/`** |
| USB updater | Menu **Black Pill Firmware Update**, or `stm32-app-flash` |
| ST-Link burner | `stm32-bootloader-flash` |

Stop the servers first (`mscc stop`). Then follow the **Raspberry Pi** steps above. The commands and file names are the same.

## If something goes wrong

| What you see | What to do |
| --- | --- |
| "The MSCC servers are running" | Stop the servers, then run the tool again. |
| "Error during download get_status" after a USB burn | Normal: the pill resets at once. |
| The LED keeps blinking **B** after an update | Remove the BOOT jumper and press **NRST**. |
| The USB updater can't find a new pill | A new pill has no bootloader. Do the virgin burn with the ST-Link first. |
| The ST-Link can't find the pill | Check SWDIO, SWCLK, GND and 3.3 V, and that the pill has power from one source only. |
| An update stopped part way | Run it again. A USB update never touches the bootloader, so the pill can always take a new one. |

Still stuck? Post on the Multus forum, <https://multussdr.groups.io/g/main/topics>, with the tool's messages.
