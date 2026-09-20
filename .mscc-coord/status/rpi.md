# Status — rpi

| | |
|--|--|
| **Host** | raspberrypi |
| **Checkout** | `/home/pi/src/mscc-station` |
| **Build** | Grok Build |
| **Last command id** | cmd-011 |
| **State** | done |
| **Updated** | 2026-09-20 |

## ACK log

| command id | state | note |
|------------|-------|------|
| cmd-011 | accepted | csproj + control Version 0.6.57 |
| cmd-011 | running | native `mscc-ui-arm64.sh` + deb + `drop-installers.sh rpi` |
| cmd-011 | done | `installers/rpi/mscc-ui_0.6.57_arm64.deb` Package mscc-ui Version 0.6.57 Architecture arm64. Installed on this Pi. |
| cmd-007 | done | 0.6.56 Pulse VAC |

## Notes

ELF aarch64. Replaced 0.6.56 in installers/rpi/. Title ATU/PTT smoke not run (Connect/radio identity optional). Commit kit + this status only.
