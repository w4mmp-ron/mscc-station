# Updating Firmware

2026-11-24

## What you can update

An MSCC radio has up to three parts with their own firmware. The MSCC installers carry the current file for each, so you never download firmware separately. Version numbers are in the **MSCC Software Versions Addendum**.

| Part | What it is | Which radios | File | How to burn it |
| --- | --- | --- | --- | --- |
| PSoC radio firmware | The radio's own processor (Cypress PSoC) | Proficio, Geminus and Ultimus, Legacy and MKII, with the original processor | `<Radio>-<version>.cyacd` | [**MSCC Burning PSoC Firmware**](https://github.com/w4mmp-ron/mscc-station/blob/main/docs/MSCC-Burning-PSoC-Firmware.md) |
| Black Pill | The STM32 board that replaces the PSoC | Radios fitted with a Black Pill (25 MHz board) | `BlackPill-25MHz-<version>-app.bin` | [**MSCC Burning Black Pill Firmware**](https://github.com/w4mmp-ron/mscc-station/blob/main/docs/MSCC-Burning-Black-Pill-Firmware.md) |
| Keyer | The PIC on the daughter board | MKII radios with the keyer daughter board | `Keyer-MCU-app-<date>.hex` | This manual, below |

A radio has either a PSoC or a Black Pill, never both. The keyer is updated through the radio, so the radio's firmware must be new enough first (see the Addendum for the minimum versions).

The PSoC and Black Pill guides cover Windows, Raspberry Pi and Ubuntu. Each covers normal updates, burning a new (virgin) chip, and where each installer puts the files.

All three updates keep your calibration and settings. The keyer keeps its settings and CQ memories.

## Before you start

1. **Stop the MSCC servers.** The update tools need the radio's USB connection to themselves. The Black Pill and keyer tools refuse to run while the servers are up.
2. **Keep the radio's USB cable plugged in** to the computer that runs the servers.
3. **Don't unplug or power off** while an update runs. It takes under a minute.

## Keyer

### Windows

The installer puts the keyer tool and firmware in `C:\mscc-net9\Keyer Firmware\`. Close the MSCC client and the servers first.

1. Start menu **MSCC-NET9 → Keyer Firmware Update** (`C:\mscc-net9\Keyer Firmware\keyer-flash-gui.exe`).
2. Click **Load File…**, pick `Keyer-MCU-app-<date>.hex`.
3. Click **Program** and wait for it to finish.
4. **Check Keyer** shows the keyer's version.
5. **Power the radio off and on.** There is no RF output until you do.

Use the `Keyer-MCU-app-` file. The `Keyer-MCU-full-` file is for a PICkit programmer only (bootloader + application). Command line: `keyer-flash.exe <file>`.

### Raspberry Pi and Ubuntu

The `mscc` package carries the tool and the firmware files, in `/usr/share/mscc/firmware/keyer/`. No `sudo` is needed. Stop the servers first (menu **MSCC Stop**, or `mscc stop`).

1. Menu **MSCC → Keyer Firmware Update**.
2. **Load File…** → `Keyer-MCU-app-<date>.hex` → **Program**.
3. **Check Keyer** shows the keyer's version.
4. **Power the radio off and on.** There is no RF output until you do.

Command line: `keyer-flash /usr/share/mscc/firmware/keyer/Keyer-MCU-app-<date>.hex`

## If something goes wrong

| What you see | What to do |
| --- | --- |
| "The MSCC servers are running" | Stop the servers (Windows: close the client and servers; Pi / Ubuntu: `mscc stop`), then start the tool again. |
| Keyer tool can't find the keyer | Update the radio firmware (PSoC or Black Pill) first; the keyer update needs the minimum version in the Addendum. |
| No RF output after a keyer update | Power the radio off and on. |
| PSoC or Black Pill problems | See the troubleshooting table at the end of that guide. |

Still stuck? Post on the Multus forum, <https://multussdr.groups.io/g/main/topics>, with the tool's messages.
