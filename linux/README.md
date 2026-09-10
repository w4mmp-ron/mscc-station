# Ubuntu x86_64 (this laptop)

Ron’s **Pi** trees live under [`rpi/`](../rpi/). Treat those as a **guide only**.  
Do **not** edit `rpi/` for Ubuntu laptop fixes.

This folder is the Ubuntu Desktop (amd64) working copy: same product, different host (distro PortAudio/Pulse, x86-64 binaries, user-session icons).

| Path | Role |
|------|------|
| `SDRcore-recv-linux/` | RX DSP |
| `SDRcore-trans-linux/` | TX DSP |
| `ms-sdr-linux/` | command hub |
| `mscc-init-linux/` | CLI init |
| `psoc-usb-bootload-linux/` | firmware **CLI** (`make` → `bootloader`) + **GUI** (`bootloader-gui.py`) |
| `helpers/` | `mscc.sh`, status, virtual-audio, desktop-ctl, bootloader-gui launcher |
| `tty0tty-master/` | CAT null-modem |
| `udev/` | Proficio USB rules |
| `mscc-init-files-linux/` | config seed |

Share kit (GitHub web): [`../installers/linux/`](../installers/linux/).  
Build/install: [`../linux-build/README.md`](../linux-build/README.md) and [`../INSTALL-UBUNTU.md`](../INSTALL-UBUNTU.md).  
Output is **`$HOME/mscc`** (x86-64). Never copy those ELFs into [`../rpi/mscc-binaries/`](../rpi/mscc-binaries/).

## Firmware upload: two files (not the same)

| File | What it is |
|------|------------|
| **`bootloader`** | Compiled **C** CLI (HID `.cyacd` upload). Produced by `make`. **Not in git** (see `.gitignore`). Pi ships an **AArch64** copy in `rpi/mscc-binaries/bootloader`. Ubuntu `make` produces **x86-64** into `$HOME/mscc/bootloader`. |
| **`bootloader-gui.py`** | **Python/Tk** GUI. It runs the CLI `bootloader`. Same script the Pi package installs as `bootloader-gui`. |

Pi and Ubuntu may need different **binaries** (CPU + hidapi). The GUI script can stay shared until Ubuntu needs its own changes — then edit `linux/psoc-usb-bootload-linux/bootloader-gui.py`, not `rpi/`.
