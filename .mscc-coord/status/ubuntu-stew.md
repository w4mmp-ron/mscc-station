# Status — ubuntu-stew

| | |
|--|--|
| **Host** | stew-HP-Notebook |
| **Checkout** | `/home/stew/Documents/GitHub/mscc-station` |
| **Build** | cmd-051 |
| **Last command id** | cmd-051 |
| **State** | done |
| **Updated** | 2026-09-29 |

## ACK log

| command id | state | note |
|------------|-------|------|
| cmd-051 | done | Avalonia 0.6.68 Remote AF stop-before-restart. Commit 0d61d13. Smoke 1 Phones to Digital pass. Smoke 2-5 not run. Deb mscc-ui_0.6.68_amd64.deb. No rpi. No pull, no push. |
| cmd-049 | done | Avalonia 0.6.67 S-meter + ALC HOLD/Peak. Smoke 1-5, 7-9 pass. Smoke 6 ALC needles not run (boxes work). No rpi. No pull, no push. |
| cmd-047a | done | Avalonia 0.6.66 follow-up eb08e5e. Disconnect+MAIN mid-AUTO stays MAIN mode. Smoke 3, 5, and corner re-run pass. tty0tty leftovers unstaged. No rpi. No pull, no push. |
| cmd-047 | done | Avalonia 0.6.63 local smoke pass. Launch uses mscc-desktop-ctl. Drain about 30s. Hi-cut recv High: 1000.000000. DKMS tty0tty/1.4 installed. CAT works. E deferred (local only). tty0tty leftovers unstaged. No rpi. No pull, no push. |
| cmd-043 | done | mscc_1.0.47_amd64.deb; recv/trans 3.141; ms-sdr 3.171. Smoke 1 pass (servers running). 2-8 waiting on Stew. Commit 519c682. No pull, no push. CAT works after 1.0.47 install (see Notes). |
| cmd-042 | folded | Ubuntu part rides in cmd-043 (no 1.0.45 amd64). |
| cmd-040 | done | Avalonia 0.6.60 remote-audio parity |
| cmd-032 | done | mscc_1.0.44_amd64.deb with remote_mic stream reset |

## Notes

### cmd-051

Commit **0d61d13**. Deb: `installers/linux/mscc-ui_0.6.68_amd64.deb`. Client **0.6.68**.
Stop RemoteAf (RX+player+mic) before restart on Phones↔Digital. Do not call StopRemoteAf.
Smoke 1: Remote on Phones, switch to Digital, app survived. Pass (Stew: fixed, good job).
Smoke 2: Digital to Phones not run.
Smoke 3: start Digital then flip to Phones not run.
Smoke 4: play/mic device change not run.
Smoke 5: Mic TX drops=0 not run.

### cmd-049

Deb: `installers/linux/mscc-ui_0.6.67_amd64.deb`. Client **0.6.67**.
HOLD default on, Peak default off. Persist SMETER_HOLD / SMETER_PEAK / ALC_HOLD / ALC_PEAK in `~/.config/MSCC/mscc-avalonia.ini`.
Smoke 1: HOLD/Peak clickable, first start HOLD on Peak off. Pass.
Smoke 2: HOLD on slow fall. Pass.
Smoke 3: HOLD off instant, on slow again. Pass.
Smoke 4: Peak orange holds ~2 s then falls. Pass.
Smoke 5: Peak off orange gone. Pass.
Smoke 6: ALC needle tests not run (no TX today). ALC checkboxes work.
Smoke 7: independent mix sticks. Pass.
Smoke 8: restart mix + INI SMETER_HOLD=0 SMETER_PEAK=1 ALC_HOLD=1 ALC_PEAK=0. Pass.
Smoke 9: HOLD on Peak off both meters, INI all HOLD=1 PEAK=0. Pass.
