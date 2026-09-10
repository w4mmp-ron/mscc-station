# Current installers (grab these)

GitHub web: open this folder and pick **your computer**. Do **not** mix folders.

| Folder | Computer | Packages |
|--------|----------|----------|
| **[linux/](linux/)** | Ubuntu Desktop **x86_64** (amd64) | PortAudio, servers, init-gui, Avalonia UI |
| **[rpi/](rpi/)** | Raspberry Pi OS **64-bit** (arm64) | PortAudio, servers, init-gui, Avalonia UI |
| **[windows/](windows/)** | Windows | WPF `mscc-net9-R*-install.exe` |

Each folder has a short **INSTALL.md**.

## Rule (builders and AI)

When a kit is updated, **copy the newest file here** and leave older versions in the builder folder (for example `rpi/mscc-init-gui/`, `linux/mscc-deb/`, `mscc-ui/Release/windows-wpf/`).

```bash
./linux-build/drop-installers.sh          # all three
./linux-build/drop-installers.sh linux    # after an Ubuntu .deb
```

Scripts that already drop here after a successful build:

- `linux-build/build-mscc-deb-amd64.sh` → `installers/linux/`
- `linux-build/build-mscc-ui-deb-amd64.sh` → `installers/linux/`
- `rpi/mscc-deb/build-deb.sh` → `installers/rpi/`

Never copy `$HOME/mscc` x86 binaries into `rpi/mscc-binaries/`.
