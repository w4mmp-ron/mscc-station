# Windows — install

**This is not a Linux `.deb` kit.** Pi: [../rpi/](../rpi/). Ubuntu: [../linux/](../linux/).

## Package (this folder)

Run the newest **`mscc-net9-R*-install.exe`** (Advanced Installer). Typical deploy folder: **`C:\mscc-net9`**.

That installer is the **WPF** client plus Windows servers (`ms-sdr`, recv, trans).

## Use

- **Local radio on this PC:** start the Windows servers, then the WPF UI.
- **Remote to a Pi:** run only the WPF UI and Connect to the Pi’s IP, UDP port **8888** (Pi servers already running).

History / extra EXEs: [`../../mscc-ui/Release/windows-wpf/`](../../mscc-ui/Release/windows-wpf/).
