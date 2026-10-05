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

## Open list (Ron, updated 2026-09-29) - read first on resume

1. Test FREQ CAL STOP on the Pi (needs WPF 9.26.5+). Back burner (Ron 2026-09-27).
2. cmd-046: TUNE power separate (Windows/Ubuntu trans + WPF). With Stew; no Pi change.
3. RF check of remote TX audio (Stew, spectrum analyzer).
4. cmd-048 (Stew): port Pi QRP/QRO power cal to Ubuntu/Windows; Windows factory-seed choice -> Ron.
5. cmd-050 (Stew): Ubuntu kit/docs/drop-installers.sh for the mscc-init rename (below).
6. Done 2026-09-29: mscc-init 1.0.18 installed on the Pi, works (Ron).
8. Spectrum -12 kHz DC-spur notch sized in FFT bins (2026-09-29, `SDRcore-recv-linux/sources/panadapter.c`,
   was fixed 2 px, spur showed at PAN RESOLUTION 1600/3200). MP_HALF_BINS 3 tested on the Pi: better, a
   trace remained -> 4: still a ~5 dB skirt bump + faint waterfall line (screenshot) -> 6 (2026-09-29):
   **tested on the Pi, OK (Ron; "a bit blotchy but OK").** Committed b08af65. Stew brief cmd-053 (Windows + Ubuntu recv) pushed fe5cabb. PAN -> SPECTRUM label rename: cmd-054 (WPF only, label text).
   Blotchy fix (2026-09-29): 4 px avg each side tested = better, not enough -> A+B: fill level smoothed over frames (MP_LEVEL_ALPHA 0.2) + noise texture mirrored from neighbours; tested on the Pi 2026-09-29, Ron OK. Committed ef1d612; cmd-053 brief already describes this fill (checked 2026-09-30).
   Stew status (2026-09-30): Windows recv 3.142 done with the Pi fill (live spectrum smoke not run, not pushed); Ubuntu `linux/` running.
9. Stew's `.mscc-coord/QUESTIONS-FOR-RON.md` answered (ce731ee, 522ea21). Ron's position (#4): trans
   owns power_cal.ini. #6 (2026-09-30): Ron said YES to detection-driven auto-restore for
   cmd-055 (Stew swaps Ultimus/Geminus often) instead of the manual switch-radio tool; trans
   still owns power_cal.ini, ms-sdr must not overwrite it. #5 second-STOP: Stew pushed
   cmd-056 (ms-sdr 3.177); ported 2026-09-30 to rpi/ms-sdr-linux calibrate.c (abort while
   cal_abort_pending = log only, drain kept). **Built on the Pi, works (Ron 2026-09-30).** Committed 8889d77.
10. mscc-portaudio 19.8.3 (2026-10-01): `apt full-upgrade` showed "ldconfig: /usr/local/lib/libportaudio.so.2
   is not a symbolic link". 19.8.2 shipped 3 full copies (links lost via Windows). `build-deb.sh` now ships
   `libportaudio.so.19.8` + makes the `.so.2` / `.so` links; same binary (sha c4f6c2b9...). Build needs
   `PORTAUDIO_ROOT=<worktrees>/portaudio` (default `rpi/portaudio` doesn't exist). In installers/rpi
   (19.8.2 removed). **Installed on the Pi 2026-10-01, `sudo ldconfig` silent (Ron).** Committed 316d005, pushed.
11. **Per-radio cal (park / restore) ported to the Pi 2026-10-03, Ron: "just make it work".**
   Stew built it his way on Windows (cmd-055/059/062, ms-sdr 3.181, WPF 10.2.0) and Ubuntu (cmd-061):
   ms-sdr copies the files; client "Save settings" sends `0x29 CMD_SET_PARK_CAL_SETTINGS` to the host.
   Pi port = Ubuntu `factory_seed.c/.h` copied into `rpi/ms-sdr-linux/source/` + the same hooks
   (Makefile, `usbavrcmd.h` 0x29, `main-controller.c` case, `main.c` seed at version read + reload
   when the network is up). One Pi difference, in `factory_seed.c`: Save settings first sends
   trans `CMD_SET_SDRCORE_TRANS_INITIALIZE` + 300 ms wait, so trans writes any pending slider
   value before the copy.
   **`CMD_SET_IQ_DEFAULTS` now works as on Windows (Ron 2026-10-03: "exactly like Windows"):**
   trans `udp_thread.c` no longer deletes iq.ini on 0x8D, it re-reads the live file (built-in
   only if missing); ms-sdr `iq.c` TX Reset All copies `factory/iq/<line>/iq.ini` over the live
   file first; the reload after a radio swap sends 0x8D. So **ms-sdr and sdrcore-trans must be
   updated together** (new ms-sdr + old trans = swapped-in iq.ini deleted), and TX Reset All
   needs `/usr/share/mscc/factory` (mscc 1.0.51+ package); without it Reset All does nothing.
   **Rest of the Windows state ported too (Ron 2026-10-03: "same state as Windows"):** ms-sdr
   `calibrate.c` CAL_RESET loads `factory/freq/<line>/freq_cal.ini` (fallback `Create_PPM_ini`);
   trans no longer creates power_cal.ini when missing (`Create_power_cal_file` removed from
   `main.c` / `power.c` / `extern.h`; built-in table in RAM until ms-sdr seeds the file and
   sends INITIALIZE). sdrcore-recv unchanged.
   **Pi-only safety additions in `factory_seed.c`** (Ron: "make sure it does not trash anything"):
   `copy_file` writes `<dst>.tmp` then renames (no half-written file); when a live file replaces a
   different parked file (radio-swap stash or Save settings) the old parked file is kept as
   `<leaf>.bak`. Windows / Ubuntu have neither.
   Trash audit, run in a WSL harness with the real `factory_seed.c` (first start after upgrade,
   same radio, Save, swap to a new radio, swap back, resets, LAST_LINE.txt lost, no factory tree):
   user cal survived every case except one in Stew's design: **if `cal/LAST_LINE.txt` is lost
   while the live files belong to a different radio than the one connected, the live files are
   taken as the connected radio's, and at the next radio swap they overwrite that radio's parked
   copy.** On the Pi the `.bak` keeps the good copy; on Windows/Ubuntu it is gone. Not told to
   Stew (Ron's choice). Also: with no factory tree, a never-seen radio type keeps the previous radio's
   iq/power files live (nothing lost, wrong cal until calibrated).
   Files: live `~/.local/mscc/`, parked `~/.local/mscc/cal/<line>/` (iq.ini, power_cal.ini,
   amplifier_cal.ini), `cal/LAST_LINE.txt`, factory `/usr/share/mscc/factory/` (in the mscc .deb
   since Stew's 1.0.51 packaging change; without it only new-radio seeding is skipped).
   **Built on the Pi 2026-10-03 (ms-sdr + sdrcore-trans), Ron's Proficio MKII: first start logged
   `Factory_seed. major=3 line=proficio-mkii` + 3 `cal bootstrap` lines; WPF Save settings logged
   `Factory_park. done line=proficio-mkii major=3 files=3`.** Radio swap not testable (Ron has one
   rig). Pi still has mscc 1.0.50, so "factory tree missing" is logged (resets fall back / do
   nothing until a package with `factory/` is installed).
   **mscc 1.0.52 built 2026-10-03** (WSL; LF export of the git index + the new Pi ms-sdr and
   sdrcore-trans; sdrcore-recv as 1.0.49+; same file list and modes as Stew's 1.0.51, incl.
   `factory/`). In `rpi/mscc-deb/` and `installers/rpi/` (1.0.51 removed there). **Installed on
   the Pi 2026-10-03 (Ron): log shows `cal keep live` x3 + `keep live freq_cal.ini`, no "factory
   tree missing".** Resets not pressed (would reset Ron's cal).
   **mscc 1.0.53 (2026-10-03):** same servers as 1.0.52; only change = packaged
   `usr/share/mscc/bin/bootloader-gui` now has Stew's cmd-063 "Load File opens in
   /usr/share/mscc/firmware" (copied from `rpi/psoc-usb-bootload-linux/bootloader-gui.py`; the
   package copy is separate and Stew had not updated it). In `rpi/mscc-deb/` and `installers/rpi/`
   (1.0.52 removed there). Not installed on the Pi yet.
   `installers/rpi`: `mscc-init-gui_1.0.17_all.deb` removed again (Stew re-added it);
   `mscc-firmware_1.0.0_all.deb` (Stew, files only under /usr/share/mscc/firmware/<Radio>/, 8 radios)
   kept, INSTALL.md lists it as optional; not installed on the Pi yet.
   Note for Stew: `.mscc-coord/NOTE-FOR-STEW-UBUNTU-IQ-INI-2026-10-03.md` = ONLY the Ubuntu
   iq.ini delete (Ron 2026-10-03). Ron chose NOT to tell Stew about the LAST_LINE overwrite
   case or about his three commits in rpi/ + installers/rpi; don't raise them with Stew. Build recipe that works: `git -c core.autocrlf=false checkout-index` of
   mscc-deb/build-deb.sh + packaging, mscc-binaries, mscc-init-files-linux, tty0tty-master/module
   and factory into a temp dir, copy to WSL /tmp, files 644 / dirs 755 / index-755 files 755,
   run build-deb.sh. Committed c3c48a5 (port + 1.0.52), 0882e3f (1.0.53).
   To test: build ms-sdr AND sdrcore-trans on the Pi; log should show `Factory_seed. major=N line=...` and
   `cal bootstrap live→line=...` on first start; WPF Save settings -> `Factory_park. done ... files=3`.
   Then a new mscc .deb (1.0.52) with the new ms-sdr binary.
   Stew's push also touched `rpi/` (mscc-deb build-deb.sh + control, mscc_1.0.51_arm64.deb,
   psoc-usb-bootload-linux default folder) and put `mscc-init-gui_1.0.17_all.deb` back in
   installers/rpi (we replaced it with mscc-init 1.0.18) and added `mscc-firmware_1.0.0_all.deb`.
   Ron has not ruled on those.
12. ms-sdr thread names (2026-10-03, Ron asked): `Set_Thread_Name()` in `platform_linux.c` /
   `platform.h` (`pthread_setname_np`, max 15 chars), called first thing in each of the 11 thread
   functions: cmd-processor, gui-send (main-controller.c); log-flusher, last-used, ptt-switch,
   key-status (main.c); temperature; freq-queue; cat-port, cat-pin-check (comm-port.c); swr-meter.
   **The main thread is NOT named on purpose:** its name is the process name that `mscc.sh`
   finds with `pgrep -x ms-sdr` / `pkill -x`. See them with `top -H -p $(pidof ms-sdr)` or
   `ps -T -p $(pidof ms-sdr)`. Syntax-checked + helper run in WSL (names show in `ps -T`).
   **Built on the Pi 2026-10-03 (Ron): `ps -T` shows ms-sdr, libusb_event (libusb's own) and 10
   named threads; `last-used` is not in the list because its `pthread_create` in main.c (~1228) is commented out: that thread is never started.** Committed 67d4260.
   sdrcore-trans / recv threads not named (not asked).
13. **-12 kHz spectrum spur removed in the DSP (2026-10-04).** `SDRcore-recv-linux/sources/dsputils.c`
   `framesToComplex`: DC blocker on raw I and Q, `y = x - x_prev + DC_BLOCK_A * y_prev`,
   `DC_BLOCK_A 0.98f` (-3 dB at ~300 Hz). `panadapter.c`: pixel notch + fill code REMOVED (Ron
   2026-10-04; old code is in git, b08af65 / ef1d612). **Built on the Pi with the notch switched
   off, Ron: FT8 on 21.074 OK, "no spike, no discernible dips"; still clean with the full FFT.**
   Committed 8f09661 (pushed), with the full FFT and the cleanup (14).
   Why 0.98: 0.9999 (1.5 Hz) was tested first, spur came back with the notch off. Raw I/Q recording
   (servers stopped, `parec` from the Proficio source, dummy load, 8 s; `sdrcore-recv` holds the
   device directly, so parec gives 0 bytes while it runs): signal ~1 count rms of 16 bit, DC -0.5
   count, a comb at 16.08 Hz + harmonics (source unknown), noise rising toward DC, still above the
   floor ~300 Hz out. Emulated display: hump +27 dB at DC, 0.9999 -> +23, 0.99 -> +5, 0.98 -> ~0.
   Blanker (`blanker.c`, envelope-relative) not affected; audio / FREQ CAL 12 kHz away.
   **Spectrum FFT was half empty, fixed 2026-10-04 (Ron OK'd).** Blocks are 2048 frames but
   `doPanadapter` copied 4096 from `incplx`; the upper 2048 were never written (zeros), so the 4096
   Hamming was cut off at its peak -> wide skirts on every strong signal (-21 dB at 6 bins vs -43 or
   better). Now `pan_hist[4096]` in `doPanadapter` keeps the previous block (updated every call,
   also on the early return). Display only; audio/S-meter/cal use `fastconv`. Expected: FFT
   magnitude x2 on carriers, x1.41 on noise = about +3 / +1.5 on the client scale (client dB is
   `Y/150 - 40` = 10*log10(mag), `RawYToDb`), so NOT the 6 / 3 dB first told to Ron -> client dB
   CAL redo. Not measured yet. **Built on the Pi, Ron: "signals look
   narrower, audio OK".** In 8f09661. Windows/Ubuntu recv have the same code.
   2026-10-04 "no signals on the spectrum" after an `mscc.sh stop` / start was a bad server start,
   fixed by restarting again; not the blocker.
   Notch removal built on the Pi 2026-10-04, Ron: "spectrum looks the same".
   Open: level shift / dB CAL not measured yet (Ron, or Stew per cmd-064).
15. **mscc 1.0.54 + brief cmd-064 (2026-10-04).** `mscc_1.0.54_arm64.deb` built in WSL with the
   usual recipe (LF export of the index): new `sdrcore-recv` (Ron's Pi build 2026-10-04: DC
   blocker + full FFT, no notch; checked with nm / DWARF) and new `ms-sdr` (thread names, Ron's
   Pi build 2026-10-03, he had forgotten to copy it for 1.0.52/53); `sdrcore-trans` unchanged.
   Same 110 files / modes as 1.0.53, only those two binaries differ. In `rpi/mscc-deb/` and
   `installers/rpi/` (1.0.53 removed there). Not installed on the Pi yet (the Pi runs the same
   hand-built binaries). Pi recv is still `VERSION_MINOR 141` (Windows/Ubuntu 142, 143 after
   cmd-064); not bumped, Ron not asked yet.
   Brief `.mscc-coord/briefs/cmd-064.md` + COMMANDS.yaml entry: Stew ports the DC blocker, the
   full FFT and the notch removal to Windows + Ubuntu recv (3.143), measures the level shift.
14. **SDRcore-recv-linux cleanup (2026-10-04, Ron: .o files, anything Windows, clutter).** Tree is now
   Linux only: Makefile, 2 .md, udp_smoke.sh, `sources/` = the 15 built .c + mscc_resampler +
   `resampler/` + their headers + `portaudio.h` (kept: WSL syntax check falls back on it; the Pi
   uses /usr/local/include). Deleted (54 tracked files, restorable from git, plus 24 untracked .o):
   pthreads-win32 `pthread.h`/`sched.h`/`semaphore.h`, `sdrcore-recv.c` (old Windows main),
   `tonegen-old.c`, `panadapter - Copy.c`, `getwav.c`, `sendwav.c`, `wavfmt.h`, `mfc.h`,
   `portaudio_stub.c`, `log_msg.c/.h`, all `.bak-*` / `.copy`, `backup-nr/`, unused duplicate
   `sources/include/`, root `tonegen.c`, empty `core`, x86-64 `sdrcore-recv` binary.
   Windows branches stripped from built files: `extern.h`, `platform.h`, `platform_linux.c`,
   `main.c` (My_getenv, host API pick, speaker list), `print-utils.c`, `udp_thread.c` (bad recv =
   continue), `sdrcore.h` + `dsputils.c` (WIN32 / WINDBG), `_CRT_*` defines, Makefile comment.
   Kept on purpose: the Linux versions of Windows names in `platform.h` (Sleep, MessageBoxA,
   SOCKET, WSAStartup...), still used all over. All 15 .c + 9 .cpp syntax-check clean in WSL with
   the Makefile flags. **Built on the Pi 2026-10-04, Ron: "spectrum and audio OK".** In 8f09661.
   SDRcore-trans-linux has the same kind of leftovers (VS project files, `libs/` Windows libs,
   pthreads-win32 headers, .bak files): only listed, nothing removed (Ron said stop; he meant recv).
16. **800-bin spectrum fix ported from Ubuntu (2026-10-05, Stew's `.mscc-coord/NOTE-FOR-RON-rpi-pan-800.md`).**
   `ms-sdr-linux/source/user_controls.c` `CMD_GET_SET_PANADAPTER_REFRESH`: was `< 1` -> 6, so index 0
   (800 bins) never reached recv; now only `> 10` -> 6 (0/1/2 = 800/1600/3200, 3-10 = refresh blocks).
   Recv unchanged. Settings load (line ~406) still turns a saved 0 into 6, same as Ubuntu.
   **Built on the Pi 2026-10-05, Ron: "800 works".** Committed 10d90bf (pushed).
   **mscc 1.0.55 built 2026-10-05** (WSL, usual recipe): same 110 files / modes as 1.0.54, only
   `ms-sdr` (Ron's Pi build 2026-10-05) and the control version differ. In `rpi/mscc-deb/` and
   `installers/rpi/` (1.0.54 removed there). Committed accd904 (pushed). Not installed on the Pi yet.
7. Next mscc .deb build picks up the "(package mscc-init)" hint text in mscc-deb postinst /
   build-deb.sh / install-mscc.sh (source only, committed 82b819b). No rebuild just for that.

Done: cmd-042 hi-cut (mscc 1.0.48), FREQ CAL fixes (1.0.49), QRP/QRO cal + amplifier.ini
removed (1.0.50, installed + works). mscc-init C CLI dropped from the mscc .deb in 1.0.47.

## Per-radio cal on the Pi - OLD DRAFT, superseded 2026-10-03 (see Open list 11; 0x29 now means CMD_SET_PARK_CAL_SETTINGS)

"Parked" (Stew's word) = saved copy of one radio model's cal files, kept in a folder per model
while another radio is in use. Per model, not per individual radio.
Windows as built (cmd-055/059, `ms-sdr-MKII/source/factory_seed.c`, WPF `CalPark.cs`): ms-sdr copies
the files at startup (same line: keep live; other line: stash live -> old line, parked -> live; new
line: factory -> parked -> live), then tells trans to reload. Save settings = the client copies files
on the Windows PC. No new opcodes. Does nothing for a remote Pi.
Claude's Pi draft (Stew has not seen it):
- Live `~/.local/mscc/`; parked `~/.local/mscc/cal/<line>/` (`iq.ini`, `power_cal.ini`,
  `amplifier_cal.ini`); `cal/LAST_LINE.txt`; factory `/usr/share/mscc/factory/{iq,power,freq}/<line>/`
  in the mscc .deb (repo `factory/` tree). Line names from FW major: 1 proficio-legacy, 2 geminus-mkii,
  3/4 proficio-mkii, 5 geminus-legacy, 6 ultimus-legacy, 7/8 ultimus-mkii.
- **trans does the copying** (new file, e.g. `cal_park.c`), so trans stays the only writer of
  `power_cal.ini`. Flush pending slider saves before any copy.
- `0x29 CMD_SET_RADIO_LINE` ms-sdr -> trans, data = FW major (sent next to `CMD_SET_PCB_VERSION`,
  main-controller.c:1417). trans compares with LAST_LINE, swaps if different, reloads its tables.
- `0x2A CMD_SET_CAL_SAVE` client -> ms-sdr -> trans: live -> `cal/<line>/` (Save settings; works
  remote; WPF change = Stew). 0x29-0x2F were the only free numbers (Pi + Windows headers, opcodes.txt,
  WPF source checked). Names/numbers not approved.
- ms-sdr never writes the cal files; re-reads power_cal/amplifier_cal after the swap for the client.
- Do NOT use trans `CMD_SET_IQ_DEFAULTS` (0x8D) to reload after a swap: on the Pi it deletes
  `iq.ini` and rebuilds defaults.
- Same as Windows: `freq_cal.ini` seed-if-missing only, `recv-iq.ini` not parked, no auto-park on
  exit, no mirror. Swap is detected only at ms-sdr start (restart needed).
Ron (2026-10-02): two different methods make no sense; Windows should do it the trans way, but he
does not want to fight that battle now. Open: trans vs ms-sdr copying; opcodes; park freq_cal /
recv-iq too?; restart-to-detect OK? Alternative = Pi copies the Windows method + only 0x2A.

## mscc-init package (2026-09-28, committed 82b819b; Stew brief cmd-050 = 443d7a4)

Ron missed the CLI after 1.0.47 and wanted it in Python, kept in the init-gui package.
- `rpi/mscc-init-gui/mscc_init_gui/cli.py`: port of C `mscc-init-linux/sources/main.c` (same 4
  steps/prompts), reuses config.py/devices.py (SWR keep fix; radio I/Q devices hidden like the
  GUI; warns if servers run). `/usr/bin/mscc-init`. **Works on the Pi (Ron).**
- GUI blank window (old bug, also in 1.0.15): "servers running" askyesno ran before the root
  was mapped and stayed hidden. Now `after(200, _startup_server_check)`, dialogs `parent=self`.
  **Works on the Pi (Ron, 1.0.17).**
- Package renamed (Ron: old name misleading): `mscc-init` 1.0.18, `mscc-init_1.0.18_all.deb`,
  Provides/Conflicts/Replaces mscc-init-gui (dpkg upgrade from 1.0.17 checked in WSL). Commands
  `mscc-init` / `mscc-init-gui`, folder `rpi/mscc-init-gui/`, `/usr/share/mscc-init-gui` unchanged.
  In installers/rpi (INSTALL.md + install-mscc.sh updated).
- Build: `build-deb.sh` in WSL Debian from an LF copy (strip CR with sed), repo files are CRLF.
- The C source `mscc-init-linux/` and `mscc-binaries/mscc-init` are kept but unused.

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
  Limit explained (Ron OK'd this wording 2026-09-29): the limit is a bar - FREQ CAL measures how
  loud the tone is (average of low/center/high peaks, udp_thread.c:289); above the bar = found,
  below = fail (freq 0). Normal 5,000,000, LOOSE 1,000,000. Complex Goertzel = a better ear: tone
  reads louder (x2, +6 dB), noise also louder but less (x1.41, +3 dB), so the tone stands out
  more (the 3 dB gain). Everything reads louder, so the bar must go UP: x2 (10,000,000 / 2,000,000)
  = same as today; x1.41 (~7,070,000 / ~1,414,000) = a 3 dB weaker tone passes, noise still
  doesn't (the benefit). Left at 5,000,000 -> noise could pass as a tone.
Future item (Ron 2026-09-29): FM transmit. trans already has it: wire mode 5 -> MODE_FM
(`udp_thread.c:1277`), `main.c:327` path (skips fastconv), `fm_modulate` (`dsputils.c:434`),
fixed `FM_PEAK_DEV_HZ` 5000. Never tested on air (as far as known). Plan:
1. trans: deviation becomes a setting via a new opcode (CMD_SET_FM_DEVIATION, 3000/5000),
   default 3000 (safe). US rule: below 29.0 MHz modulation index <= 1 -> 3 kHz; 29.0-29.7 -> 5 kHz.
2. trans: mic audio is x2 then hard-clipped -> harmonics above 3 kHz break index <= 1. Add a
   limiter + 3 kHz low-pass AFTER the clip. Optional pre-emphasis.
3. ms-sdr: forward the new opcode to trans. Mode letter: 'F' goes to the rig unchanged and
   that is fine - firmware only cares 'C' vs not-'C'; si5351.c A/U/L/C switch only sets
   E_tune_freq, which nothing reads (dead); LO = E_current_LO_freq for all non-C modes; TUNE
   already goes as 'T' and works. display.c is legacy, not in the build (Ron). No 'U' mapping,
   no firmware change.
4. Client: picks 3 or 5 kHz from TX freq vs 29.000 MHz, sends on FM select and on crossing 29.0.
5. Test (Stew, spectrum analyzer): deviation (Bessel null) at 3 and 5 kHz, occupied BW.
6. Windows / Ubuntu trans + ms-sdr same. Ron: comment the code heavily.
DONE 2026-10-04 with a wider blocker (0.98), see Open list 13. Old note: real fix for the -12 kHz spectrum spur =
DC blocker on raw I/Q. Spur is I/Q DC (ADC offset + LO leakage); `complex_shift` runs
`doPanadapter` on raw I/Q, then shifts by fixed 12 kHz (`loFreq`), so DC sits at VFO-12k,
display only (audio never hears it). Add `y = x - x_prev + a*y_prev`, a ~0.9999, on I and Q
right after `framesToComplex` (main.c:342), before `complex_shift`. Caveat: notch needed
+/-6 bins (pure DC would be ~+/-2), so part may be non-steady skirt; test with notch kept,
then `MP_HALF_BINS` 0 to see what's left. Windows/Ubuntu recv same path.

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

## Done (mscc 1.0.48): cmd-042 (from pull f9efa00, 2026-09-25)

Windows `mscc-recv` 3.141 added 0xD1 CMD_SET_BW_HICUT index 5 = 1400 Hz, 6 = 1000 Hz
(DIG-U Hi, WPF cmd-041). Pi to match in `SDRcore-recv-linux/sources/udp_thread.c`
(hi-cut switch ~line 1547) + version bump -> `mscc_1.0.48_arm64.deb` (1.0.47 used 2026-09-27). Brief:
`.mscc-coord/briefs/cmd-042.md`. Order: after ubuntu-stew is done and Stew pushes.
Reviewed: 0-4 unchanged, low-cut max 500 < 1000, ms-sdr passes index through. Done, see Open list.

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
