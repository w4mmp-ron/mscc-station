# Status — rpi

| | |
|--|--|
| **Host** | raspberrypi |
| **Checkout** | `/home/pi/src/mscc-station` |
| **Build** | Grok Build |
| **Last command id** | cmd-032 |
| **State** | done |
| **Updated** | 2026-09-21 |

## ACK log

| command id | state | note |
|------------|-------|------|
| cmd-032 | accepted | Ship mscc 1.0.44 arm64; UI stays 0.6.59 |
| cmd-032 | running | Live trans already has reset; bump 1.0.44; build-deb.sh |
| cmd-032 | done | Debs in `installers/rpi/` and `rpi/Rpi-installers/` |
| cmd-031 | done | trans 17:56 stream reset |

## Notes

- `strings $HOME/mscc/sdrcore-trans` → `remote_mic: stream reset (REMOTE path)` (no rebuild needed).
- Packaged trans in 1.0.44 also has that string.
- `installers/rpi/mscc_1.0.44_arm64.deb` + `mscc-ui_0.6.59_arm64.deb`
- `rpi/Rpi-installers/mscc_1.0.44_arm64.deb` + `mscc-ui_0.6.59_arm64.deb` (refreshed from 0.6.44)
- No Avalonia rebuild. Not pushed.
