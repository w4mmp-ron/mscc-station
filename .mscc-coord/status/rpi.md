# Status — rpi

| | |
|--|--|
| **Host** | raspberrypi |
| **Checkout** | `/home/pi/src/mscc-station` |
| **Build** | Grok Build |
| **Last command id** | cmd-004 |
| **State** | done |
| **Updated** | 2026-09-17 |

## ACK log

| command id | state | note |
|------------|-------|------|
| cmd-004 | accepted | Yaml target is ubuntu-stew. This session is **rpi** (aarch64). User asked to run cmd-004 here. Native arm64 publish, not Ubuntu cross-build. Did **not** write `status/ubuntu-stew.md`. |
| cmd-004 | running | .NET 9 SDK 9.0.318 installed; `mscc-ui-arm64.sh` + `build-mscc-ui-deb-arm64.sh` |
| cmd-004 | done | `mscc-ui_0.6.54_arm64.deb` — Package mscc-ui, Version 0.6.54, Architecture arm64. ELF aarch64. Dropped `installers/rpi/`. |

## Notes

- Deb: `installers/rpi/mscc-ui_0.6.54_arm64.deb` (also `mscc-ui/Release/avalonia/arm64/` and Avalonia-Migration/).
- Binary: `publish/linux-arm64-sc/MSCC.Avalonia` ARM aarch64.
- Did not touch amd64 kit or `rpi/` server trees in this commit.
- Pi install: `sudo apt install ./installers/rpi/mscc-ui_0.6.54_arm64.deb`

cmd-002 still listed by overseer as pending on this host; not run in this turn.
