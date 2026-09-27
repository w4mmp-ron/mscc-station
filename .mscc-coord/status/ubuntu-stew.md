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
| cmd-047 | pending | Avalonia 0.6.61: WPF parity (cmd-041 DIG-U Hi 1.4k/1.0k, cmd-044/045/045a/045b FREQ CAL + VFO B + QRP/AMP sync, cmd-046 power bank), local Launch (127.0.0.1) + Auto (any Host), tty0tty DKMS (Stew pastes one sudo line). Gate: HEAD = cmd-047 orders on 519c682. LOCAL smoke only; remote deferred. No rpi edits. No pull, no push. Orders on this host (briefs/cmd-047.md). |
| cmd-043 | done | mscc_1.0.47_amd64.deb; recv/trans 3.141; ms-sdr 3.171. Smoke 1 pass (servers running). 2-8 waiting on Stew. Commit 519c682. No pull, no push. |
| cmd-042 | folded | Ubuntu part rides in cmd-043 (no 1.0.45 amd64). |
| cmd-040 | done | Avalonia 0.6.60 remote-audio parity |
| cmd-032 | done | mscc_1.0.44_amd64.deb with remote_mic stream reset |

## Notes

Deb: `installers/linux/mscc_1.0.47_amd64.deb`.
Smoke 1: recv/trans/ms-sdr running after radio USB attached. Digital play_latency=40 ms. CAT /dev/tnt1 still missing (local WSJT CAT).
Smoke 2-8: not run yet.
CAT update 2026-09-27: mscc 1.0.47 installed; servers and local WSJT-X CAT (/dev/tnt0 + /dev/tnt1) now work.
