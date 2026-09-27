# Status — ubuntu-stew

| | |
|--|--|
| **Host** | stew-HP-Notebook |
| **Checkout** | `/home/stew/Documents/GitHub/mscc-station` |
| **Build** | cmd-047 |
| **Last command id** | cmd-047 |
| **State** | done |
| **Updated** | 2026-09-27 |

## ACK log

| command id | state | note |
|------------|-------|------|
| cmd-047 | done | Avalonia 0.6.61 kit built. Launch uses mscc-desktop-ctl. Drain time n/a (FREQ CAL smoke not run). Hi-cut recv line n/a. DKMS not registered yet (Stew pastes sudo). Smoke 1-25 after apt install. E deferred (local only). tty0tty leftovers unstaged. No rpi. No pull, no push. |
| cmd-043 | done | mscc_1.0.47_amd64.deb; recv/trans 3.141; ms-sdr 3.171. Commit 519c682. |
| cmd-042 | folded | Ubuntu part rides in cmd-043. |
| cmd-040 | done | Avalonia 0.6.60 remote-audio parity |

## Notes

Deb: `installers/linux/mscc-ui_0.6.61_amd64.deb`. Client 0.6.61.
Launch resolve: `/usr/local/bin/mscc-desktop-ctl`.
DKMS: headers present, Secure Boot off, `dkms` package not installed; `/usr/share/mscc/tty0tty/module/dkms.conf` present. CAT `/dev/tnt0` and `/dev/tnt1` already work.
Smoke: apt install needs Stew password. 1-22 not run in this session. 23-24 DKMS line given to Stew. 25 CAT already works. E: Auto remote, remote Host/Port, Remote Digital, Pi kit, kernel-update DKMS rebuild = deferred (local only).
VFO A always starts active (WPF rule). DIG-U 1.0k does not come back after leaving DIG-U (shared Hi).
