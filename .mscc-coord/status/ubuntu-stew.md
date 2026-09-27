# Status — ubuntu-stew

| | |
|--|--|
| **Host** | stew-HP-Notebook |
| **Checkout** | `/home/stew/Documents/GitHub/mscc-station` |
| **Build** | cmd-047a |
| **Last command id** | cmd-047a |
| **State** | done |
| **Updated** | 2026-09-27 |

## ACK log

| command id | state | note |
|------------|-------|------|
| cmd-047a | done | Avalonia 0.6.66 follow-up. Disconnect+MAIN mid-AUTO stays MAIN mode. Smoke 3, 5, and corner re-run pass. tty0tty leftovers unstaged. No rpi. No pull, no push. |
| cmd-047 | done | Avalonia 0.6.63 local smoke pass. Launch uses mscc-desktop-ctl. Drain about 30s. Hi-cut recv High: 1000.000000. DKMS tty0tty/1.4 installed. CAT works. E deferred (local only). tty0tty leftovers unstaged. No rpi. No pull, no push. |
| cmd-043 | done | mscc_1.0.47_amd64.deb; recv/trans 3.141; ms-sdr 3.171. Smoke 1 pass (servers running). 2-8 waiting on Stew. Commit 519c682. No pull, no push. CAT works after 1.0.47 install (see Notes). |
| cmd-042 | folded | Ubuntu part rides in cmd-043 (no 1.0.45 amd64). |
| cmd-040 | done | Avalonia 0.6.60 remote-audio parity |
| cmd-032 | done | mscc_1.0.44_amd64.deb with remote_mic stream reset |

## Notes

### cmd-047a

Deb: `installers/linux/mscc-ui_0.6.66_amd64.deb`. Client **0.6.66**.
Launch resolve: `/usr/local/bin/mscc-desktop-ctl`.
0.6.64 nits; 0.6.65 DIG-U overlay re-enter CW; 0.6.66 Disconnect keepOpen: MAIN mid-AUTO then Disconnect/Connect stays MAIN (not CW). Re-apply CW log only when CW is applied.
Smoke 1-11 (0.6.65): pass.
Re-run on 0.6.66: step 3 pass, step 5 pass, corner (FREQ CAL AUTO, MAIN mid-run, Disconnect, Connect: MAIN mode, not CW) pass.
Smoke 9 note: VFO A can show 0.000000 after X+Connect until the radio reports (cmd-027 idle). Not a 047a fail.
E: not this command (local only).

### cmd-047

Deb: `installers/linux/mscc-ui_0.6.63_amd64.deb`. Client **0.6.63**.
Launch resolve: `/usr/local/bin/mscc-desktop-ctl`.
Close-X Launch stop: fixed in 0.6.62 (async close); 0.6.63 FREQ CAL tabs stay clickable with popup.
FREQ CAL drain: STOPPED then AUTO/CHECK back in about 30s.
recv hi-cut: `High: 1400.000000` then `High: 1000.000000`.
DKMS: `tty0tty/1.4, 7.0.0-34-generic, x86_64: installed (Original modules exist)`. `/dev/tnt0` `/dev/tnt1` present. CAT works with WSJT-X.
VFO A always starts active. DIG-U 1.0k does not come back after USB (shared Hi).
Smoke 1-23 local: pass (Launch/Auto, FREQ CAL tab CW/STOP/popup, VFO B, QRP/AMP, no radio-model button, SSB vs Tune, DIG-U Hi, tooltips, 0.6.63, DKMS, CAT).
E deferred (local only): Auto with remote Host; remote Host/Port; Remote Digital; Pi kit; DKMS after a real kernel update.

### cmd-043 (history)

Deb: `installers/linux/mscc_1.0.47_amd64.deb` (also `linux/mscc-deb/`).
recv 3.141, trans 3.141, ms-sdr 3.171 (make bumped twice: build then pack).
x86-64 only. `rpi/` untouched.
apt install needs Stew sudo (no TTY for sudo here). `$HOME/mscc` already has the new binaries.
Smoke (519c682): radio Proficio USB not seen, `/dev/tnt1` missing. recv/trans exited "No Proficio/Multus I/Q". 1-8 not run (radio not available).
Smoke 1 (231d521): recv/trans/ms-sdr running after radio USB attached. Digital play_latency=40 ms. CAT `/dev/tnt1` still missing (local WSJT CAT).
Smoke 2-8: not run yet (at that ACK).
CAT update 2026-09-27: mscc 1.0.47 installed; servers and local WSJT-X CAT (`/dev/tnt0` + `/dev/tnt1`) now work.
