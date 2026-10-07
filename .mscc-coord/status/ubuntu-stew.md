# Status — ubuntu-stew

| | |
|--|--|
| **Host** | stew-HP-Notebook |
| **Checkout** | `/home/stew/Documents/GitHub/mscc-station` |
| **Build** | cmd-065 |
| **Last command id** | cmd-065 |
| **State** | done |
| **Updated** | 2026-10-07 |

## ACK log

| command id | state | note |
|------------|-------|------|
| cmd-065 | done | Ubuntu mscc-ui 0.6.72 dock icon + mscc 1.0.50 (recv 3.143, ms-sdr 3.176). Commit 9814897. Stew: installed, dock icon works. No rpi. No Solidus. No pull, no push. |
| cmd-064 | running | Ubuntu linux/SDRcore-recv-linux spectrum spur fix. Commit ef67816. recv 3.143 in $HOME/mscc. Smoke 1-5 not run (no radio USB). Rides with next kit. No rpi write. No Windows. No pull, no push. |
| cmd-063 | done | Optional mscc-firmware 1.0.0 all. Commit 54054c1. Load File default /usr/share/mscc/firmware. Eight radios installed. Load File opened at share. Remove-package warning not run. Skipped mscc Suggests. No Keil. No Avalonia/WPF/Core. No Solidus. No pull, no push. |
| cmd-061 | done | Linux host park/restore + Avalonia Save settings 0x29. Commit 1327cd0. mscc 1.0.49, ui 0.6.71, ms-sdr 3.173. Smoke 1 Save settings park pass. Smoke 2 live unchanged pass. Smoke 3 detect/swap not run (one radio). Smoke 4 same-line restart keep-live pass. No rpi server source. No WPF. No pull, no push. |
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

### cmd-065

Commit **9814897**. Debs: `installers/linux/mscc_1.0.50_amd64.deb`, `installers/linux/mscc-ui_0.6.72_amd64.deb`. recv 3.143, ms-sdr 3.176. Dock StartupWMClass and WmClass mscc-ui. Stew: installed and dock icon works. No rpi. No Solidus.

### cmd-064

Commit **ef67816**. Built locally into $HOME/mscc/sdrcore-recv. VERSION_MINOR 143. panadapter.c copied from Pi (pixel notch gone, TX-monitor blanking kept). dsputils.c DC_BLOCK_A 0.98 + pan_hist[4096]. Rides with next Ubuntu kit. No rpi write. No Windows.
Smoke 1-5 not run: no radio USB (16c0:05dc missing). Level numbers not measured.

### cmd-063

Commit **54054c1**. Debs: `installers/linux/mscc-firmware_1.0.0_all.deb`, `installers/rpi/mscc-firmware_1.0.0_all.deb`. Install path `/usr/share/mscc/firmware/<RadioName>/`. Eight radios, 16 cyacd + 16 hex. Load File patched in `linux/psoc-usb-bootload-linux/bootloader-gui.py`, `linux/helpers/bootloader-gui`, `rpi/psoc-usb-bootload-linux/bootloader-gui.py`. Skipped mscc Suggests so servers 1.0.49 was not rebuilt. Desktop Firmware Upload still uses the 1.0.49 helper until the next mscc kit.
Smoke 1: dpkg mscc-firmware 1.0.0, eight radio dirs. Pass.
Smoke 2: Load File opened at `/usr/share/mscc/firmware`. Pass. Did not Program.
Smoke 3: remove-package warning path not run.

### cmd-061

Commit **1327cd0**. Debs: `installers/linux/mscc_1.0.49_amd64.deb`, `installers/linux/mscc-ui_0.6.71_amd64.deb`. ms-sdr VERSION_MINOR 173. Firmware 3.232 line proficio-mkii.
Park: `~/.local/mscc/cal/proficio-mkii/` iq.ini, power_cal.ini, amplifier_cal.ini. LAST_LINE.txt = proficio-mkii. freq_cal.ini not parked.
Smoke 1: Save settings 0x29 Factory_park files=3. Pass.
Smoke 2: live iq/power/amp/freq bytes unchanged vs pre-start backup. Pass.
Smoke 3: detect/swap not run. Only one radio on USB. First detect did bootstrap live to park for proficio-mkii.
Smoke 4: same-line restart logged cal keep live for all three plus freq; live bytes unchanged. Pass.

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
