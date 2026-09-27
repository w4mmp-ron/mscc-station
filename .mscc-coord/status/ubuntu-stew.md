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
| cmd-047a | pending | Avalonia 0.6.64: 10m button width, remove stray Band label over USER, FREQ CAL reconnect / VFO B / deferred-restore fixes, minor Launch/log/tooltip fixes, restore cmd-032 and cmd-043 history. Gate: HEAD = cmd-047a orders on 8999f9a. LOCAL smoke only. No rpi edits. No pull, no push. Orders in briefs/cmd-047a.md. |
| cmd-047 | done | Avalonia 0.6.63 local smoke pass. Launch uses mscc-desktop-ctl. Drain about 30s. Hi-cut recv High: 1000.000000. DKMS tty0tty/1.4 installed. CAT works. E deferred (local only). tty0tty leftovers unstaged. No rpi. No pull, no push. |
| cmd-043 | done | mscc_1.0.47_amd64.deb; recv/trans 3.141; ms-sdr 3.171. Commit 519c682. |
| cmd-042 | folded | Ubuntu part rides in cmd-043. |
| cmd-040 | done | Avalonia 0.6.60 remote-audio parity |

## Notes

Deb: `installers/linux/mscc-ui_0.6.63_amd64.deb`. Client **0.6.63**.
Launch resolve: `/usr/local/bin/mscc-desktop-ctl`.
Close-X Launch stop: fixed in 0.6.62 (async close); 0.6.63 FREQ CAL tabs stay clickable with popup.
FREQ CAL drain: STOPPED then AUTO/CHECK back in about 30s.
recv hi-cut: `High: 1400.000000` then `High: 1000.000000`.
DKMS: `tty0tty/1.4, 7.0.0-34-generic, x86_64: installed (Original modules exist)`. `/dev/tnt0` `/dev/tnt1` present. CAT works with WSJT-X.
VFO A always starts active. DIG-U 1.0k does not come back after USB (shared Hi).
Smoke 1-23 local: pass (Launch/Auto, FREQ CAL tab CW/STOP/popup, VFO B, QRP/AMP, no radio-model button, SSB vs Tune, DIG-U Hi, tooltips, 0.6.63, DKMS, CAT).
E deferred (local only): Auto with remote Host; remote Host/Port; Remote Digital; Pi kit; DKMS after a real kernel update.
