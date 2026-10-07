# Windows — install

**This is not a Linux `.deb` kit.** Pi: [../rpi/](../rpi/). Ubuntu: [../linux/](../linux/).

## Package (this folder)

Run the newest **`mscc-net9-R*-install.exe`** (Advanced Installer) — currently **`mscc-net9-R10-4-0-install.exe`** (title bar 10.4.0). Older installers here are for roll-back only. Typical deploy folder: **`C:\mscc-net9`**.

That installer is the **WPF** client plus Windows servers (`ms-sdr`, recv, trans).

Factory calibration tables are installed next to the servers (`C:\mscc-net9\factory\`). After calibrating, press **Save settings** (right panel, under LOG) to keep the calibration for that radio on the computer running the servers.

What changed: [`CHANGELOG.md`](CHANGELOG.md). Problem reports: https://multussdr.groups.io/g/main/topics

## Use

- **Local radio on this PC:** start the Windows servers, then the WPF UI.
- **Remote to a Pi:** run only the WPF UI and Connect to the Pi’s IP, UDP port **8888** (Pi servers already running).

History / extra EXEs: [`../../mscc-ui/Release/windows-wpf/`](../../mscc-ui/Release/windows-wpf/).
