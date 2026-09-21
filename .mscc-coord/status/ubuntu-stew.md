# Status — ubuntu-stew

| | |
|--|--|
| **Host** | stew-HP-Notebook |
| **Checkout** | `/home/stew/Documents/GitHub/mscc-station` |
| **Build** | cmd-032 |
| **Last command id** | cmd-032 |
| **State** | done |
| **Updated** | 2026-09-21 |

## ACK log

| command id | state | note |
|------------|-------|------|
| cmd-032 | done | mscc_1.0.44_amd64.deb with remote_mic stream reset; UI 0.6.59 kept |
| cmd-028 | done | Avalonia 0.6.59 FW band gate |
| cmd-027 | done | Avalonia idle + DIG-U + FW title |

## Notes

Deb: `installers/linux/mscc_1.0.44_amd64.deb` (also `linux/mscc-deb/`).
Live `$HOME/mscc/sdrcore-trans` rebuilt; strings has `stream reset (REMOTE path)`.
Stack was not running — no restart. UI left `mscc-ui_0.6.59_amd64.deb`. No Pi arm64 this host.
