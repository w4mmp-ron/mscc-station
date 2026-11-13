# Raspberry Pi OS (64-bit) — install

**This is not the Ubuntu kit.** Use [../linux/](../linux/) on an x86_64 PC.

Need: Pi **4 or 5**, 64-bit Raspberry Pi OS (desktop recommended).

## Download

The packages are **not in git**. Download them from the **Releases** page:
<https://github.com/w4mmp-ron/mscc-station/releases> (newest release at the top; older
releases stay there for roll-back). Put the `.deb` files in one folder on the Pi.

| Package | Version | |
|---|---|---|
| `mscc` | 1.0.88 | required: servers (ms-sdr 3.206, recv 3.153, trans 3.158), PortAudio, MSCC Init, MSCC Volume, PSoC + Black Pill + keyer firmware and flash tools (window "Black Pill Firmware Update" since 1.0.85) |
| `mscc-avalonia` | 1.6.96 | optional: MSCC Avalonia client on the Pi (not needed with the Windows client) |

```bash
cd /path/to/the/downloaded/debs
sudo apt install -y ./mscc_*_arm64.deb             # everything for the radio
sudo apt install -y ./mscc-avalonia_*_arm64.deb    # optional
```

Since mscc 1.0.79 the old separate packages (`mscc-portaudio`, `mscc-init`, `mscc-volume`,
`mscc-firmware`) are part of `mscc`; installing it removes them. Installing `mscc` asks before it
stops running servers (answer `y`).

Then: log out/in (first install) → **MSCC Init** → **MSCC Start** → connect the client
(Windows client to the Pi's IP, or MSCC Avalonia on the Pi).

## Firmware upload

- PSoC radios: BOOT jumper → Morse LOADER → menu **MSCC → Firmware Upload**
  (`psoc-bootloader.gui`), or `psoc-bootloader file.cyacd`.
  PSoC files: `/usr/share/mscc/firmware/<Radio>/` (same versions as the Windows installer).
- Black Pill (25 MHz board), files in `/usr/share/mscc/firmware/black-pill/`:
  `mscc stop; sudo stm32-app-flash /usr/share/mscc/firmware/black-pill/BlackPill-25MHz-3.16-app.bin`
  (USB, app) or `sudo stm32-bootloader-flash /usr/share/mscc/firmware/black-pill/BlackPill-25MHz-3.16-boot-app.bin` (ST-Link).
- Keyer (PIC on the daughter board; Black Pill 3.16, or PSoC Proficio MKII PTT 3.244 / ATU 4.245,
  Geminus MKII 2.154, Ultimus MKII PTT 8.244 / ATU 7.245 or later; the PSoC ones not tested yet): stop the servers (`mscc stop`), then
  menu **MSCC → Keyer Firmware Update** (`keyer-flash.gui`): Load File → Program. Or
  `keyer-flash /usr/share/mscc/firmware/keyer/Keyer-MCU-app-20261112.hex`. Settings and CQ
  memories are kept. The `Keyer-MCU-full-` file is for the PICkit only (bootloader + app).

Full how-to: [`../../rpi/mscc-deb/INSTALL-FOR-PI.md`](../../rpi/mscc-deb/INSTALL-FOR-PI.md).
