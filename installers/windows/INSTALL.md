# Windows — install

**This is not a Linux `.deb` kit.** Pi: [../rpi/](../rpi/). Ubuntu: [../linux/](../linux/).

## Download

The installer is **not in git**. Download it from the **Releases** page:
<https://github.com/w4mmp-ron/mscc-station/releases> (newest release at the top; older
releases stay there for roll-back).

Run the newest **`mscc-net9-R*-install.exe`** (built with Actual Installer) — currently
**`mscc-net9-R11-16-1-install.exe`** (title bar 11.15.0). Typical deploy folder: **`C:\mscc-net9`**.

That installer is the **WPF** client plus the Windows servers (versions: see the Software
Versions Addendum), the PSoC radio firmware, the Black Pill firmware + flash tools, and the keyer
firmware + update tool.

**USB driver:** a radio with Black Pill 3.17 or later needs no Zadig: Windows installs its own
driver (WinUSB) when the radio is plugged in. PSoC radios and older Black Pill firmware still need
the Zadig driver once per PC (Operator's Guide 2.4). A PC that already has it keeps working.

Factory calibration tables are installed next to the servers (`C:\mscc-net9\factory\`). The
radio keeps its own calibration (firmware with the calibration store); the client says at
connect if the radio firmware needs updating.

**Keyer (PIC on the daughter board):** close the servers, then Start menu **MSCC-NET9 →
Keyer Firmware Update** (`C:\mscc-net9\Keyer Firmware\keyer-flash-gui.exe`): Load File →
`Keyer-MCU-app-<date>.hex` → Program. Needs Black Pill 3.16 or later, or the MKII PSoC firmware in
the Addendum; settings and CQ memories are kept. **Power-cycle the radio afterwards** (no RF
until then). Command line: `keyer-flash.exe`. The `Keyer-MCU-full-` file is for the
PICkit only (bootloader + app).

Problem reports: https://multussdr.groups.io/g/main/topics

## Use

- **Local radio on this PC:** tick **Launch Servers** and press **Connect** (or start the
  Windows servers yourself and Connect).
- **Remote to a Pi or another PC:** untick Launch Servers and Connect to that computer's IP,
  UDP port **8888** (its servers already running).

## Uninstall

Close the client and the servers, then Settings → Apps → **MSCC-NET9** → Uninstall (or run
`C:\mscc-net9\Uninstall.exe`). It removes the program files and shortcuts; your settings in
`%LOCALAPPDATA%\MSCC-NET9` stay, so a reinstall picks them up again. Since R11-12-5; older
installs have no uninstaller: install the newest release over them first. Installing a new release over an
existing one updates it in place (no uninstall needed).

How the installer is built: [`../../mscc-ui/installer/windows/README.md`](../../mscc-ui/installer/windows/README.md).
Old release notes (to R10.13.2): [`CHANGELOG.md`](CHANGELOG.md).
