# Raspberry Pi OS (64-bit) — install

**This is not the Ubuntu kit.** Use [../linux/](../linux/) on an x86_64 PC.

Need: Pi **4 or 5**, 64-bit Raspberry Pi OS (desktop recommended).

## Download

The packages are **not in git**. Download them from the **Releases** page:
<https://github.com/w4mmp-ron/mscc-station/releases> (newest release at the top; older
releases stay there for roll-back). Put the `.deb` files in one folder on the Pi.

| Package | Version | |
|---|---|---|
| `mscc` | 1.0.82 | required: servers (ms-sdr 3.206, recv 3.153, trans 3.158), PortAudio, MSCC Init, MSCC Volume, PSoC + Black Pill firmware and flash tools |
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
  `mscc stop; sudo stm32-app-flash /usr/share/mscc/firmware/black-pill/BlackPill-25MHz-3.15-app.bin`
  (USB, app) or `sudo stm32-bootloader-flash /usr/share/mscc/firmware/black-pill/BlackPill-25MHz-3.15-boot-app.bin` (ST-Link).

Full how-to: [`../../rpi/mscc-deb/INSTALL-FOR-PI.md`](../../rpi/mscc-deb/INSTALL-FOR-PI.md).
