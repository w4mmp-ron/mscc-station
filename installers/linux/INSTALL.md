# Ubuntu Desktop (x86_64) — install

**This is not the Pi kit.** Use [../rpi/](../rpi/) on a Raspberry Pi.

Need: Ubuntu Desktop amd64, Proficio on USB, PipeWire (do **not** `apt install pulseaudio`, and do **not** install any `*_arm64.deb` except `mscc-init-gui_*_all.deb`).

## 1. Packages (this folder)

```bash
sudo usermod -aG dialout,audio,plugdev "$USER"
# log out and back in

cd /path/to/this/folder
sudo apt install -y ./mscc-portaudio_*_amd64.deb    # first — Pulse+ALSA at /usr/local
chmod +x install-mscc.sh
./install-mscc.sh                                   # mscc_*_amd64.deb (servers)
sudo apt install -y ./mscc-init-gui_*_all.deb
sudo apt install -y ./mscc-ui_*_amd64.deb
```

Order: **PortAudio → servers → init-gui → UI**.

## 2. First run

1. **MSCC Init** — this PC’s speaker and mic. Digi stays VirtualA / VirtualB.monitor.
2. **MSCC Start**
3. **MSCC UI** — Connect `127.0.0.1` port **8888**

Everyday: `mscc start` / `mscc status` / `mscc stop`.

Firmware: BOOT jumper → Morse LOADER → menu **Firmware Upload** (or `bootloader file.cyacd`).

Longer laptop how-to (from source): [`../../INSTALL-UBUNTU.md`](../../INSTALL-UBUNTU.md).
