# Raspberry Pi (Ron)

Pi OS **arm64** servers, packaging, and install kit. Ubuntu laptop work is **not** here — see [`../linux/`](../linux/) and [`../INSTALL-UBUNTU.md`](../INSTALL-UBUNTU.md).

Treat this folder as the **Pi source of truth**. For Ubuntu-only fixes, copy/adapt into `linux/` instead of editing these trees.

| Path | Role |
|------|------|
| `ms-sdr-linux/` | Command hub |
| `SDRcore-recv-linux/` / `SDRcore-trans-linux/` | RX / TX DSP |
| `mscc-deb/` / `mscc-binaries/` | `mscc_*_arm64.deb` (AArch64 ELFs) |
| `pi-install/` | Operator kit + [`INSTALL.md`](pi-install/INSTALL.md) |
| `psoc-usb-bootload-linux/` | Firmware upload: **`bootloader-gui.py`** (GUI) + `make` → **`bootloader`** (C CLI, gitignored). Pi binary: `mscc-binaries/bootloader` |
| `mscc-init-gui/` / `mscc-init-linux/` / `mscc-init-files-linux/` | Init |
| `mscc-portaudio/` | Pulse+ALSA PortAudio for the Pi |
| `tty0tty-master/` | CAT null-modem |

Build on the Pi (no extra Ubuntu scripts):

```bash
cd rpi/SDRcore-recv-linux  && make clean && make
cd ../SDRcore-trans-linux && make clean && make
cd ../ms-sdr-linux        && make clean && make
```

Longer checklist: [`mscc-deb/BUILD-SERVERS-ON-PI.md`](mscc-deb/BUILD-SERVERS-ON-PI.md).
