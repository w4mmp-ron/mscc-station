# Raspberry Pi OS (64-bit) — install

**This is not the Ubuntu kit.** Use [../linux/](../linux/) on an x86_64 PC.

Need: Pi **4 or 5**, 64-bit Raspberry Pi OS (desktop recommended).

## Packages (this folder)

```bash
cd /path/to/this/folder
chmod +x install-mscc.sh
sudo apt install -y ./mscc-portaudio_*_arm64.deb    # first
./install-mscc.sh                                    # mscc_*_arm64.deb
sudo apt install -y ./mscc-init_*_all.deb        # MSCC Init: GUI + text wizard
sudo apt install -y ./mscc-ui_*_arm64.deb           # optional if you use Windows WPF
sudo apt install -y ./proficio-flash-tools_*_all.deb # optional: Black Pill flash tools (ST-Link / USB DFU)
sudo apt install -y ./mscc-firmware_*_all.deb        # optional: radio (PSoC) firmware files for the USB Bootloader
```

Order: **PortAudio → servers → mscc-init → UI**.

Then: log out/in → **MSCC Init** → **MSCC Start** → UI at `127.0.0.1:8888` (or Windows WPF to the Pi’s IP).

Firmware: BOOT jumper → Morse LOADER → **Firmware Upload**.

Full how-to: [`../../rpi/mscc-deb/INSTALL-FOR-PI.md`](../../rpi/mscc-deb/INSTALL-FOR-PI.md).
