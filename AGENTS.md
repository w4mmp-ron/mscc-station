# Agent notes

## Installer drop (required)

When a release package is built or refreshed, **copy the newest file into `installers/<platform>/`**. Leave previous versions in the builder folder.

| Platform | Current kit (GitHub web) | History stays in |
|----------|--------------------------|------------------|
| Ubuntu x86_64 | `installers/linux/` | `linux/mscc-deb/`, `mscc-ui/Avalonia-Migration/`, `rpi/mscc-init-gui/` |
| Raspberry Pi | `installers/rpi/` | `rpi/mscc-deb/`, `rpi/mscc-init-gui/`, `rpi/mscc-portaudio/` |
| Windows | `installers/windows/` | `mscc-ui/Release/windows-wpf/` |

```bash
./linux-build/drop-installers.sh          # all three
./linux-build/drop-installers.sh linux
```

Do **not** copy `$HOME/mscc` x86 ELFs into `rpi/mscc-binaries/`. Ubuntu work is `linux/`; treat `rpi/` as a guide.
