# Status — ubuntu-stew

| | |
|--|--|
| **Host** | stew-HP-Notebook |
| **Checkout** | `/home/stew/Documents/GitHub/mscc-station` |
| **Build** | cmd-049 |
| **Last command id** | cmd-049 |
| **State** | done |
| **Updated** | 2026-09-28 |

## ACK log

| command id | state | note |
|------------|-------|------|
| cmd-049 | done | Avalonia 0.6.67 S-meter + ALC HOLD/Peak. Smoke 1-5, 7-9 pass. Smoke 6 ALC needles not run (boxes work). No rpi. No pull, no push. |
| cmd-047a | done | Avalonia 0.6.66 follow-up eb08e5e. Disconnect+MAIN mid-AUTO stays MAIN mode. Smoke 3, 5, and corner re-run pass. tty0tty leftovers unstaged. No rpi. No pull, no push. |
| cmd-047 | done | Avalonia 0.6.63 local smoke pass. Launch uses mscc-desktop-ctl. Drain about 30s. Hi-cut recv High: 1000.000000. DKMS tty0tty/1.4 installed. CAT works. E deferred (local only). tty0tty leftovers unstaged. No rpi. No pull, no push. |
| cmd-043 | done | mscc_1.0.47_amd64.deb; recv/trans 3.141; ms-sdr 3.171. Smoke 1 pass (servers running). 2-8 waiting on Stew. Commit 519c682. No pull, no push. CAT works after 1.0.47 install (see Notes). |
| cmd-042 | folded | Ubuntu part rides in cmd-043 (no 1.0.45 amd64). |
| cmd-040 | done | Avalonia 0.6.60 remote-audio parity |
| cmd-032 | done | mscc_1.0.44_amd64.deb with remote_mic stream reset |

## Notes

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

### cmd-047a

Deb: `installers/linux/mscc-ui_0.6.66_amd64.deb`. Client **0.6.66**. Commit **eb08e5e**.
Launch resolve: `/usr/local/bin/mscc-desktop-ctl`.
0.6.64 nits; 0.6.65 DIG-U overlay re-enter CW; 0.6.66 Disconnect keepOpen: MAIN mid-AUTO then Disconnect/Connect stays MAIN (not CW). Re-apply CW log only when CW is applied.
Smoke 1-11 (0.6.65): pass.
Re-run on 0.6.66: step 3 pass, step 5 pass, corner (FREQ CAL AUTO, MAIN mid-run, Disconnect, Connect: MAIN mode, not CW) pass.
Smoke 9 note: VFO A can show 0.000000 after X+Connect until the radio reports (cmd-027 idle). Not a 047a fail.
E: not this command (local only).
