# Ubuntu Desktop (x86_64) — install

**This is not the Pi kit.** On a Raspberry Pi, use the Raspberry Pi install guide.

Need: Ubuntu Desktop amd64, the radio on USB, PipeWire. Do not `apt install pulseaudio`. Do not install any `*_arm64.deb`.

## 1. Groups

```bash
sudo usermod -aG dialout,audio,plugdev "$USER"
```

Log out and back in.

## 2. Packages (this folder)

```bash
cd /path/to/this/folder
sudo apt install -y ./mscc_1.0.105_amd64.deb
sudo apt install -y ./mscc-avalonia_1.7.08_amd64.deb
```

`mscc_1.0.105_amd64.deb` is the servers, PortAudio, MSCC Init, MSCC Volume, the firmware files, and the flash tools. It replaces the old `mscc-portaudio`, `mscc-firmware`, and `mscc-init-gui` packages.

`mscc-avalonia_1.7.08_amd64.deb` is the client.

If the old client is still installed:

```bash
sudo apt remove mscc-ui
```

For the version list, see the Software Versions Addendum.

## 3. First run

1. **MSCC Init** — this PC's speaker and mic. Digi stays VirtualA / VirtualB.monitor.
2. **MSCC Start**
3. **MSCC Avalonia** — Connect `127.0.0.1` port **8888**

Everyday: `mscc start` / `mscc status` / `mscc stop`.

## 4. WSJT-X audio

Local (radio on this PC): Input `VirtualA.monitor`, Output `VirtualB`.

Remote Digital (MSCC Avalonia to a Pi): set WSJT-X Output to the device chosen as the Digital mic in Remote Audio (Stew used `VirtualB_TX`). The wrong output keys the radio with no signal.

Firmware: BOOT jumper, Morse LOADER, then menu **Firmware Upload** (or `psoc-bootloader file.cyacd`). **Load File** opens `/usr/share/mscc/firmware/<radio>/`.

Calibration: factory tables ship in `/usr/share/mscc/factory/`. After calibrating, press **Save settings** in MSCC Avalonia. The server keeps that radio's calibration under `~/.local/mscc/cal/<line>/`.

What changed: [`CHANGELOG.md`](CHANGELOG.md). Problem reports: https://multussdr.groups.io/g/main/topics
