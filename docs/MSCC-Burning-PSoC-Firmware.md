# Burning PSoC Radio Firmware

2026-11-24

For radios with the original Cypress PSoC processor: Proficio, Geminus and Ultimus, Legacy and MKII. A radio fitted with a Black Pill uses **MSCC Burning Black Pill Firmware** instead.

## The two kinds of burn

| Burn | When | Tool | File |
| --- | --- | --- | --- |
| **Update** | The radio already has its LOADER (every shipped radio) | USB cable and the boot shunt | `<Radio>-<version>.cyacd` |
| **Virgin PSoC** | A new chip, or one whose LOADER is gone | MiniProg3 and PSoC Programmer (Windows only) | `<Radio>-<version>.hex` |

- The `.cyacd` file is the radio firmware only. It needs the LOADER (the PSoC's bootloader), and an update never changes the LOADER.
- The `.hex` file is the whole chip: LOADER and firmware together. Burn it only on a virgin chip.
- **Pick the folder that matches your radio exactly**, for example Proficio MKII PTT, not ATU. The wrong file loads, but the radio won't work properly.

Each MSCC installer carries the newest firmware for every radio, so you never download it separately.

## Before you start

1. **Stop the MSCC servers.** The tools need the radio's USB connection to themselves.
2. Keep the radio's USB cable plugged in to the computer that runs the servers.
3. **Move the boot shunt only with the radio off.**
4. Don't unplug or power off during a burn. It takes under a minute.
5. An update keeps your calibration and settings.

### Putting the radio in LOADER mode (updates)

1. Power the radio off. Leave the USB cable connected.
2. Put the shunt on the two boot pins of the **BOOT1** header (the pair joined by a printed U on the board).
3. Power the radio on. The LED blinks **LOADER** in Morse. If it doesn't, power off and move the shunt.

After the update: power off, take the shunt off the boot pins, and power on.

## Windows

The installer puts everything under `C:\mscc-net9`:

| What | Where |
| --- | --- |
| Firmware files, one folder per radio | **`C:\mscc-net9\PSoC Firmware\<Radio>\`** (`.cyacd` and `.hex`) |
| USB updater | `C:\mscc-net9\utilities\bootloader.exe` (window **USB Bootloader Host**) |

Close the MSCC client and the servers first.

### Update over USB

No Zadig is needed: the updater uses a driver that comes with Windows. Don't run Zadig while the radio blinks LOADER.

1. Put the radio in LOADER mode (see **Before you start**).
2. Run `bootloader.exe`. Wait until **USB HID State** shows **Connected**. Leave Vendor ID `4b4`, Product ID `b71d` and the Security Key as they are.
3. Click **Load File**, open `C:\mscc-net9\PSoC Firmware\<your radio>\` and pick the `.cyacd` file.
4. Click **Program**. The Status Log says **Bootload successful !!**.
5. Power off, take the shunt off the boot pins, and power on.
6. Start MSCC and connect. The title bar shows the new **FW** number.

### Virgin PSoC (MiniProg3)

This is factory work. You need a **MiniProg3** and Cypress (Infineon) **PSoC Programmer**, which is a free download. The MSCC installer doesn't include it.

1. Connect the MiniProg3 to the radio's PSoC programming header and to the PC.
2. Open PSoC Programmer and select the MiniProg3 under **Port Selection**.
3. Click **File Load** and pick `C:\mscc-net9\PSoC Firmware\<your radio>\<Radio>-<version>.hex`.
4. Click **Program**, and wait for it to finish and verify.
5. Disconnect the MiniProg3, power the radio off and on, and connect with MSCC. The title bar shows the **FW** number.

From then on, updates go over USB with the `.cyacd` file.

## Raspberry Pi

The `mscc` package installs the updater, the firmware and the USB rules. No `sudo` is needed.

| What | Where |
| --- | --- |
| Firmware files, one folder per radio | **`/usr/share/mscc/firmware/<Radio>/`** (`.cyacd` and `.hex`) |
| USB updater | Menu **MSCC → Firmware Upload**, or `psoc-bootloader` |

Stop the servers first: menu **MSCC Stop**, or `mscc stop`.

### Update over USB

1. Put the radio in LOADER mode (see **Before you start**).
2. Menu **MSCC → Firmware Upload**.
3. **Load File** opens in the firmware folder. Pick your radio's `.cyacd` file.
4. Wait for **Connected**, then click **Program**.
5. Power off, take the shunt off the boot pins, and power on. Start the servers and check the **FW** number in the client.

Command line:
```
psoc-bootloader /usr/share/mscc/firmware/<Radio>/<Radio>-<version>.cyacd
```

### Virgin PSoC

PSoC Programmer runs on Windows only. Burn a virgin PSoC on a Windows PC (see **Windows**), then update it from the Pi as normal.

## Ubuntu

The Ubuntu `mscc` package (amd64) installs the same updater, firmware and USB rules as the Pi, in the same places:

| What | Where |
| --- | --- |
| Firmware files, one folder per radio | **`/usr/share/mscc/firmware/<Radio>/`** (`.cyacd` and `.hex`) |
| USB updater | Menu **Firmware Upload**, or `psoc-bootloader` |

Stop the servers first (`mscc stop`), then follow the **Raspberry Pi** steps above. A virgin PSoC is burned on Windows, as on the Pi.

## If something goes wrong

| What you see | What to do |
| --- | --- |
| "The MSCC servers are running" | Stop the servers, then run the updater again. |
| The LED doesn't blink LOADER | The shunt is on the wrong pins. Power off, move it, power on. |
| The updater never shows **Connected** | Check the USB cable and the LOADER blink. On Windows, don't run Zadig while in LOADER. |
| The radio doesn't work after the update | Check you used the folder for your exact radio, and update again with the right file. |
| An update stopped part way | Run it again. An update never changes the LOADER, so the radio can always take a new one. |
| No LOADER blink at all, even with the shunt right | The LOADER may be gone. The chip needs the virgin burn with a MiniProg3. |

Still stuck? Post on the Multus forum, <https://multussdr.groups.io/g/main/topics>, with the tool's messages.
