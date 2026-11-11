# Windows — install

**This is not a Linux `.deb` kit.** Pi: [../rpi/](../rpi/). Ubuntu: [../linux/](../linux/).

## Download

The installer is **not in git**. Download it from the **Releases** page:
<https://github.com/w4mmp-ron/mscc-station/releases> (newest release at the top; older
releases stay there for roll-back).

Run the newest **`mscc-net9-R*-install.exe`** (built with Actual Installer) — currently
**`mscc-net9-R11-10-14-install.exe`** (title bar 11.10.14). Typical deploy folder: **`C:\mscc-net9`**.

That installer is the **WPF** client plus the Windows servers (ms-sdr 3.206, recv 3.153,
trans 3.158), the PSoC radio firmware and the Black Pill firmware + flash tools.

Factory calibration tables are installed next to the servers (`C:\mscc-net9\factory\`). The
radio keeps its own calibration (firmware with the calibration store); the client says at
connect if the radio firmware needs updating.

Problem reports: https://multussdr.groups.io/g/main/topics

## Use

- **Local radio on this PC:** tick **Launch Servers** and press **Connect** (or start the
  Windows servers yourself and Connect).
- **Remote to a Pi or another PC:** untick Launch Servers and Connect to that computer's IP,
  UDP port **8888** (its servers already running).

How the installer is built: [`../../mscc-ui/installer/windows/README.md`](../../mscc-ui/installer/windows/README.md).
Old release notes (to R10.13.2): [`CHANGELOG.md`](CHANGELOG.md).
