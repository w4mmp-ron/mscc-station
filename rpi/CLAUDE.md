# CLAUDE.md — sdrcore (Pi servers)

Claude's notes for the Pi servers `sdrcore-recv`, `sdrcore-trans` and `ms-sdr`
(Proficio / MSCC station). Owner: **Ron (W4MMP)**. Grok follows `AGENTS.md`; this
file is for Claude only.

## Working with Ron

- Short, plain-English answers. Say what is verified and what isn't.
- **"stop" means stop immediately.**
- Ask before large or risky changes. Commit / push **only when asked**,
  `git add` specific paths only (Grok and other tools work in this repo too).
- Ron does **not** `git pull` on the Pi. He copies the changed source files from
  Windows to the Pi and builds there. Give him the file list, not git steps.
- Ubuntu (`linux/`) work is Stew's; don't track or report it. After a pull, report
  only Stew's **Windows** changes and whether the Pi needs a matching port.

## Folder rules

| Path | Rule |
|---|---|
| `rpi/SDRcore-recv-linux/`, `rpi/SDRcore-trans-linux/`, `rpi/ms-sdr-linux/` | **Working trees** (Pi source of truth). Claude edits here. |
| Rest of `C:\Users\Ron\.grok\worktrees` | Read-only reference unless Ron says otherwise. |

History: Claude worked in a copy `rpi/sdrcore-claude/` (commits fe0c9a1, 0443c21,
9bb4be2, 4cb8b74). On 2026-09-25 Ron had the changes moved back into the originals
(originals unchanged since 8dee14a) and the copy deleted.
Git repo root: `C:\Users\Ron\.grok\worktrees`.

## Open list (Ron, 2026-09-26) - read first on resume

Done 2026-09-26: mscc 1.0.46 + mscc-init-gui 1.0.15 installed and tested on the Pi (Ron).
1. Test FREQ CAL STOP on the Pi (needs WPF 9.26.5+). Back burner (Ron 2026-09-27).
2. cmd-042: hi-cut 1400/1000 Hz in sdrcore-recv -> 1.0.48. Source done 2026-09-27 (Ron: do it now, not waiting for Ubuntu): udp_thread.c case 5/6, recv 3.141, WSL syntax OK. Ron built it on the Pi; sdrcore-recv copied to mscc-binaries. mscc 1.0.48 built 2026-09-27 (only sdrcore-recv differs from 1.0.47), in installers/rpi (1.0.47 removed). **Installed on the Pi 2026-09-27, filters work (Ron). Done.**
3. cmd-046: TUNE power separate (Windows/Ubuntu trans + WPF). With Stew; no Pi change.
4. RF check of remote TX audio (Stew, spectrum analyzer).
Done 2026-09-27 (committed): mscc-init dropped from the .deb (build-deb strips it, postinst removes old ~/mscc/mscc-init + link, docs point to mscc-init-gui). Source + mscc-binaries/mscc-init kept. Built into mscc 1.0.47 (2026-09-27, WSL; servers identical to 1.0.46), copied to installers/rpi (1.0.46 removed). Not installed on the Pi yet. cmd-042 now -> 1.0.48.

mscc-init CLI back as Python (Ron 2026-09-28): `mscc-init-gui/mscc_init_gui/cli.py` = port of
C `mscc-init-linux/sources/main.c` (same 4 steps/prompts), reuses config.py/devices.py (SWR keep
fix included; radio I/Q devices hidden like the GUI; warns if servers run). Shipped as
`/usr/bin/mscc-init` in mscc-init-gui 1.0.16 (built WSL, LF copy; in installers/rpi, 1.0.15
removed). mscc .deb unchanged (still drops the C binary). Tested on Windows Python with a fake
HOME (files OK); device pick needs PyAudio -> test on the Pi. Not committed.
Pi test 1.0.16: CLI works (Ron). GUI blank window = old bug: "servers running" askyesno ran
before the root was mapped and stayed hidden (traceback). 1.0.17: check runs via after(200)
once the window is up, dialogs parent=self. GUI works on the Pi (Ron).
Package renamed (Ron): `mscc-init` 1.0.18 -> `mscc-init_1.0.18_all.deb` (Provides/Conflicts/
Replaces mscc-init-gui; upgrade from 1.0.17 checked in WSL dpkg: old one removed). Commands and
folder rpi/mscc-init-gui/, /usr/share/mscc-init-gui unchanged. installers/rpi INSTALL.md +
install-mscc.sh updated; mscc-deb postinst/build-deb/install-mscc.sh hint text "(package
mscc-init)" edited in source only (goes in next mscc build). Stew's Ubuntu docs/drop script
still say mscc-init-gui_*.deb - briefed as cmd-050.

## Architecture (per Ron)

WPF client (Windows) <-> **ms-sdr** (controller, UDP :8888) <-> **sdrcore-recv** (:9000)
and **sdrcore-trans** (:9200). ms-sdr is the only interface between the client and the
two DSP cores. ms-sdr is also the **radio controller** (talks to the radio hardware
directly; CAT, USB, keyer code live there). Other ports in ms-sdr `port_defines.h`:
8889 MSCC, 9600 panadapter, 9700 spectrum, 9800 waterfall.
Remote audio: client mic -> sdrcore-trans UDP 9101 (MSA1); sdrcore-recv -> client
phones UDP 9100 (MSA1). CW is not part of remote audio.

## Build (on the Pi, arm64)

```bash
cd rpi/SDRcore-recv-linux  && make clean && make
cd ../SDRcore-trans-linux  && make clean && make
cd ../ms-sdr-linux         && make clean && make
```

`make` installs to `~/mscc/` (replaces the running binary; `BINDIR=...` to avoid).
See also `rpi/mscc-deb/BUILD-SERVERS-ON-PI.md`.
Syntax check on Windows: WSL `Debian` has gcc. Use `-iquote sources -idirafter sources`
(plain `-I sources` pulls in the Windows `pthread.h` and fails).

## Ron confirmed (not issues)

Overdrive code is legacy, ignore it. CW carrier is generated continuously (keyed
elsewhere). No thread locking is fine (works for years).
CW snap (ms-sdr `cw-snap.c`, recv `CMD_CW_SNAP_*`) is not in the client yet and never
runs during FREQ CAL (Ron 2026-09-27). Image check can't overlap FREQ CAL either (client
blocks it). So the `doRxCalibrate` static-index overrun (count change mid-fill) can't happen.
FREQ CAL saturation fix (2026-09-27; mscc 1.0.49 installed on the Pi, FREQ CAL works - Ron): Goertzel mags were stored as int(mag*1e6),
capped at INT_MAX (~-24 dBFS raw IQ) -> ties, first (low) step won, 3-sum wrapped. Now
`calMag*F` floats in `sdrcore.h`, `cal_data` floats, average in double (same 1e6 units vs
Calibration_Low_Limit). Image check keeps the scaled ints (`cal_mag_scaled`, clamped).
WSL syntax OK. Windows recv has the same issue (not touched).
#5 fixes (same batch, in 1.0.49): main.c freq_low init typo; cal log %ld -> %u/%d (udp_thread
217, 299, 755, 1395, 1404); cal_data index clamped to MAX_CALIBRATION_ELEMENT-1.
Future idea (Ron: keep in mind, not now): `goertzel_mag` uses only I (`data[i].real`) -> no
+600/-600 side check, ~3 dB SNR loss. Complex Goertzel (~10 lines) fixes both; level numbers
change, so recheck CALIBRATION_LOW_LIMIT / LOOSE. Mirror is 1200 Hz away, outside the sweep.

## Changes (2026-09-25)

sdrcore-trans:
1. `udp_thread.c` `CMD_SET_IQ_BAND`: unknown band ignored (was `G_iq_stack[200]` write).
2. `udp_thread.c` `CMD_SET_BAND_POWER_POWER`: `Get_Power_Mode_Index()` maps wire mode
   (0 AM,1 LSB,2 USB,3 CW,4 TUNE,5 FM) to table slot (USB,LSB,AM,CW,TUNE,FM).
3. `driver.c`: USB/LSB always use USB_POWER/LSB_POWER (Stew's 3ec263a had digital on
   TUNE_POWER). **Verified on air**: full power, follows SSB slider.
   Ron rule: TUNE power is TUNE only, fully separate. Windows/Ubuntu trans + WPF cmd-045
   item 8 break it -> brief `.mscc-coord/briefs/cmd-046.md` for Stew. Pi not changing.
4. `dsputils.c`: local DIGITAL_AUDIO (0) back on analog gain (6.324 stereo); remote
   modes 2/3 stay 2.5. Digital mic gain slider needs turning down.
5. `remote_mic.c`: smooth +/-300 ppm fill trim toward 100 ms (was 0.8 %/2.4 % pitch
   steps); prime/resync/re-prime. No file I/O in the audio callback; receiver thread
   logs EVENTs (hold_last reprime, resync, primed, overflow, udp_gap). **Remote audio
   verified on air** 2026-09-26: 30 m FT8 QSO, bad/under/overflow 0, no EVENTs,
   occ ~5020 (~105 ms), step 0.500020, peak ~14250 TX.
6. `main.c`: remote ring drained during TUNE (split streams); mic ring drift-safe (see 8).

sdrcore-recv:
7. `udp_thread.c`: `CMD_SET_IQ_BAND` bounds; phones/digital volume ATTN separate per
   audio mode; after CW TX reopen output for the current mode.
8. `main.c` (and trans mic ring): dual-stream ring primes to 64 ms, +/-300 ppm trim,
   resync, silence + re-prime on underrun (was ~21 ms dropout every few minutes).
9. `panadapter.c`: smoothing = true average of n frames in 32 bits, clamped 1..4
   (was n+1/n in uint16, overflow at 4). Pan levels shift; client dB CAL may need redoing.

Status: recv + ring fixes built on the Pi and working in Ron's first tests.

sdrcore-recv (2026-09-26):
13. `main.c` `manage_stream`: digital play stream (VirtualA) opens with >= 40 ms latency
    (`DIGI_PLAY_LATENCY`; was defaultLowOutputLatency). Cause: WSJT-X decode CPU bursts
    made the stream miss PipeWire cycles -> 128-sample silence gaps (clicks), `pw-top` ERR
    climbing only during decode (Ron confirmed). Phones stream unchanged. Log line
    "rate plan ... play_latency=N ms". First Pi test ran the OLD binary (not restarted;
    `mscc.sh start` skips running servers) - ignore it. 2026-09-26 after rebuild + restart:
    VirtualA play=48000 resample=1 dual=1 play_frames=1024 play_latency=40 ms (confirmed).
    Cold start (no client): phones 10 ms first, ms-sdr sends digital mode ~6 s later ->
    VirtualA 40 ms. With WSJT-X: ERR 0 -> 6 over several decodes (was climbing every
    decode). **Ron OK with this (tested OK 2026-09-26).** Renice not needed.
    Committed 658ae76.

ms-sdr (2026-09-26, cmd-044 port from Windows 93302b4):
14. `calibrate.c`: `Report_Calibration` always sends the per-run count (Cal_Reset removed);
    failed freq cal restores the previous mode (was `ModeChanged('A')`). Same hunks as
    Windows ms-sdr 3.174. Built on the Pi 2026-09-26 (see 15); done per Ron.
15. cmd-045a/b FREQ CAL STOP port (2026-09-26): `CMD_SET_CAL_ABORT 0x1F` (`usbavrcmd.h`,
    `main-controller.c` cal case list), `calibrate.c` = Windows 3.176 file (split auto/check
    run state, abort + 35 s drain, start refused while draining, GUI 0x1F 1=done 2=refused),
    `GetTickCount64` -> `Cal_Now_Ms()` (CLOCK_MONOTONIC). Pi CAL_RESET kept (`Create_PPM_ini`,
    not Windows' `Factory_reseed_live_file`). sdrcore-recv unchanged. Syntax-checked in WSL;
    Pi recv step wait ~30-33 s < 35 s drain. **Built on the Pi 2026-09-26 (with 14).
    Ron: treat as done; he will test later.** Not committed yet.

mscc-init-linux (2026-09-26):
10. `main.c` `init_mscc`: rewrites only its own keys, keeps other `mscc.ini` lines, and adds
    `SWR_METER=1`, `SWR_METER_PORT=6999`, `SWR_METER_TO_GUI=1` if missing (calibration had
    wiped them). Syntax-checked in WSL; not built/run on the Pi yet.
11. `mscc-init-gui/mscc_init_gui/config.py` `write_mscc_ini`: same keep + SWR defaults.
    Tested with Windows Python (existing and new file). Shared `_all.deb` with Ubuntu (Ron OK'd).
    File is `~/.local/mscc/mscc.ini` (not `~/mscc.ini`).
    **Verified 2026-09-26**: with `SWR_METER_TO_GUI=1` in `~/.local/mscc/mscc.ini`, SWR readings show in the client. 1.0.45 + init-gui 1.0.15 not yet installed/tested on the Pi.
12. `mscc-init-gui` 1.0.15: volume GUI dropped (Ron: didn't work out); postinst removes
    old `~/mscc/mscc-volume-gui`. Build with `build-deb.sh` in WSL from an LF copy (repo
    files are CRLF); the `.ps1` builder is stale. `mscc.sh` skips volume restore if absent.

## mscc 1.0.46 (2026-09-26)

Built in WSL from an LF copy of mscc-deb + mscc-binaries (Pi ELFs). Contents: ms-sdr (cal
STOP + cmd-044), sdrcore-recv (#13 40 ms digi), sdrcore-trans 09-25, mscc-init OLD
(09-16, no SWR keep fix; Ron: CLI not used, nothing runs it; source kept). postinst hint
now says run `mscc-init-gui`. Copied to installers/rpi (1.0.45 removed). Installed + tested OK (Ron).

## On hold (Ron, 2026-09-26): cmd-042 (from pull f9efa00, 2026-09-25)

Windows `mscc-recv` 3.141 added 0xD1 CMD_SET_BW_HICUT index 5 = 1400 Hz, 6 = 1000 Hz
(DIG-U Hi, WPF cmd-041). Pi to match in `SDRcore-recv-linux/sources/udp_thread.c`
(hi-cut switch ~line 1547) + version bump -> `mscc_1.0.48_arm64.deb` (1.0.47 used 2026-09-27). Brief:
`.mscc-coord/briefs/cmd-042.md`. Order: after ubuntu-stew is done and Stew pushes.
Reviewed: 0-4 unchanged, low-cut max 500 < 1000, ms-sdr passes index through. Not done yet.

## To do: RF check of remote TX audio (Stew, spectrum analyzer)

FT8 QSOs don't prove a clean signal (FT8 tolerates dropouts/pitch steps). Check:
- Single tone (FT8 or steady whistle): one clean line; sidebands/spurs = ring stepping or dropouts.
- Two-tone: IMD (3rd/5th) for overdrive; digital mic gain slider still high (see 4).
- TX on/off edges: no key-up/key-down splatter.
- Full 13 s FT8 over: no frequency jumps or wobble.
- Same test local vs remote audio; a difference points at the remote path.
If bad: `grep "remote_mic EVENT" ~/sdrcore-trans.log` for the same time.

## Closed: RX low-edge hash = WSJT-X display, not in the audio (2026-09-26)

Seen 2026-09-26 in local digital audio (WSJT-X Wide Graph): ~300 Hz band of hash just
above the RX low-cut. Moves with low-cut (500 -> hash at 500-800), same width. Present
on dummy load and in LSB. Not seen in remote: the two back-to-back USB audio adapters
(analog) add a noise floor that hides it. Very weak; phones/on-air likely unaffected.
Not from our changes (`sdrcore.c` untouched).

Earlier guess (overlap-save wrap from FFT-bin zeroing in `fastconv`) is **disproved**.
Simulation 2026-09-26 (real `sdrcore.c` + `wsfirgen.c` + `jimfft` + `doAGC`, white
noise, USB 500-3000, 96 kHz, 2048/4096): passband flat +/-0.1 dB from 600 Hz up, no
bump at 500-800, no block-rate ripple. `wsfirBP` = LP(fc2) - LP(fc1) (center deltas
cancel), clean linear phase. AGC (per-sample gain on noise) lifts the stopband below
low-cut from -90 to about -45 dB: smear *below* the edge, not above it.
NR and auto-notch were OFF when seen (Ron). Not yet tested: digital output path (ring /
resample to VirtualA), WSJT-X Wide Graph "Flatten" (can draw artifacts at steep edges).
Harness was in the session scratchpad (not kept); rebuild from these notes if needed.
Recorded `parec -d VirtualA.monitor` (48 kHz, 28 s, dummy load, USB, low-cut 500) while
the hash showed on the Wide Graph (Flatten off): 540-900 Hz flat within +/-0.5 dB and
noise-like over time, same as the rest of the passband. Edge falls ~40 dB from 540 to
420 Hz. So the hash is how WSJT-X draws the steep edge; nothing to fix in sdrcore.
Seen in the same recording: weak steady tones at 1000, 2000, 3000, 1359 Hz (3-8 dB
above noise in a 1.5 Hz bin), likely birdies. Note: RX digi audio is VirtualA
(VirtualB = TX).

## QRP power cal moved to sdrcore-trans (2026-09-28; built + tested on the Pi by Ron: 2 bands calibrated, power response very good. Committed 5e06637; in mscc 1.0.50)

trans owns `~/power_cal.ini`: `power.c` Create_power_cal_file (startup, factory table
47,30,24,29,53,72,28,30,45,40,40,40 = old ms-sdr PCB 2/4/5/6), bounds/NULL-safe
Init_Proficio_calibration (defaults then RECORD=n overrides), Update_power_cal_file
(tmp + rename). `udp_thread.c` 0xA1 unknown band -> index -1; 0xA2 -> Set_QRP_calibration
(all modes, write file, Drive_Manager reload). Get_Power_Mode_Index removed.
ms-sdr `power_calibration.c`: read-only file load on 0xA1 -> GUI 0xB4; 0xA2 kept as copy +
forwarded (no file write, no INITIALIZE, no 100 ms sleep); 0xB4 from copy; defaults (0xAA)
ignored (client never sends it); Create/Update/Delete + defaults tables removed; main.c no
longer creates the file. No protocol change. Client already forces TUNE power 100 on the
Pwr Cal tab, so no TUNE override in trans. Client never sends 0xAB master reset.
Windows ms-sdr/trans still old way (same file format). Stew briefed: cmd-048 (95601bb; Ubuntu + Windows port, Windows factory-seed choice goes to Ron).
Client flow (Ron 2026-09-28, for reference only - no change wanted): CALIBRATE -> 0xA2 0,
slider moves -> 0xA2 each (saved each time), CALIBRATE again -> "Accept?" YES (nothing sent),
NO (nothing sent; value stays saved), CANCEL (0xA2 previous value).
Slider speed-up (same day): 0xA2 now RAM only + G_drive_recalc (Drive_Manager recomputes);
power_cal.ini saved after ~500 idle Drive_Manager loops (G_power_cal_save_countdown), or
right away before any G_power_file_needs_updated reload. No power.ini/amplifier rewrite,
no Init_Power_All per step; 1 log line per step (was ~70 + 7 file ops).

## Amp cal slider like QRP (2026-09-28; built + tested on the Pi by Ron with amplifier.ini removal: QRP + QRO cal smoother. Committed 1b03de5; in mscc 1.0.50)
trans 0x08 (CMD_SET_POTENTIA_CALIBRATION, -99..0): RAM stack + table (1 + v/100) + G_drive_recalc;
amplifier_cal.ini saved after ~500 idle loops (G_amp_cal_save_countdown) or before any reload.
Flush_pending_cal_saves() before INITIALIZE's Init_Power_All. Update_amplifier_calibration:
RECORD/BAND = index (missing file made every line RECORD=0), tmp + rename, 1 log line.
Found (not changed): amplifier.ini "user power" is unused (client always sends 0xFA=100);
ms-sdr Delete_amplifier_ini_file has remove(homedir) bug; deb amplifier_cal.ini has
calibrated values (-37..-50), not 0.
amplifier.ini removed (same day, Ron): ms-sdr amplifier.c no longer creates/reads/writes it
(Check_Amplifier_Version, Create/Update/Delete/Initialize gone, incl. remove(homedir) bug);
0xF9 replies 0xFB = 100 (constant, what the file held after band select; client shows it on
PowerOut) + 0x08 cal from amplifier_cal.ini (read-only, bounds-checked). 0xFA ignored in
ms-sdr and trans (no INITIALIZE / full reload). trans: Init_amplifier_user_values +
amplifier_table.user_power_value removed. Old ~/amplifier.ini left on disk, harmless.
mscc 1.0.50 built 2026-09-28 (WSL, LF copy): ms-sdr + sdrcore-trans from the Pi (QRP/QRO cal
changes, amplifier.ini removed); sdrcore-recv same as 1.0.49. In installers/rpi (1.0.49 removed).
**Installed on the Pi 2026-09-28, works (Ron).** Deb still ships init-files/amplifier.ini (unused; Ron: keep for now).
