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

## MSCC coord (Grok Build)

Overseer bot **Build Commander** drives multi-host work via `.mscc-coord/`.

1. Work only in this repo checkout (`handoff.md` paths).
2. On start: `git pull`, read `.mscc-coord/OVERSEER.md` + `COMMANDS.yaml`.
3. If a command’s `target` includes this host (or `all`) and you have not finished that `id`:
   - Update only `.mscc-coord/status/<your-host-id>.md` → `accepted` → `running` → `done` or `blocked`.
   - Do not edit `COMMANDS.yaml`, `OVERSEER.md`, or another host’s status file.
4. Host ids: `windows-new-hp`, `windows-shack`, `ubuntu-stew`, `rpi`.
5. Norman prefers **one Build at a time**.
6. Full protocol: `.mscc-coord/README.md`.
