# Status — ubuntu-stew

| | |
|--|--|
| **Host** | stew-HP-Notebook |
| **Checkout** | `/home/stew/Documents/GitHub/mscc-station` |
| **Build** | cmd-013 |
| **Last command id** | cmd-013 |
| **State** | done |
| **Updated** | 2026-09-20 |

## ACK log

| command id | state | note |
|------------|-------|------|
| cmd-013 | done | linux/ms-sdr owner re-ack resends packed 0xB2+0xB3+status. Built `$HOME/mscc/ms-sdr` (Core 3.167). |
| cmd-010 | done | Avalonia 0.6.57 ATU/PTT + amd64 kit |

## Notes

- Mirrored Pi cmd-012 block in `linux/ms-sdr-linux/source/main-controller.c`. No Session_Claim on re-ack.
- Binary in `$HOME/mscc/ms-sdr` (not git). Servers were not running at rebuild (no radio).
- Stew: `mscc start`, Connect, Disconnect UI only, Connect again — title should keep FW + ATU/PTT; log `owner re-ack FW packed`.
- Stew pushes.

## Blocked

(none)
