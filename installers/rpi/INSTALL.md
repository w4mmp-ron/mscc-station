# Raspberry Pi OS (64-bit) — install

**This is not the Ubuntu kit.** Use [../linux/](../linux/) on an x86_64 PC.

Need: Pi **4 or 5**, 64-bit Raspberry Pi OS (desktop recommended).

## Packages (this folder)

| Package | Version | |
|---|---|---|
| `mscc-portaudio` | 19.8.3 | required, install first |
| `mscc` | 1.0.71 | required: servers, PSoC and Black Pill flash tools |
| `mscc-init` | 1.0.18 | recommended: MSCC Init (GUI + text wizard) |
| `mscc-ui` | 0.6.71 | optional: Avalonia UI on the Pi (not needed with the Windows client) |
| `mscc-volume` | 1.0.1 | optional: MSCC Volume, levels of VirtualA / VirtualB |
| `mscc-firmware` | 1.0.0 | optional: PSoC radio firmware files for Firmware Upload |

```bash
cd /path/to/this/folder
sudo apt install -y ./mscc-portaudio_*_arm64.deb   # first
sudo apt install -y ./mscc_*_arm64.deb             # servers + flash tools
sudo apt install -y ./mscc-init_*_all.deb          # MSCC Init
sudo apt install -y ./mscc-ui_*_arm64.deb          # optional
sudo apt install -y ./mscc-volume_*_all.deb        # optional
sudo apt install -y ./mscc-firmware_*_all.deb      # optional
```

Order: **PortAudio → mscc → mscc-init → the optional ones**.

Then: log out/in (first install) → **MSCC Init** → **MSCC Start** → connect the client
(Windows client to the Pi's IP, or mscc-ui on the Pi).

## Firmware upload

- PSoC radios: BOOT jumper → Morse LOADER → menu **MSCC → Firmware Upload**
  (`psoc-bootloader.gui`), or `psoc-bootloader file.cyacd`.
- Black Pill (mscc 1.0.69+, replaces `proficio-flash-tools`):
  `mscc stop; sudo stm32-app-flash app-firmware-async.bin` (USB, app) or
  `sudo stm32-bootloader-flash boot-app-firmware-async.bin` (ST-Link).

Full how-to: [`../../rpi/mscc-deb/INSTALL-FOR-PI.md`](../../rpi/mscc-deb/INSTALL-FOR-PI.md).
