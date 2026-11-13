# Windows — install

**This is not a Linux `.deb` kit.** Pi: [../rpi/](../rpi/). Ubuntu: [../linux/](../linux/).

## Download

The installer is **not in git**. Download it from the **Releases** page:
<https://github.com/w4mmp-ron/mscc-station/releases> (newest release at the top; older
releases stay there for roll-back).

Run the newest **`mscc-net9-R*-install.exe`** (built with Actual Installer) — currently
**`mscc-net9-R11-12-4-install.exe`** (title bar 11.11.4). Typical deploy folder: **`C:\mscc-net9`**.

That installer is the **WPF** client plus the Windows servers (ms-sdr 3.208, recv 3.153,
trans 3.158), the PSoC radio firmware, the Black Pill firmware + flash tools, and the keyer
firmware + update tool.

Factory calibration tables are installed next to the servers (`C:\mscc-net9\factory\`). The
radio keeps its own calibration (firmware with the calibration store); the client says at
connect if the radio firmware needs updating.

**Keyer (PIC on the daughter board):** close the servers, then Start menu **MSCC-NET9 →
Keyer Firmware Update** (`C:\mscc-net9\Keyer Firmware\keyer-flash-gui.exe`): Load File →
`Keyer-MCU-app-<date>.hex` → Program. Needs Black Pill 3.16 or PSoC Proficio MKII PTT 3.244 / ATU 4.245, Geminus MKII 2.154, Ultimus MKII PTT 8.244 / ATU 7.245 or later (PSoC ones not tested yet); settings and CQ
memories are kept. Command line: `keyer-flash.exe`. The `Keyer-MCU-full-` file is for the
PICkit only (bootloader + app).

Problem reports: https://multussdr.groups.io/g/main/topics

## Use

- **Local radio on this PC:** tick **Launch Servers** and press **Connect** (or start the
  Windows servers yourself and Connect).
- **Remote to a Pi or another PC:** untick Launch Servers and Connect to that computer's IP,
  UDP port **8888** (its servers already running).

How the installer is built: [`../../mscc-ui/installer/windows/README.md`](../../mscc-ui/installer/windows/README.md).
Old release notes (to R10.13.2): [`CHANGELOG.md`](CHANGELOG.md).
