# Status — rpi

| | |
|--|--|
| **Host** | raspberrypi |
| **Checkout** | `/home/pi/src/mscc-station` |
| **Build** | Grok Build |
| **Last command id** | cmd-032 |
| **State** | idle (await cmd-034 ACK) |
| **Updated** | 2026-09-23 |

## ACK log

| command id | state | note |
|------------|-------|------|
| cmd-032 | done | Debs in `installers/rpi/` and `rpi/Rpi-installers/` |
| cmd-031 | done | trans 17:56 stream reset |

## Notes

- Next: **cmd-034** in COMMANDS.yaml (clear-on-TX + fixed 2:1 fill). ACK when Build starts.
- Live `$HOME/mscc/sdrcore-trans` still has cmd-031 reset string; dpkg may show 1.0.43 while 1.0.44 kit exists.
