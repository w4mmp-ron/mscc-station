# Status — ubuntu-stew

| | |
|--|--|
| **Host** | stew-HP-Notebook |
| **Checkout** | `/home/stew/Documents/GitHub/mscc-station` |
| **Build** | cmd-061 |
| **Last command id** | cmd-061 |
| **State** | running |
| **Updated** | 2026-10-02 |

## ACK log

| command id | state | note |
|------------|-------|------|
| cmd-061 | running | Linux host park/restore + Avalonia Save settings 0x29. mscc 1.0.49, ui 0.6.71. No rpi server source. No WPF. No pull, no push. |
| cmd-060 | done | factory/ in mscc 1.0.48 amd64 and 1.0.51 arm64. Commit 0f92239. Smoke 1-4 pass. Live marker and iq.ini untouched. No postinst seed change. No pull, no push. |
| cmd-058 | running | Avalonia 0.6.70 SPECTRUM RESOLUTION 800/1600/3200. HEAD b6f4505. Leave cmd-053 linux dirty unstaged. No Core. No rpi. No pull, no push. |
| cmd-053 | running | Ubuntu recv panadapter -12 kHz notch width + fill. Copy Pi panadapter.c, VERSION_MINOR 142. No rpi write. No pull, no push. |
| cmd-052 | running | Avalonia 0.6.69 StartRemoteAf stop-before-open. HEAD 825781a. REMOTE smoke. No rpi. No pull, no push. |
| cmd-051 | done | Avalonia 0.6.68 Remote AF stop-before-restart. Commit 0d61d13. Smoke 1 Phones to Digital pass. Smoke 2-5 not run. Deb mscc-ui_0.6.68_amd64.deb. No rpi. No pull, no push. |
| cmd-049 | done | Avalonia 0.6.67 S-meter + ALC HOLD/Peak. Smoke 1-5, 7-9 pass. Smoke 6 ALC needles not run (boxes work). No rpi. No pull, no push. |
| cmd-047a | done | Avalonia 0.6.66 follow-up eb08e5e. Disconnect+MAIN mid-AUTO stays MAIN mode. Smoke 3, 5, and corner re-run pass. tty0tty leftovers unstaged. No rpi. No pull, no push. |
| cmd-047 | done | Avalonia 0.6.63 local smoke pass. Launch uses mscc-desktop-ctl. Drain about 30s. Hi-cut recv High: 1000.000000. DKMS tty0tty/1.4 installed. CAT works. E deferred (local only). tty0tty leftovers unstaged. No rpi. No pull, no push. |
| cmd-043 | done | mscc_1.0.47_amd64.deb; recv/trans 3.141; ms-sdr 3.171. Smoke 1 pass (servers running). 2-8 waiting on Stew. Commit 519c682. No pull, no push. CAT works after 1.0.47 install (see Notes). |
| cmd-042 | folded | Ubuntu part rides in cmd-043 (no 1.0.45 amd64). |
| cmd-040 | done | Avalonia 0.6.60 remote-audio parity |
| cmd-032 | done | mscc_1.0.44_amd64.deb with remote_mic stream reset |

## Notes

### cmd-061

Built locally. Debs: `installers/linux/mscc_1.0.49_amd64.deb`, `installers/linux/mscc-ui_0.6.71_amd64.deb`. ms-sdr VERSION_MINOR 173. Opcode 0x29 park. Avalonia Save settings under LOG. No rpi server source. No WPF. No Solidus.
Smoke 1-4 not run: no radio USB on lsusb (no 16c0:05dc). Detect/swap needs a second radio. Live `~/.local/mscc/cal/` does not exist yet. Waiting on Stew to install then Connect + Save settings.

### cmd-060

Commit **0f92239**. Debs: `installers/linux/mscc_1.0.48_amd64.deb`, `installers/rpi/mscc_1.0.51_arm64.deb`.
Ships `/usr/share/mscc/factory/{iq,freq,power}/<line>/` for six lines. Files-only. postinst still seeds flat init-files only.
Smoke 1: six iq.ini, six freq_cal.ini, six power_cal.ini installed. Pass.
Smoke 2: live marker OK, iq.ini identical to pre-install backup. Pass.
Smoke 3: /usr/share/mscc/init-files/ still present. Pass.
Smoke 4: Pi deb listing same 18 factory inis. Pass. Not installed on this laptop.
init-files = first-boot seed. factory/ = per-line templates for phase 2+. No reboot needed for this cmd.

### cmd-058

Accepted then running. S/W SPECTRUM RESOLUTION Normal/High/Max, sticky PAN_RESOLUTION, ApplyPanResolution on Connect and S/W change. Bump 0.6.70. Do not edit Core. Leave cmd-053 linux files unstaged.

### cmd-053

Accepted then running. Copy `rpi/SDRcore-recv-linux/sources/panadapter.c` onto `linux/` as-is (b08af65 + ef1d612). Bump recv VERSION_MINOR 141 to 142. make clean && make. No deb.

### cmd-052

Accepted then running. StartRemoteAf Stop before ApplyRemoteAfDevices / StartRx / StartMic. Do not call StopRemoteAf. Leave ApplyRemoteAfDevicesAndRestart alone. Bump 0.6.69.

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
