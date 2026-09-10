# Raspberry Pi OS (64-bit) — install

**This is not the Ubuntu kit.** Use [../linux/](../linux/) on an x86_64 PC.

Need: Pi **4 or 5**, 64-bit Raspberry Pi OS (desktop recommended).

## Packages (this folder)

```bash
cd /path/to/this/folder
chmod +x install-mscc.sh
sudo apt install -y ./mscc-portaudio_*_arm64.deb    # first
./install-mscc.sh                                    # mscc_*_arm64.deb
sudo apt install -y ./mscc-init-gui_*_all.deb
sudo apt install -y ./mscc-ui_*_arm64.deb           # optional if you use Windows WPF
```

Order: **PortAudio → servers → init-gui → UI**.

Then: log out/in → **MSCC Init** → **MSCC Start** → UI at `127.0.0.1:8888` (or Windows WPF to the Pi’s IP).

Firmware: BOOT jumper → Morse LOADER → **Firmware Upload**.

Full how-to: [`../../rpi/pi-install/INSTALL.md`](../../rpi/pi-install/INSTALL.md).
