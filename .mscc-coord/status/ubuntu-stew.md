# Status — ubuntu-stew

| | |
|--|--|
| **Host** | stew-HP-Notebook |
| **Checkout** | `/home/stew/Documents/GitHub/mscc-station` |
| **Build** | cmd-043 |
| **Last command id** | cmd-043 |
| **State** | done |
| **Updated** | 2026-09-27 |

## ACK log

| command id | state | note |
|------------|-------|------|
| cmd-043 | done | mscc_1.0.47_amd64.deb; recv/trans 3.141; ms-sdr 3.171. Radio USB not attached so live smoke 1-8 not run. No rpi edits. No pull, no push. |
| cmd-042 | folded | Ubuntu part rides in cmd-043 (no 1.0.45 amd64). |
| cmd-040 | done | Avalonia 0.6.60 remote-audio parity |
| cmd-032 | done | mscc_1.0.44_amd64.deb with remote_mic stream reset |

## Notes

Deb: `installers/linux/mscc_1.0.47_amd64.deb` (also `linux/mscc-deb/`).
recv 3.141, trans 3.141, ms-sdr 3.171 (make bumped twice: build then pack).
x86-64 only. `rpi/` untouched.
apt install needs Stew sudo (no TTY for sudo here). `$HOME/mscc` already has the new binaries.
Smoke: radio Proficio USB not seen, `/dev/tnt1` missing. recv/trans exited "No Proficio/Multus I/Q". 1-8 not run (radio not available).
