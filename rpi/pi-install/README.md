# Pi install kit (start here)

This kit lives under **`rpi/pi-install/`**. Ubuntu Desktop (x86_64) is a separate tree: [`../../linux/`](../../linux/) and [`../../INSTALL-UBUNTU.md`](../../INSTALL-UBUNTU.md).

**One place** for current Raspberry Pi `.deb` packages and install instructions.

| File / folder | Purpose |
|---------------|---------|
| **[INSTALL.md](INSTALL.md)** | **Full how-to** — servers, init, UI, remote audio, firmware upload, upgrades |
| **[INSTALL.pdf](INSTALL.pdf)** | **Printable** copy of the how-to (regenerate with `python md_to_pdf.py`) |
| **`packages/`** | Latest release `.deb` files (copy this folder to a USB stick or `scp` to the Pi) |
| **`collect-packages.ps1`** | Refresh `packages/` from the build trees (`.\rpi\pi-install\collect-packages.ps1` from repo root) |
| **`install-mscc.sh`** | Optional helper (same as `rpi/mscc-deb/install-mscc.sh`) |

### Current package set (see `packages/`)

| Order | Package | Example file |
|-------|---------|----------------|
| 1 | PortAudio | `mscc-portaudio_19.8.2_arm64.deb` |
| 2 | Servers + bootloader | `mscc_1.0.41_arm64.deb` |
| 3 | Setup wizard | `mscc-init-gui_1.0.13_all.deb` |
| 4 | Avalonia UI (optional on Pi) | `mscc-ui_0.6.41_arm64.deb` |

Open **[INSTALL.md](INSTALL.md)** and follow it top to bottom.
