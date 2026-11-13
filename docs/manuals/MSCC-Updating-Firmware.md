# Updating Firmware

2026-11-13

## What you can update

An MSCC radio has up to three parts with their own firmware. The MSCC installers carry the current file for each, so you never download firmware separately. Version numbers are in the **MSCC Software Versions Addendum**.

| Part | What it is | Which radios | File |
| --- | --- | --- | --- |
| PSoC radio firmware | The radio's own processor (Cypress PSoC) | Proficio, Geminus and Ultimus, Legacy and MKII, with the original processor | `<Radio>-<version>.cyacd` |
| Black Pill | The STM32 board that replaces the PSoC | Radios fitted with a Black Pill (25 MHz board) | `BlackPill-25MHz-<version>-app.bin` |
| Keyer | The PIC on the daughter board | MKII radios with the keyer daughter board | `Keyer-MCU-app-<date>.hex` |

A radio has either a PSoC or a Black Pill, never both. The keyer is updated through the radio, so the radio's firmware must be new enough first (see the Addendum for the minimum versions).

All three updates keep your calibration and settings. The keyer keeps its settings and CQ memories.

## Before you start

1. **Stop the MSCC servers.** The update tools need the radio's USB connection to themselves. The Black Pill and keyer tools refuse to run while the servers are up.
2. **Keep the radio's USB cable plugged in** to the computer that runs the servers.
3. **Pick the file for your radio.** The PSoC files are in one folder per radio; use the folder that matches your radio exactly (for example Proficio MKII PTT, not ATU).
4. **Don't unplug or power off** while an update runs. It takes under a minute.
5. **Move the PSoC boot shunt only with the radio off**.

## Windows

The Windows installer puts everything under `C:\mscc-net9`. Close the MSCC client and the servers first.

### PSoC radio firmware

No Zadig needed: the upload program uses a driver that comes with Windows. Don't run Zadig while the radio blinks LOADER.

1. Power the radio off. Leave the USB cable connected.
2. Put the shunt on the two boot pins of the BOOT1 header (the pair joined by a printed U on the board).
3. Power the radio on. The LED blinks **LOADER** in Morse. If not, power off and move the shunt.
4. Run `C:\mscc-net9\utilities\bootloader.exe` (window **USB Bootloader Host**). Wait until **USB HID State** shows **Connected**. Leave Vendor ID `4b4`, Product ID `b71d` and the Security Key as they are.
5. Click **Load File**, open `C:\mscc-net9\PSoC Firmware\<your radio>\`, pick the `.cyacd` file.
6. Click **Program**. The Status Log says **Bootload successful !!**.
7. Power off, take the shunt off the boot pins, power on.
8. Start MSCC and connect. The title bar shows the new **FW** number.

### Black Pill

1. Put the Black Pill in flash mode: fit the BOOT jumper and press **NRST**. The LED shows **B**.
2. Run `C:\mscc-net9\Black Pill Firmware\stm32-app-flash.exe` (window **STM32 App Flash**).
3. Click **List DFU devices** to check the pill is seen. "No Black Pill in flash mode found" means step 1 didn't take.
4. Click **Browse…**, pick `BlackPill-25MHz-<version>-app.bin` (not the `-boot-app` file).
5. Click **Burn application**.
6. Remove the BOOT jumper and press **NRST**. No power cycle needed.

This updates the application only; the bootloader is never touched (that needs an ST-Link: `stm32-bootloader-flash.cmd`).

### Keyer

1. Start menu **MSCC-NET9 → Keyer Firmware Update** (`C:\mscc-net9\Keyer Firmware\keyer-flash-gui.exe`).
2. Click **Load File…**, pick `Keyer-MCU-app-<date>.hex`.
3. Click **Program** and wait for it to finish.
4. **Check Keyer** shows the keyer's version.

Use the `Keyer-MCU-app-` file. The `Keyer-MCU-full-` file is for a PICkit programmer only (bootloader + application). Command line: `keyer-flash.exe <file>`.

## Raspberry Pi

The `mscc` package carries the tools and the firmware files. All three tools are in the desktop menu under **MSCC**. Stop the servers first (menu **MSCC Stop**, or `mscc stop`).

| Part | Menu entry | Files |
| --- | --- | --- |
| PSoC radio firmware | **Firmware Upload** | `/usr/share/mscc/firmware/<Radio>/` |
| Black Pill | **Black Pill Firmware Update** | `/usr/share/mscc/firmware/black-pill/` |
| Keyer | **Keyer Firmware Update** | `/usr/share/mscc/firmware/keyer/` |

No `sudo` is needed: the package installs the USB rules for all three.

### PSoC radio firmware

1. Power the radio off, put the shunt on the boot pins, power on. The LED blinks **LOADER** (same as Windows, steps 1–3).
2. Menu **MSCC → Firmware Upload**.
3. **Load File** opens in the firmware folder: pick your radio's `.cyacd` file.
4. Wait for **Connected**, then **Program**.
5. Power off, take the shunt off, power on. Start the servers and check the **FW** number in the client.

Command line: `psoc-bootloader /usr/share/mscc/firmware/<Radio>/<file>.cyacd`

### Black Pill

No jumper needed: the tool switches the pill into flash mode by itself.

1. Menu **MSCC → Black Pill Firmware Update**.
2. It lists only the application files (`BlackPill-25MHz-<version>-app.bin`). Pick one and start the update.
3. At the end it waits for the radio and shows the new firmware version.

Command line: `stm32-app-flash /usr/share/mscc/firmware/black-pill/BlackPill-25MHz-<version>-app.bin`

If it ends with the pill still in the bootloader (LED **B**), remove the BOOT jumper and press **NRST**.

### Keyer

1. Menu **MSCC → Keyer Firmware Update**.
2. **Load File…** → `Keyer-MCU-app-<date>.hex` → **Program**.
3. **Check Keyer** shows the keyer's version.

Command line: `keyer-flash /usr/share/mscc/firmware/keyer/Keyer-MCU-app-<date>.hex`

## Debian / Ubuntu

The Debian / Ubuntu (amd64) kit is being brought up to date and isn't on the Releases page yet. When it is, its `mscc` package is planned to carry the same tools, firmware files and menu entries as the Raspberry Pi, and the steps will be the same as the **Raspberry Pi** section above.

## If something goes wrong

| What you see | What to do |
| --- | --- |
| "The MSCC servers are running" | Stop the servers (Windows: close the client and servers; Pi: `mscc stop`), then start the tool again. |
| PSoC: LED doesn't blink LOADER | The shunt is on the wrong pins. Power off, move it, power on. |
| PSoC: USB Bootloader Host never shows **Connected** | Check the USB cable and the LOADER blink. On Windows, don't run Zadig while in LOADER. |
| Black Pill: "No Black Pill in flash mode found" (Windows) | Fit the BOOT jumper and press **NRST** (LED **B**), then try again. |
| Black Pill: "Error during download get_status" right after the burn | Normal: the pill resets at once. |
| Black Pill: LED stays **B** after the update | Remove the BOOT jumper and press **NRST**. |
| Keyer tool can't find the keyer | Update the radio firmware (PSoC or Black Pill) first; the keyer update needs the minimum version in the Addendum. |
| An update stopped part way | Run it again. The bootloader is never changed, so the radio can always take a new upload (PSoC: LOADER; Black Pill: flash mode). |

Still stuck? Post on the Multus forum, <https://multussdr.groups.io/g/main/topics>, with the tool's messages.
