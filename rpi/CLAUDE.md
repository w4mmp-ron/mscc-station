# CLAUDE.md — sdrcore (Pi servers)

Claude's notes for the Pi servers `sdrcore-recv`, `sdrcore-trans` and `ms-sdr`
(Proficio / MSCC station). Owner: **Ron (W4MMP)**. Grok follows `AGENTS.md`; this
file is for Claude only.

## Working with Ron

- Short, plain-English answers. Say what is verified and what isn't.
- **"stop" means stop immediately.**
- Ask before large or risky changes. `git add` specific paths only (Grok and other
  tools work in this repo too).
- **Rules since 2026-11-11 (Ron):** (1) NO commits until Ron says we are finished (notes too:
  "update the notes" = edit only). (2) Keep in sync: every server change (ms-sdr, recv, trans)
  goes into BOTH the Pi tree (`rpi/*-linux`, copied to V:) and the Windows tree
  (`mscc-ui/windows-work-tree/`); every WPF client change also goes into Avalonia (Ron's copy
  `worktrees/Avalonia`). (3) On "finished": build the relevant installers (Windows R-build,
  Pi `mscc`, `mscc-avalonia` if the client changed), one commit, push, GitHub Release.
- Ron does **not** `git pull` on the Pi. He copies the changed source files from
  Windows to the Pi and builds there. Give him the file list, not git steps.
- Ubuntu (`linux/`) work is Stew's; don't track or report it. After a pull, report
  only Stew's **Windows** changes and whether the Pi needs a matching port.

## Folder rules

| Path | Rule |
|---|---|
| `rpi/SDRcore-recv-linux/`, `rpi/SDRcore-trans-linux/`, `rpi/ms-sdr-linux/` | **Working trees** (Pi source of truth). Claude edits here. |
| `swr-meter/ron/HF_SWR_R4_1/` | **Our copy of the WiFi SWR meter firmware** (ESP32 Arduino; copied 2026-11-01 from Stew's `swr-meter/HF_SWR_R4_1/` @ 67fa644). In git since 2026-11-04: the source plus `swr-meter/ron/build/HF_SWR_R4_1.ino.bin` (the file to OTA-load at http://hf-swr-meter.local/update); other build output stays local. Claude edits here; Stew's copy is read-only. |
| Rest of `C:\Users\Ron\.grok\worktrees` | Read-only reference unless Ron says otherwise. |

History: Claude worked in a copy `rpi/sdrcore-claude/` (commits fe0c9a1, 0443c21,
9bb4be2, 4cb8b74). On 2026-09-25 Ron had the changes moved back into the originals
(originals unchanged since 8dee14a) and the copy deleted.
Git repo root: `C:\Users\Ron\.grok\worktrees`.

## Open list (Ron, updated 2026-11-01) - read first on resume

- **Commits (Ron, 2026-10-05): too many commits and pushes.** Finish everything that belongs
  to a piece of work first (code, notes, note for Stew), then ONE commit and one push. No
  follow-up "notes: committed xyz" commits.

**HOLD LIFTED 2026-11-10 (Ron: Stew gave the go-ahead to implement all the changes). Was: ON HOLD (Ron, 2026-11-02): no new changes until Stew catches up on the important Pi work
(automatic NB, spectral NR, NR Low/Med/High, client controls).** When he pushes: pull, short
review of what he ported, say if the Pi needs a matching change. Don't start new work meanwhile.

**Stew's list, Ron doing it (2026-11-10): servers first, then client. Versions after all server changes.**
- Done (not on air yet, Windows rig needed): Windows recv automatic NB (blanker.c + struct), Windows recv spectral NR (denoise()).
- TX IQ RESET ALL (Ron 11-10): ms-sdr writes the FACTORY TX IQ values into the radio (no clear). A radio with factory values = calibrated (to factory), unlike a new rig with nothing stored. DONE in Pi ms-sdr (iq.c, factory_seed.c/.h; copied to V:, syntax-checked, NOT built) and Windows ms-sdr (built). Ron tests later (needs a full calibration after).
- Windows ms-sdr, built, not on air: heat sink 0x06 read + 30 s log (proficio-temperature.c); WiFi SWR meter port (swr_wifi_meter.c added to vcxproj, Windows clock / thread-name shims; 0x0F fault, 0x2E reset, tune flag, SWR_* ini keys; needs SWR_METER_TO_GUI=1 in mscc.ini, firewall UDP 6999, meter sending to the PC); Pill_stats (PILL_STATS=1). Windows build auto-bumped ms-sdr 3.189 -> 3.191.
- Server list done 2026-11-10. Versions (11-10): Windows recv 3.148 -> 3.152 (= Pi recv 3.152 features), Windows ms-sdr 3.196 and Pi ms-sdr 3.196 (built on the Pi into ~/mscc/ms-sdr, runs after the next mscc restart). Pi recv unchanged (3.152, rename only). Next: the client list.
- **Hard calibration resets (Ron 2026-11-10).** Soft = host files only (radio wins at next start); hard = files AND radio. New client opcode 0x2F CMD_SET_CAL_RESET_ALL (1 byte: 1 RX IQ = 0/0 all bands, recv + radio; 2 QRP power = factory table, trans saves + radio; 3 AMP = trans rewrites amplifier_cal.ini -99, host only). Pi ms-sdr Radio_cal_reset_all (power_calibration.c) + trans CMD_SET_AMPLIFIER_CALIBRATION_RESET switched on (udp_thread.c): on V:, syntax-checked, NOT built. Windows ms-sdr 3.197 + trans built. Client 11.10.7: RESET ALL on RX IQ / QRP CAL / AMP CAL (Yes/No, then OK-only pop-up). Existing hard resets: TX IQ RESET ALL (0x8D = CMD_SET_IQ_DEFAULTS, factory to radio since ms-sdr 3.196), FREQ CAL RESET. Reset configuration = client settings only (keeps iq/recv-iq/freq_cal/power_cal/amplifier_cal.ini + cal).
- **Client list DONE 2026-11-10 (client 11.10.13, not on air on a Windows rig):** NB Low/Med/High + pulse 50..510 + NB sent at connect; NR Low/Med/High; heat sink display; SWR RESET via 0x2E; 0x30 server-message pop-up; RESET ALL pop-ups; Reset configuration = settings only; hard resets (0x2F); PADDLE checkbox (0x9D); cal band-box fix; firmware-update pop-up (0x2D store=0: OK-only, once per run, lists minimum firmware); SWR fault now also turns TUN off (TESTED Ron 11-10 on the Pi: fault, pop-up, TUN off, RESET via ms-sdr). 0xD7-0xD9 removed from the client.
- **Servers 2026-11-10 (late):** Pi + Windows ms-sdr 3.200: server last-used band stack (Last_used.ini, vfoa/vfob) REMOVED (thread was commented out in both, client never asked for it, client keeps its own last-used); 0xD7-0xD9 reclaimed (usbavrcmd.h free list). Pi client keep-alive timeout 45 -> 15 s (= Windows). trans 3.157 both (Pi built). Pi runs ms-sdr 3.200, trans 3.157, recv 3.152, pill 3.15.
- **Pill 3.15 (2026-11-10, flashed, works):** heat sink below -40 C = OPEN (-99), above 150 C = SHORT (999); was clamped, an unplugged thermistor showed -40 C (pull-up is on the board, Ron).
- **Windows installer now builds from `mscc-ui/installer/windows/`** (payload\ + MSCC.aip + stage.ps1 + README), not C:\mscc-net9. Actual Installer: `actinst.exe /S MSCC.aip` -> `installers/windows/`. PSoC firmware = newest versioned build per radio (1.236 / 2.153 / 3.243 / 4.244 / 5.153 / 6.236 / 7.244 / 8.243), Black Pill = 25 MHz bins + flash tools (no 8 MHz). R11-10-13 built. Open: MSCC-Remote has no Release build (Oct-31 copy in payload); MSCC.aip paths are Ron's PC.
- **Pi package mscc 1.0.77** (2026-11-10): ms-sdr 3.200, trans 3.157, recv 3.152; built on the Pi from a /tmp copy (CR stripped from scripts), in `installers/rpi/`.
- **2026-11-10 (night): Pi and Windows servers made functionally identical (Ron).** Platform code (USB, sockets, paths, audio device discovery / ALSA-Pulse vs MME) stays different. NOT on the air yet.
  - ms-sdr: Windows 3.206, Pi 3.203. Windows got the Pi's 1 s Flusher loop (client timeout ~15 s, was ~45 s; core check ~10 s), CAT `MDn;` mode set (was commented out), mode saved to startup.ini on every change, signed RIT +-500 Hz, Radio_cal_startup_load (radio cal read at first connect if startup missed it), safe cal copies (.tmp + replace, .bak of parked, trans INITIALIZE before park), default cw.ini / mscc.ini instead of stopping, host name fallback 127.0.0.1, the Pi user_controls.c (ALC on/off saved, NR, pan guards), FARNSWORTH 1-4 -> 5. 3.201: a client leaving no longer stops the Windows servers. Both: TUNE never saved to startup.ini (stop while TUN on came back in TUNE); `last_used.c/.h` renamed `startup.c/.h` (only startup.ini code left); WATERFALL_STATUS `;` fixed in user_controls.ini.
  - recv 3.153 both. Windows got the Pi udp_thread.c (socket error = log + continue, digital volume per mode, I/Q band guard), float FREQ CAL / CW snap magnitudes, server pop-ups (I/Q device / digital speaker / audio start failed), 0xF4 keep-alive with source id, recv-iq.ini reload after create, Audio_Device_Error exits (was a NULL crash with no I/Q device), normal stop just exits. Both: 10 ms pause after a recvfrom error.
  - trans 3.158 both. Ron's choices: mic gate at key-down (35 / 200 ms + 10 ms fade) and 5 ms TUN ramp on both; ALC ceiling 1.0 digital / 0.89 voice on both (Pi digital was 0.89); line gain 2.5x for Digital, Remote-Digital AND Remote-Phones (only the local Phones mic gets 6.3x); voice ALC meter 0-10 dB. Windows got the Pi remote mic receiver (100 ms priming, resync >250 ms, +/-300 ppm trim, no pitch steps), recvfrom skip (was: trans stopped and handled a stale buffer), pop-ups, exit(1) on start-up failure, 0xF4 keep-alive, Flusher no busy spin, power-table log index fix. Pi got the phase reapply after CMD_SET_IQ_DEFAULTS. Both: a close error at a normal stop is only logged. TO CHECK ON AIR: Pi WSJT-X power / ALC (digital gain 6.3 -> 2.5, ceiling 0.89 -> 1.0); Windows Remote-Phones voice needs ~8 dB more mic gain.
  - Client: WPF 11.10.14 and Avalonia 1.6.93 / 1.6.94: SERVER button = Connect / Disconnect with a tooltip saying whether the servers start / stop; checkbox "Launch Servers" on both; Avalonia sends CW BW before CW pitch at connect (recv "CW Bandwidth NOT SET").
  - Installers: Windows R11-10-14, Pi mscc 1.0.78 + mscc-avalonia 1.6.94 in `installers/`. Pi runs ms-sdr 3.203, recv 3.153, trans 3.158, pill 3.15.
  - **Installers are GitHub Release downloads, not committed** (`.gitignore` installers/windows/*.exe, installers/rpi/*.deb; Ubuntu installers/linux unchanged). Release R11-10-14: https://github.com/w4mmp-ron/mscc-station/releases/tag/R11-10-14.
  - **Pi = 2 packages since mscc 1.0.79 (Ron):** `mscc` now also carries PortAudio (/usr/local lib + headers, loader file /etc/ld.so.conf.d/mscc.conf), MSCC Init, MSCC Volume, PSoC firmware (/usr/share/mscc/firmware/<Radio>/, same versioned files as the Windows payload) and Black Pill 25 MHz firmware (/usr/share/mscc/firmware/black-pill/). Conflicts/Replaces/Provides mscc-portaudio, mscc-init(-gui), mscc-volume(-gui), mscc-firmware: apt removes them. build-deb.sh takes the firmware from mscc-ui/installer/windows/payload (run stage.ps1 first) and PortAudio from rpi/mscc-portaudio/mscc-portaudio_19.8.3_arm64.deb. Installed on the Pi 2026-11-10 (old packages removed, servers running); mscc-ui 0.6.31 purged from the Pi. Optional: `mscc-avalonia`. INSTALL-FOR-PI .docx/.pdf not regenerated.

**2026-11-11 (finished, Ron): Release R11-11-2.** Windows `mscc-net9-R11-11-2-install.exe` (WPF 11.11.2,
ms-sdr 3.208); Pi `mscc_1.0.80_arm64.deb` (= 1.0.79 + ms-sdr 3.206, md5 = the running ~/mscc/ms-sdr) and
`mscc-avalonia_1.6.95_arm64.deb` (both also on V:\; both INSTALLED on the Pi 2026-11-11, servers
were stopped, /dev/tnt0-1 and the .ini files fine; Avalonia 1.6.95 not run yet). INSTALL docs +
Windows CHANGELOG updated, INSTALL-FOR-PI .docx / .pdf regenerated. Pi runs ms-sdr 3.206, recv 3.153,
trans 3.158, pill 3.15.
- **WSJT-X right-click Tune (CAT `AC111;`) DONE, tested on the Pi (Ron).** Capture (strace on
  ms-sdr's /dev/tnt0): each right-click sends `AC111;ID;`, nothing to end it, and WSJT-X sends
  NOTHING for a right-click while its tune runs (so the tune can't be stopped from WSJT-X).
  ms-sdr `comm-port.c`: AC111; = MSCC TUN carrier like the client TUN button (mode T, tune power if
  the client sent one, Tune_button = SWR meter tune flag before RF, TX on), client gets
  CMD_SET_TX_SET_BY_SERVER 1 (as CAT PTT; client locks PTT/TUN). Ends after 10-11 s
  (`Cat_tune_tick()` from the 1 s Flusher loop, main.c), at a second AC111; (other apps), or
  before a CAT `TX;`; then TX off, old mode back, TX released. Ended elsewhere (SWR / temperature
  fault, client mode change / disconnect): tick puts the mode back if still T and releases TX.
  An AC111; within 1 s of the start is ignored (seen 4 times ~150 ms after the start, cause not
  found, not seen under strace). Log: `CAT tune. START / STOP (reason) / ignored`. Files: comm-port.c,
  main.c, extern.h in both trees. Pi ms-sdr built (version.h on the Pi 3.206), Windows ms-sdr 3.208
  built, not on a Windows rig. Tune power shows 0 in the log until the client sends it (trans
  then uses its own; Ron: power looked normal).
  **Later 11-11 (Avalonia 1.6.95 on the Pi):** a second right-click 6.4 s into a tune DID arrive
  (`MD2;` + `AC111;`, 16:00:49 UTC) and stopped it; at 2-3 s earlier it sent nothing. Ron: WSJT-X
  right-click Tune itself looks buggy (sometimes sends, sometimes not). ms-sdr handles both. Note:
  an `MD2;` during a CAT tune changes the mode with the carrier still keyed (harmless in USB);
  option, not done: hold CAT mode sets until the tune ends.
  Avalonia 1.6.95 meter in watts: fine (Ron). SWR fault 3.40 on 15 m at 16:00:33 = Ron's
  left-click Tune by mistake (antenna not matched there), not a bug.
**2026-11-11 (finished, Ron): Release R11-11-4.** S/W **SPECTRUM SIZE** slider (20-80 %, default 40,
under Climbing): spectrum share of the panadapter height, waterfall gets the rest (was fixed: WPF 38 %,
Avalonia 40 %; the waterfall rows are resampled to its height, so nothing else changes). Global, not per
band. WPF: SpectrumColorSettings.SpectrumSplitPercent (renderer + peak marker), SpectrumWaterfallSettings
`SPECTRUM_SPLIT_PERCENT` in MSCC_Client.ini, PanadapterControlsWindow slider. Avalonia:
SpectrumDisplaySettings.SpectrumSplitPercent (LoadExtras), SpectrumRenderer, ClientSettingsStore
`SPECTRUM_SPLIT_PERCENT`, SpectrumWaterfallViewModel + window. WPF 11.11.4 TESTED by Ron (Windows, saved 49 %,
no log errors); Avalonia 1.6.96 INSTALLED on the Pi 2026-11-11 (over 1.6.95), Ron trying it (he reports later if it fails). Installers: Windows `mscc-net9-R11-11-4-install.exe`
(servers unchanged), Pi `mscc-avalonia_1.6.96_arm64.deb` (on V:\, installed); `mscc_1.0.80` unchanged. Commit e203740,
GitHub Release R11-11-4 (the .exe + both .deb). Actual Installer: run `actinst /S` from PowerShell only (Git Bash
turns `/S` into a path, the GUI opens and nothing is built).
Option, not done: drag the divider on the display.
- **RELEASE STATUS (Ron, 2026-11-11): Windows and Pi are READY FOR RELEASE at R11-11-4** (GitHub Release
  R11-11-4, Latest: mscc-net9-R11-11-4-install.exe, mscc 1.0.80, mscc-avalonia 1.6.96). The Debian (Ubuntu x86_64,
  `linux/`, `installers/linux`) implementation is WAITING ON STEW; don't release it until he's done.
- **Firmware check (2026-11-11, Ron): MKII-style CW + QRP / I/Q cal store, from the source.** MKII-style CW =
  key-delay buffer (non-QSK amp delay), key-click shaping, paddles. Cal store = 0xC8 CMD_CALIBRATION with
  CAL_TYPE_POWER / RX_IQ / TX_IQ.
  - PSoC, all YES on both: Proficio MKII PTT 3.243 / Ultimus 8.243 (the MKII code); MKII ATU 4.244 / Ultimus 7.244
    (cw.c identical to PTT); Proficio Legacy 1.236 / Ultimus 6.236 (keys by gating DIN = TX I/Q, not the PA line);
    Geminus MKII 2.153 and Geminus Legacy 5.153 (relays latched for the semi-break-in session, no QSK). Shaping on
    PSoC = PCM3060 DAC soft mute. = the client's minimum-firmware list.
  - Black Pill: F411 25 MHz 3.15 (shipped) YES / YES, shaping = raised-cosine ramp on the TX I/Q (also TUN / SSB /
    digital), cal store also holds PPM. F411 8 MHz 3.14 (not shipped) YES / YES. STM32H743 3.232: paddles + QSK
    only (no key delay, no shaping) and NO cal store (client would ask for a firmware update); not shipped.
  - Empty leftovers in radio-psoc-firmware: `Proficio-Legacy`, `Proficio-MKII-PTT`, `Proficio-MKII-ATU`;
    `Proficio-Legacy - Copy` (1.236) is an old copy, the current Legacy is `Proficio-Ultimus-Legacy`.
- **proficio power (2026-11-11, Ron's custom buck board in the MII):** the Pi 5 said the supply can't give
  5 A. Cause: a Pi 5 asks the supply over USB-C Power Delivery for 5 A; a plain buck can't answer, so the Pi assumes
  3 A and limits USB to 600 mA. Supply is fine (EXT5V 5.07-5.09 V, get_throttled=0x0). Fix: EEPROM
  `PSU_MAX_CURRENT=5000` (rpi-eeprom-config --apply, rebooted 17:28): usb_max_current_enable=1 (USB 1.6 A), no
  warning in the log. Old EEPROM: BOOT_UART=1, BOOT_ORDER=0xf461, NET_INSTALL_AT_POWER_ON=1. The MSCC servers
  do not start by themselves after a reboot (MSCC Start). Undo: remove the line, reboot.
- **proficio backup card (2026-11-11, same as squireoaks):** USB-adapter card `sda` (29.2 GB). The old
  copy had the SD's disk ID 21382c14 (boot mix-up risk); Ron re-ran SD Card Copier with New Partition
  UUIDs -> copy ID e6308e0a (cmdline.txt + fstab match). rpi-clone 2.0.27 (geerlingguy fork,
  /usr/local/sbin, not on ron's PATH; Ron installed it). First `sudo /usr/local/sbin/rpi-clone -u sda`
  32 s, exit 0. Root cron: `0 3 * * * /usr/local/sbin/rpi-clone -u sda > /var/log/rpi-clone.log 2>&1`.
  Check after 03:00: log ends "Done with clone to /dev/sda". squireoaks: reach it only through
  proficio (`ssh proficio 'ssh ron@squireoaks ...'`); its clone ran OK 11-11 03:00 (49 s).
- **RX IQ START order fixed (client):** WPF `StartRxIq` and Avalonia `StartRxIqAsync` now send
  0x55 RX before 0x58 band (ms-sdr iq.c routes 0x58 by the last 0x55; band first after a TX I/Q
  session took the TX path: retune to the TX cal freq + trans TUNE). WPF 11.11.1, Avalonia
  1.6.95 (csproj + DEBIAN/control), both built, not tested on the rig. Test: TX I/Q cal, then
  RX IQ START: rig stays on the receive freq, trans not in TUNE.
- Stew note `.mscc-coord/NOTE-FOR-STEW-RX-IQ-TOOLS-2026-10-14.md`: option 4 (client AUTO
  button) removed (Ron).
- **Client power meter in watts (Ron, TESTED on WPF 11.11.2 at ~120 W, "the fix is fine").** Was
  x1 / x10 / x100 (10 / 100 / 1000 W full scale): 110 W jumped to x100 = needle at 1.1 of 10. Now
  full scale 10 / 20 / 50 / 100 / 200 / 500 / 1000 W, tick labels in watts (200 W = 0 40 80 120
  160 200), centre legend "W", up at 95 % of full scale, down below 30 % of the next lower one
  (as before). `AnalogSMeterControl.xaml.cs` (WPF) and `.axaml.cs` (Avalonia 1.6.95, built, not
  tried). New WPF + Windows ms-sdr 3.208 copied to `C:\mscc-net9` (old ms-sdr in
  `C:\mscc-net9-old\ms-sdr-MKII-3.206-backup.exe`).
- **WiFi SWR meter vs client (checked, fine):** tcpdump of UDP 6999 on the Pi during a WSJT-X PTT
  tune: packets every 0.1-0.2 s, `fwd` 120.8 easing to ~117.5 W, `peak` 122.2, `avg` 111-114,
  swr 1.32. ms-sdr passes `fwd` to the client (client ~120 W = right). ms-sdr logs only the FIRST
  meter packet after start (SD wear) plus fault edges; option (not done): one "TX end: peak fwd /
  ref / swr" line per transmit. `tune` = 0 on CAT PTT (flag only on CAT AC111; or client TUN).

**Current list (updated 2026-11-06, after bb31561):**
- **DONE 2026-11-11 (see above). Was: WSJT-X "right-click for rig tune" does nothing.** WSJT-X on
  the Pi (rig TS-2000) asks hamlib for the rig's tuner, which should send Kenwood `AC111;` (not yet
  seen on the serial line). ms-sdr `comm-port.c` only handles AI ID IF MD PS RM FW KS FA FB RX TX,
  so AC is ignored. Fix: handle `AC` in comm-port.c and start MSCC's own TUN carrier
  (`tune_modulate`); first capture what WSJT-X really sends, and decide how TUN ends (AC has no
  stop: timeout, or WSJT-X Tune released). Left-click Tune (CAT PTT + tone) works and stays as is.
  **What WSJT-X 3.0.2 can send (Pi uses hamlib 4.6.2; from hamlib source `rigs/kenwood/ts2000.c`
  + `kenwood.c` tag 4.6.2, not a capture; actual set depends on WSJT-X settings):**

  | WSJT-X action | hamlib sends | ms-sdr handles? |
  |---|---|---|
  | Connect | `ID;` (wants `ID019;`), `AI;`, `AI0;` if AI on, `IF;` | yes (`AI0;` no reply, fine) |
  | Read freq | `FA;` (`FB;` for VFO B / split TX) | yes |
  | Set freq | `FA;`, then `FA00014074000;`, then `ID;` | yes |
  | Read mode | `MD;` | yes |
  | Set mode | `MD;`, then `MDn;` (1 LSB 2 USB 3 CW 4 FM 5 AM 6 FSK 7 CW-R) | yes for 1-5 |
  | Read PTT / VFO / split | `IF;` (cached 0.5 s) | yes |
  | PTT on | `VX0;TX;` | yes (finds `TX;`, `VX0;` ignored) |
  | PTT off | `RX;` | yes |
  | Set VFO | `SA;`, then `FR0;` / `FR1;` | **no** |
  | Split (Split = Rig only) | `IF;`, then `FR0;FT1;` / `FR0;FT0;` | **no** |
  | Power status | `PS;` | yes |
  | Right-click Tune | `AC111;` (RIG_OP_TUNE) | yes since 2026-11-11 (10-11 s TUN) |

  Every set command except `RX` is followed by `ID;` (hamlib's check). Gaps: SA, FR, FT.
- **WiFi SWR meter (2026-11-06, Ron's copy `swr-meter/ron/HF_SWR_R4_1/`):** PEAK hold + AVG mode
  built, OTA-loaded and TESTED (Ron, WSJT-X); PEAK SWR beside the big SWR (later 2026-11-06) built,
  loaded, works (Ron); details in the SWR meter firmware entry further down.
  Pushed: firmware 44b02ec, note for Stew bb31561
  (`.mscc-coord/NOTE-FOR-STEW-SWR-METER-FIRMWARE-CHANGES-2026-11-06.md`), notes 1ab59db, PEAK SWR
  a2134e5 (has the "With Stew" list). The two PSoC CW top speed commits (0cec97e, 4ea90e3) come
  after them and are still local (hold). Open: test the WiFi retry
  (router off, meter restarted); Ron to decide if ms-sdr itself stops TX on a meter fault
  (backstop for WSJT-X on the Pi keying via CAT). Build: arduino-cli inside the Arduino IDE,
  FQBN esp32:esp32:esp32 with FlashSize=4M, PartitionScheme=min_spiffs (full option string in the
  2026-11-06 session); Ron to add Windows Defender exclusions (Arduino15, arduino cache,
  libraries, build) because builds were slow.
- **Pi runs:** mscc-avalonia 1.6.89 (Ron's Avalonia copy, below), recv 3.152 (automatic NB + spectral NR), trans 3.151 (2026-11-05: operator mic
  fix, see below), ms-sdr 3.195 (Pill_stats log, below), pill 3.14 (2026-11-10, 0x9D passed through),
  PIC keyer = keyer-buffer (buffered Farnsworth), package mscc 1.0.75 installed (Ron 2026-11-05).
  **mscc 1.0.76 built 2026-11-09** (`installers/rpi/mscc_1.0.76_arm64.deb`, also V:\ = Pi home)
  = 1.0.75 + ms-sdr 3.195 (Pill_stats; md5 = the running ~/mscc/ms-sdr). 125 files, same modes, no
  CR in text files. Not installed from the .deb yet. Build: scratchpad build-mscc-deb.sh (WSL Debian,
  stages under /tmp/mscc-src, strips CR from text only, runs build-deb.sh; session-only).
  INSTALL-FOR-PI.md now names 1.0.76 (the .docx / .pdf copies not regenerated).
  Pushed 2026-11-05: keyer + 0x9D work (fb90f87), mscc 1.0.75 (f78d141), pill 3.13 (a2e351d in
  psoc-replacement-stm32). Open from today: SSB on a dummy load to confirm the mic fix;
  Stew's PADDLE checkbox (note pushed). (PIC reset kink fixed 11-10, pill 3.14.)
- **2026-11-05 mscc 1.0.74 / 1.0.75 (`installers/rpi/mscc_1.0.75_arm64.deb`):**
  - **/dev/tnt* missing after a kernel update:** the Pi booted kernel 6.18.60 (installed 11-03);
    tty0tty was only built for the older kernels. 1.0.74 adds kernel hooks
    (`/etc/kernel/postinst.d/mscc-tty0tty`, `/etc/kernel/header_postinst.d/mscc-tty0tty` ->
    `/usr/share/mscc/bin/mscc-tty0tty-kernel <kver>`, log `/var/log/mscc-tty0tty.log`, never
    fails the kernel install) and postinst builds it for every installed kernel with headers.
    Checked on the Pi: hooks run by run-parts, build with headers, skip without; tnt0-7 back.
    Next kernel update: the log should say "built and installed" for the new version.
  - **Operator mic bug (sdrcore-trans, found 11-05):** `score_operator_input()` tied the headset
    speaker monitor (`alsa_output.usb-C-Media...analog-stereo.monitor`) with the real mic
    (`alsa_input...mono-fallback`), both 6200, monitor first -> trans used the monitor: phone TX
    would send RX audio. Fix: never a `.monitor` for the operator mic (input-devices.c). Built on
    the Pi (trans 3.151), log now shows the mono-fallback mic. **Not yet checked on the air**
    (SSB on a dummy load). Windows trans (Stew's) not checked.
  - **`mscc status` false FAIL "Operator speaker: no PortAudio device matches"** in Phones mode:
    the report wanted the exact ALSA name; with recv playing through Pulse the raw hw:3,0 shows
    0 outputs. 1.0.75 report scores operator devices like recv / trans (folded name, Pulse
    preferred, no .monitor mic), so it shows the device the servers open. Passed on the Pi.
  - Package build: WSL wrapper stages under /tmp and strips CR from text only (scratchpad
    `build-mscc-deb.sh`, session-only). 51 world-writable data files in the package (same in
    1.0.73), left as is.
- **Ron:** Option: NB threshold a bit right of middle (~5x, slider ~750) to also
  catch the near misses (5.4-6.0x seen on 20 m).
- **Ron (2026-11-02): update the guides: QRP power, TX IQ and RX IQ cal are now stored in the
  rig/daughter board.** Windows, Linux and Remote drafts (`docs/manuals/*-Release-1-DRAFT.docx`)
  and the Configurations draft: check every calibration / Save settings / reset text says so
  (APPLY stores in the radio, radio values win at start, power_cal.ini only for bands the radio
  doesn't hold).
- **Ron (2026-11-02): store QRO (amplifier) cal in the radio too.** Today ~/amplifier_cal.ini
  (trans owns, ms-sdr amplifier.c reads, 12 bands, int; -99 = none). Room checked (code read only):
  pill 3.12 = flash log sector 7, 128 KB, 16-byte entries, 4 value bytes: new type 8 (not 6 =
  old clear), no layout change. PSoC MKII (ATU/PTT 3.238+) = EEPROM 2 KB = 128 rows; used rows
  0, 1, 8-19 (cal row per band is full, byte 14 only spare); plan: 12 QRO rows 20-31, same
  one-row-per-band write. Then ms-sdr load/send as QRP, trans save flag, Windows ms-sdr port,
  client (Stew).
- **2026-11-02 later: PSoC comb fix handed to Stew (Ron: Stew does the PSoC, Ron does the pill).**
  Stew's answer (`NOTE-FOR-RON-TX-EDGE-COMB-ANSWERS-2026-11-02.md`): comb on BOTH CW edges, not
  audible on air; his better plan = fade the LIVE slot (copy + raised-cosine scale only during
  fades, SetTxBufAddress), all modes, no soft mute, no host change. Ron's go-ahead:
  `NOTE-FOR-STEW-TX-EDGE-COMB-LIVE-FADE-2026-11-02.md` (8-bit gain table, ISR or dummy-Zero for
  timing, latch the slot index, 5 ms). Old idea below for history.
  **Stew, 2026-11-02 afternoon: PARKED on his side** (`NOTE-FOR-RON-FIRMWARE-FOLDERS-AND-RELEASES-
  2026-11-02.md`): 3.244 (Buff TDs re-pointed, slots 1 ms late) = noise + stalled audio; 3.245
  (scale next slot in place) = RX and TX dead; neither committed, FW back at 3.243 / 4.244. He needs
  the real USB vs DAC slot timing. From SyncSOF.v: index steps at each SOF, forced to 1 when the
  player wraps to slot 0, SOF lands mid-slot at a PLL-held point -> USB is 1 ahead for part of
  each ms and 2 ahead for the rest. Ron: leave it to Stew (his Claude can read SyncSOF.v too).
  Same push: PSoC trees renamed `Proficio-Ultimus-Legacy / -MKII-PTT / -MKII-ATU`, Ultimus trees
  removed, releases in each tree's `Release/` named by version (copy-release.py, -b2 if same
  version differs; bump the minor for every test build); Ron: reopen Creator projects from the
  new folders, fix the post-build path.
- **Ron (2026-11-02): PSoC makes its own CW I/Q (fix for Stew's 12 kHz comb).** Comb = PCM3060
  soft mute 0.5 dB per 8/fs staircase at every non-QSK release / CW key-up, ~-65 dBc (Stew's
  note `.mscc-coord/NOTE-FOR-RON-TX-EDGE-COMB-DAC-SOFT-MUTE-2026-11-02.md`). PCM3060 volume
  stepping ruled out (still a staircase, too many I2C writes). Idea (code read only, not tried):
  trans CW carrier = 12 kHz at 96 kHz = 1 ms USB slot is exactly 12 cycles; 8051 grabs one slot
  from `TxI2S[n]`, builds raised-cosine fade buffers (192 multiplies/ms), plays them with
  `PCM3060_SetTxBufAddress()` (exists, unused); DAC stays unmuted. Grab window < 1 ms because the
  Zero DMA erases each slot behind the reader: in CW point the Zero TDs (`DmaTxStart()`, our code,
  not Cypress) at a dummy byte. Checks: trans sends a steady carrier in CW RX (Pi: `#ifdef RON`),
  8051 time, fade vs key delay. Pill already fades in software (5 ms raised cosine, fw 3.6).
  Reply to Stew: `.mscc-coord/NOTE-FOR-STEW-TX-EDGE-COMB-CW-2026-11-02.md` (pushed 7089904);
  waiting on his answer.
- **Pill CW edges on an analyzer: not checked yet** (pill 5 ms raised cosine, should show no comb).
- **Keyer: buffered Farnsworth on the paddles — DONE and in the rig (2026-11-05).** Details,
  design and build / flash steps: `keyer-buffer/RESUME.md` (top section). Ron: "all the work",
  "the speed sounds spot on". You key at your own speed and hear yourself; letters go into a
  buffer and go on the air with Farnsworth gaps (letters at SET_WPM, gaps at the 0x76 text WPM);
  20 letters waiting -> STP on the sidetone, paddles off, drain, RES. Switch = new opcode
  **0x9D SET_PADDLE_FARNSWORTH** (0/1): Pi `cw.ini` `CW_Paddle_Farnsworth=` (ms-sdr 3.192),
  pill 3.13 passes it to the PIC, client never sees it (no client change). Ron's cw.ini now:
  CW_Speed 18, CW_Mem_Text_WPM 15, CW_Paddle_Farnsworth 1 (backup cw.ini.bak-20261105).
  How we got there: bit 7 of 0x76 was dropped (the WPF client clamps the 0x76 value to 60 and
  sends it back at connect); the first try, stretching LETTERSPACE in the old keyer
  (`worktrees/keyer`, d40a7ba), worked but felt "wonky" (holds your next letter back). The new
  keyer (`worktrees/keyer-buffer`) rebuilt the PIC timing on a 1 ms TMR2 tick, state machines,
  real-ms element times (the old loop timing was off; 18 now = true 18 wpm).
  **PIC reset kink FIXED 2026-11-10 (pill 3.14):** the pill now passes every 0x9D on (was: only
  changes), so after a PIC reset one mscc restart puts Farnsworth back. Tested (Ron).
  **2026-11-10 keyer streamed** (keyer-buffer 79831ce, in the rig): a held paddle starts at once
  (was ~2 s wait, 16-element chunks). Pi cw.ini = `~/.local/mscc/cw.ini`.
  Not done: Windows ms-sdr (Stew's) and PSoC have no 0x9D; ms-sdr only logs a rejected 0x9D.
  Idea, not started: keyboard CW (client text into the player).
- **Stand-alone keyer (Ron 2026-11-05, started; `worktrees/keyer-standalone`, pushed c46d5f2).**
  The PIC keyer as its own 9 V box (from Ron's Rev 2 schematic, N8VET): modern rigs only (VO1400
  key out, isolated), PTT / amp output fitted, rig audio pass-through kept (RX path normally
  closed CPC1117: no current), LM386 on switched 5 V, MCP1703 LDO, AO3401A reverse protection,
  speed knob 5-60 wpm, FN + paddle menu, 1590B-size aluminium box. Draft 1: spec, netlist, BOM
  (LCSC numbers), reference drawing, Eagle schematic in Rev 2 style (Ron imported it into
  EasyEDA: "much better"). **Next:** (1) confirm the (check) items; (2) LCSC footprints in
  EasyEDA, then send the exported .net for a check against NETLIST.md; (3) firmware: stand-alone
  build of keyer-buffer (menu, speed pot, battery check, sleep), PC bench first.
- **Opcodes: free and reclaimed (2026-11-05).** Survey of every layer (Pi + Windows ms-sdr,
  recv, trans, WPF client, pill pio-async, PSoC MKII, PIC keyer). Free everywhere: **0x2F,
  0x9F** (0x9D taken 2026-11-05 by SET_PADDLE_FARNSWORTH). Reclaimed in the Pi ms-sdr `source/usbavrcmd.h` (26 never-used defines commented out,
  marked `reclaimed 2026-11-05`; AVAILABLE OPCODES block at the top of the file): 0x15-0x1E,
  0x5C, 0x83, 0x84, 0xAD, 0xC0, 0xC1, 0xC2, 0xC4, 0xF8; also unused 0xD6. Leftover unused names
  elsewhere: 0xAD client `Opcodes.cs`, 0xC0 pill / PSoC `usbvend.h`, 0xC1 / 0xC2 / 0xC4 / 0xD6
  sdrcore-trans `commands.h`; clear before reuse. More room: 0x0B extended command sub-byte (in use
  0x09-0x0D, 0x0F, 0x30). Header committed c01f487 (Pi hold: Ron asked for it), built into
  ms-sdr 3.192. Windows ms-sdr
  (Stew's) not changed. Pi vs Windows differences seen: 0x0F SWR fault and 0x2E SWR meter reset
  (Pi ms-sdr only), 0x0B and 0x30 (Pi recv / trans only), 0x85 / 0x86 panadapter gain / base
  (Pi recv only).
- **AM receive spectrum is 140 Hz low (Ron 2026-11-03; do when the Pi hold lifts).** On WWV
  20 MHz in AM the carrier sits ~6 px left of the spectrum center line at 4x zoom (measured
  157 +-15 Hz from a PNG; USB, LSB and CW are on center). Cause: ms-sdr sets the AM receive LO
  with `AM_OFFSET_RX (-11860)`; every other mode, and AM transmit, uses -12000, and sdrcore-recv
  always mixes down by exactly 12000 Hz. Likely reason Ron set 11860 years ago (he doesn't
  remember; this is read from the code): the old AM filter was the SSB bandpass (Lo..Hi), which
  blocks 0 Hz, so the carrier had to sit 140 Hz off. The sdrcore-recv 3.147 AM fix (c9850ca)
  made AM a lowpass (0..Hi), so 11860 is no longer needed. Fix: one line,
  `AM_OFFSET_RX (-11860)` -> `(-12000)`, in `rpi/ms-sdr-linux/source/extern.h:58` (Pi) and
  `mscc-ui/windows-work-tree/ms-sdr-MKII/source/extern.h:47` (Windows ms-sdr; tell Stew). Don't
  change it in the old trees (`linux/`, `Solidus/servers/`): their recv still uses the bandpass
  for AM and would lose the carrier. Leave the PSoC `AM_OFFSET 11860` (`si5351.c`) alone unless
  the PSoC turns out to set the LO in AM. Test: WWV in AM, carrier on the center line, audio
  sounds the same.
- **Waiting on Stew (notes in `.mscc-coord/`):**
  1. Port the new NB to Windows recv; client questions (Pulse min 50 us, threshold Low/Med/High
     = 1/500/750, slider 1001..1009 oddity, send NB state on connect).
     `NOTE-FOR-STEW-RECV-NB-AUTOMATIC-2026-11-01.md`
  2. NR control as Off/Low/Med/High = 0/15/50/85 (client only).
     `NOTE-FOR-STEW-NR-LOW-MED-HIGH-2026-11-01.md`
  3. Port the spectral NR to Windows recv. `NOTE-FOR-STEW-RECV-NR-SPECTRAL-2026-10-31.md`
  4. SSB voice + ALC meter on the air, splatter check at the 0.89 ceiling; one ALC ceiling for
     voice and digital (`NOTE-FOR-STEW-ALC-ONE-CEILING-2026-10-31.md`).
  5. Windows Operator's Guide Release 1 draft: Appendix C answers (firmware table, cal survives
     FW update, ALC target, Windows NR, reset-to-factory, AMP CAL target), pictures, guide source.
  6. TX IQ RESET ALL / Reset configuration don't stick (radio values win at next start).
  7. Heat sink temperature 0x06 in the client; Pi-side ports noted in his CLAUDE.md; new
     scripts into utility-scripts.
  8. Review the Windows parts of `MSCC-Configurations-DRAFT.docx` (setups 1, 2) and
     `MSCC-Operators-Guide-Remote-Windows-Release-1-DRAFT.docx` (7 draft items; key: how a Windows
     host keeps servers running with the one-client lock). Part 3 of the same note: Ron's rules
     (2026-11-01) MSCC UI = current Windows client (NB Thr 1..100 vs 1..1009 scale!, ALC switch,
     Remote window), Ubuntu scripts/utilities = Pi (mscc-init 1.0.18 + text wizard, mscc-volume,
     apt kit order). `NOTE-FOR-STEW-USER-DOCUMENTS-2026-11-01.md`
  9. ON HOLD (Ron 2026-11-03, note committed locally, NOT pushed; do not push until Ron says): PSoC non-QSK CW top speed (Ron 2026-11-03): check on his analyzer with dits at 45 and 55 wpm
     whether the spaces go fully quiet. Why: the PCM3060 soft mute (all current PSoC trees) ramps
     0.5 dB per 8/fs = 16.75 ms full <-> mute at 96 kHz (TI SLAS533B p32/34); unmute starts 14 ms
     before each delayed key-down. Worked out, not measured: full mute in every space up to 39 wpm;
     ~-40 dB at 58 wpm; merge above ~86 wpm. Unknown: can the mute bit reverse mid-ramp? (datasheet:
     attenuation commands wait for the running change.) If not, elements above 39 wpm start late.
     Wording for users until then: "non-QSK CW clean up to at least 39 wpm". QSK is not shaped.
     Note: `NOTE-FOR-STEW-PSOC-CW-TOP-SPEED-2026-11-03.md`.
  10. WiFi SWR meter firmware: our changes (tune allowance, mDNS, WiFi retry, PEAK hold, AVG,
     PEAK SWR on the screen, JSON keys; REF table tops at 25 W, left to him), his call whether to take them into his copy.
     `NOTE-FOR-STEW-SWR-METER-FIRMWARE-CHANGES-2026-11-06.md` (pushed bb31561)
  11. Windows ms-sdr stops all servers ~45 s after a remote client leaves (one line,
     `servers_failed = 0;` in Flusher_thread case 1, as on the Pi), his option; and check the RF
     in Remote Digital from the Pi client. `NOTE-FOR-STEW-WINDOWS-MSSDR-CLIENT-GONE-2026-11-06.md`
     (committed 2026-11-06, push held with the PSoC commits).
  12. Offer: an x86_64 (amd64) package of Ron's Avalonia copy for Ubuntu / Debian; Claude adds an
     -Arch amd64 choice to the build script, builds it, README steps. Only if he wants it.
     `NOTE-FOR-STEW-AVALONIA-X86-BUILD-2026-11-06.md`
  13. Windows ms-sdr has no WiFi SWR meter support: no swr_wifi_meter.c, and meter.c is not in
     ms-sdr.vcxproj (no Meter_main thread), so a Windows host sends no SWR / FWD / REF, no 0x0F
     fault and no 0x2E reset. Option: port the Pi's swr_wifi_meter.c. His call.
     `NOTE-FOR-STEW-WINDOWS-SWR-METER-2026-11-06.md`
  14. Windows ms-sdr reads the radio's power / TX IQ / RX IQ only if the network is up within
     5 s of startup; a late network means the radio is never read that run (client connect runs
     Apply_Appliance_Startup only). Pi ms-sdr 3.194 retries once at client connect
     (Radio_cal_startup_load). Option: same change on Windows. His call.
     `NOTE-FOR-STEW-RADIO-CAL-READ-LATE-NETWORK-2026-11-06.md`
- **Pi ms-sdr 3.194 (2026-11-06): radio calibration read retried at client connect, BUILT on the Pi
  18:10 (~/mscc/ms-sdr), committed and pushed. Checked in the log 2026-11-08: PASSED** (one read at
  startup 04:59: power 10 bands same, RX IQ 12 + TX IQ 10 from radio, no errors; the two later
  client connects sent status only, no second read). New `Radio_cal_startup_load(who)` in
  power_calibration.c (mutex + once-per-run flag, needs G_network_initialized): reload replaced
  cal files, Radio_cal_load_power, Radio_cal_load_iq. Called from main.c at startup (replaces
  the three calls) and in main-controller.c at client connect before Apply_Appliance_Startup.
  Log line: `Radio_cal_startup_load . startup|client connect: reading the radio's calibration`.
- **Pi ms-sdr 3.195 (2026-11-09, Ron: the one exception to the hold): pill counters in the
  ms-sdr log. BUILT on the Pi 13:03, WORKS (Ron's mscc.ini has `PILL_STATS=1;`).** mscc.ini
  `PILL_STATS=1` (default off) -> the temperature thread (every 3 s, STM32 radio only) reads radio
  command 0xC9 (`RADIO_CMD_AUDIO_STATS`, read only, value 0; client 0xC9 = auto snap, no
  conflict) and logs `Pill_stats. 10.0 min normal running, pill up .. h: in .. empty .. gaps ..
  (max .. ms) in_drop .. out_under .. out_drop .. ep0_cut ..` (`<<<` on trouble, PILL RESTARTED
  line). `Audio_stats_busy(why)` restarts the 10 min: SetHWLO_queue when the frequency changes
  (TX on / off re-sends the same one; RIT / split keying does count), ModeChanged (incl. TUNE),
  Session_Claim, Stop_all; new window only after 30 s quiet. Files: proficio-temperature.c,
  extern.h, main.c, main-controller.c (Parse_mscc_record), version.h 195. First lines 13:14 and
  13:24 clean. Ron (2026-11-09): gaps etc. during connect / band change / tune don't matter, only
  normal running. Cron log still on. Docs: utility-scripts/Pi/README.md (audio-stats section).
- **Pill audio stats logged by cron (2026-11-08).** `audio-stats.py log` (utility-scripts Pi +
  Windows, same file; Pi copy in ~/utils) adds one line to `~/.local/mscc/audio-stats.log`: change
  since the last run, servers up / DOWN / restarted, PILL RESTARTED; trouble ends in `<<<`. State
  in `~/.audio-stats-log.json`. Ron's crontab: `*/10 * * * * /usr/bin/python3
  /home/ron/utils/audio-stats.py log`. Check: `grep '<<<' ~/.local/mscc/audio-stats.log`. First
  test line (since pill start 11-07 23:52, 17.6 h): gaps 103, in_drop 5, out_under 10. The mark
  from 16:58 is still set (since = clean so far). Question: do underruns come with server
  restarts or in normal running? The log answers it.
  **Answered by two 120 s watches (2026-11-08 17:43 / 17:47):** FT8 key-down / key-up = clean.
  One transmit underrun (out_lvl to 0) at EVERY band change and EVERY tune key-up (and one near a
  tune key-down); key-downs otherwise clean; no pill faults. Cause is sdrcore-trans, not the pill:
  `udp_thread.c:1394-1424` (CMD_SET_MAIN_MODE) in DIGITAL / R-DIGITAL audio mode always runs
  manage_stream(0) + manage_stream(1) = stop and reopen the stream, even when the mode is the same
  (the client sends mode U again after every band change). Tune: mode T reopens output-only I/Q
  just BEFORE TX on; key-up: TX off, then mode U reopens full duplex. The T/C <-> voice rebuild is
  needed (comment: idle aloop stalls full duplex); the same-mode one is not. No harm on the air
  (all while receiving / un-keyed), but it puts an underrun on the pill counters each time.
  **Fix idea (after the hold, Ron 2026-11-08: no change yet):** rebuild only when the stream kind
  changes (full duplex <-> output-only), skip when the mode stays the same. Windows trans
  (Stew's) not checked. For "the pill is solid": count these as host-side, expected.
  **Better fix idea (Ron 2026-11-08: why stop the audio at all?):** the reopen exists because
  a single full-duplex PortAudio stream (VirtualB.monitor in + Proficio I/Q out) stalls its
  output when the idle loopback input stalls in TUNE / CW. Ron's Pi runs trans in DUAL STREAM
  (log: `DUAL STREAM: in_dev=17 'VirtualB.monitor' @48000 | out_dev=1 ALSA @96000`, main.c
  520-607): mic and I/Q are separate streams, so an idle mic can't stall the I/Q. In dual mode:
  never reopen the I/Q stream (just ignore / stop the mic stream in T / C); keep the reopen only
  for the single full-duplex case. Removes the band-change and tune underruns completely. Not
  verified: whether the single-stream case still needs it.
  **Receive losses (gaps / in_drop), 2026-11-08 evening: cause still open, Ron: no FT8 impact,
  keep the cron log only (no long background watch).** Cron windows: 17:30-17:40 (FT8 + band
  change + tune) 3 gaps; 17:40-17:50 (FT8 + band changes + tunes) 9 gaps, 5 drops; 17:50-18:00
  (8 FT8 TX, no band change / tune) clean; 18:00-18:10 (idle, WSJT-X receiving) clean. Also 5
  gaps + 1 drop at ~17:29 (cause unknown). Neither 120 s watch (FT8 TX; band changes + tunes) saw
  any, so band change / tune does not cause them every time, if at all. Ruled out: recv (keeps its
  stream running through TX, band change = I/Q reload + spectrum pause only); kernel log (no USB /
  audio messages); Claude's ssh / log-search load (18:13 test: 30 s control and 30 s of 4750
  awk/grep passes over the server logs, counters unchanged: gaps 120, in_drop 11, out_under 22).
  Gaps / drops are below recv (Pi USB host late collecting IN), not the pill. Next: read each
  `<<<` cron line against the server logs (UTC there, local in the stats log) to see if they keep
  landing in band-change / tune windows. No client runs on the Pi (client is on Windows).
  **Pill code read (2026-11-08, pill pio-async, no change): the pill can't cause the gaps.**
  RX streaming is all in interrupts: I2S DMA ISR (`audio_i2s_rx_block`, 96-frame copy) fills the
  IN FIFO, the USB ISR queues the next packet (`tud_audio_tx_done_isr`, TinyUSB 0.21, which also
  counts the gaps from the USB frame number); both priority 1. Main loop (relays, Si5351 I2C,
  beat6) is pre-empted by them, so loop max 1.9 ms doesn't matter. `__disable_irq` sections are a
  few instructions. No path holds the USB ISR off ~1 ms. `gaps max 2` = single missed frames.
  **in_drop follows from the gaps:** each missed frame leaves +384 bytes in the IN FIFO, flow
  control (95/96/97 frames) drains ~4 bytes/ms, so ~5 missed frames close together (FIFO
  ~1170 of 3104) fill it and each further one drops a block (17:40-17:50: 9 gaps, 5 drops = a
  cluster). **Likely cause (NOT verified):** trans's stream reopen closes / reopens the Proficio
  ALSA playback = snd-usb-audio changes the OUT interface alt setting = the Pi's USB host
  reconfigures endpoints on the same device and can miss a few IN frames. Fits "only in band
  change / tune windows"; doesn't explain why the 17:47 watch over 4 reopens saw none. Proof:
  after the hold, the dual-stream fix (never reopen the I/Q stream) should remove these too.
  **Impact (Ron 2026-11-08: edge cases):** only band change and TUNE (trans reopen); no data is
  being received then. Frequency changes within a band can't cause it: trans has no frequency
  command at all; recv gets CMD_SET_MAIN_FREQ (`udp_thread.c:737`) only to pause the
  spectrum (short blank for small steps, longer for >= 100 kHz = Si5351 PLL reset), its audio
  stream is untouched. A gap alone loses nothing (pill sends the samples a frame later); only
  in_drop loses 1 ms of I/Q. Modes that would feel a drop: packet / VARA / ARDOP (frame resent),
  PSK31 / RTTY (maybe one character), DRM (short dropout); FT8 / WSPR / SSB / CW: none.
- **Ron 2026-11-08 22:20: the Black Pill (pill 3.13) is PRODUCTION READY.** Cron log, pill up
  22.5 h: 17:50-22:10 local clean (idle receive, WSJT on); 22:10-22:20 FT8 on 30 m (~10
  transmits) + 2 tunes: gaps 0, in_drop 0, out_under 4 = the 4 trans reopens (T/U mode commands),
  host-side as above. No pill fault counted all day (ep0_cut, abandoned, slow, out_drop, resets,
  hang record all 0). Left: the trans dual-stream no-reopen fix (after the hold). Cron log stays on.
- **Pi ms-sdr Last_used.ini is dead (found 2026-11-08; Ron: leave it alone for now).** Every
  startup logs `Init_last_used_VFO_A/B . Open file failed. Last_used.ini will be created`; it never
  is. `Update_last_used_VFO_A/B` (`last_used.c`) never reaches its write (no "Open" line in the
  log, so one of its conditions always skips it), and `~/.local/mscc/vfoa/` and `vfob/` don't
  exist anyway (ms-sdr doesn't create them). The in-memory table still fills on band changes,
  but nobody asks for it: the WPF client keeps its own per-band last-used
  (`SaveLastUsedForCurrentBand`) and sent no 0xD7 / 0xD8 / 0xD9 request all day. `startup.ini`
  (same thread, `Update_Startup`) IS used and written. Option for later (after the hold): drop
  the Init calls and the Last_used.ini write, keep startup.ini. Heat sink -40 C / OPEN in the
  log = thermistor removed (Ron), expected.
- **Setup 3 (Ron's station), Configurations draft item: ANSWERED 2026-11-03, draft updated (DRAFT box + Appendix A item 2 removed).**
  Read on the Pi: ms-sdr CAT /dev/tnt0, WSJT-X /dev/tnt1 (pair), Rig Kenwood TS-2000, PTT RTS on tnt1;
  MSCC plays VirtualA -> WSJT-X in VirtualA.monitor; WSJT-X out VirtualB -> MSCC VirtualB.monitor; Mono.
  Ron: nothing set in the Windows client for COM ports or audio; mscc-init on the Pi sets them, and the
  Phones devices too. Unknown: VirtualA_TX / VirtualB_TX exist on the Pi, unused by these settings.
- **Rules (Ron 2026-11-01):** assume Stew makes MSCC UI (Avalonia) match the current Windows client,
  and the Ubuntu scripts/utilities match the Pi. No separate Avalonia review; document the Pi
  versions for both Linux hosts. Linux guide Release 0 reviewed (old versions, Appendix G Pi install
  wrong for today's kit, cal now in the radio).
- **2026-11-01: Linux guide rewritten: `docs/manuals/MSCC-Operators-Guide-Linux-Release-1-DRAFT.docx`**
  (39 pages, Word, Pi + Ubuntu as one, Pi kit = standard, MSCC UI = Windows client). Built from
  scratchpad build_linux.py (session-only; edit the .docx from now on). Release 0 WSJT-X audio was
  backwards: correct = Input VirtualA.monitor, Output VirtualB (also fixed in Configurations setup 4).
  11 draft items (Appendix C). In the note for Stew (Linux guide section).
- **Next:** a Pi / Linux remote guide, when Ron says.
- **Remote audio summary for Stew (2026-11-05, pushed 6c8db8a): `mscc-remote-audio/REMOTE-AUDIO-WHAT-WE-KNOW.md`.**
  Plain English (Ron: the first version "was written for a machine"). Why: Stew was being led to
  redo remote audio; Ron: Pi host -> WPF Windows client works fine, same server code on every host.
  Stew mixes up which way round (host = has the radio + servers, client = where you sit); the file
  asks him to name both. Ron's device model (corrected me): VirtualA / VirtualB belong to the
  SERVERS (local Digital, WSJT-X on the same computer), not the remote client. Remote client:
  Remote Phones = real speaker + mic; Remote Digital = client <-> virtual sound device <-> WSJT-X
  (Stew: VB-Audio; Ron: two USB adapters back to back). Linux client: PipeWire null sinks
  `MSCC_Remote_RX` (client plays, WSJT-X input = MSCC_Remote_RX.monitor) and `MSCC_Remote_TX`
  (WSJT-X output, client input = MSCC_Remote_TX.monitor), made at login by Ron's mscc-avalonia
  since 1.6.84 (2026-11-06, see the Avalonia item below). The first proposal (`mscc_remote_rx`
  "MSCC Remote RX") crashed PortAudio: name and description must be the same. Open (Stew's
  mscc-ui, fixed in Ron's copy 1.6.80): Avalonia doesn't save the Remote device choices or
  Phones (reverts to one shared device; maybe the spectrum noise, a guess). Stew asked to add his VB-Audio setup to the file. Still to correct (proposed, not
  approved): the 11-01 user-documents note and the Remote guide say "Pi / Linux remote works
  differently"; the remote-audio part is the same on every host.
- **Ron's Avalonia client copy (2026-11-05/06, pushed 24d8738 = 1.6.79): `worktrees/Avalonia`, Pi
  package `mscc-avalonia` 1.6.89 installed on the Pi (2026-11-07).** Copy of Stew's
  `mscc-ui/Avalonia-Migration` 0.6.75 with its own `src/MSCC.Core` copy; never touch Stew's WPF /
  Avalonia trees (building must not write there either). Version 1.x (Ron) vs Stew's 0.6.x.
  Installs beside Stew's mscc-ui 0.6.31: /opt/mscc-avalonia, /usr/bin/mscc-avalonia, menu
  "MSCC Avalonia". Build `Avalonia/build-mscc-avalonia-deb.ps1` (bump Version in the csproj and
  `packaging/mscc-avalonia/DEBIAN/control`), deb to installers/rpi + Pi ~; publish/ and *.deb not
  committed. Own settings `~/.config/MSCC-Avalonia/mscc-avalonia.ini` (seeded once from the shared
  file); reads only a copy of `~/.local/mscc/*.ini` the launcher makes at each start
  (`~/.config/MSCC-Avalonia/host/`); never writes mscc.ini (on the Pi it is ~/.local/mscc/mscc.ini).
  Still shared with Stew's client: favorites and last-used files (Ron thinking about it).
  Ported from WPF 11.1.5: ALC button removed, 2 TONE, TX IQ PHASE, cuts at connect, QRP save step +
  0x2D lamps, SAVE / APPLY ALL labels, FREQ CAL note + 4x, SMOOTHING + Falling waterfall, SWR meter
  (FWD face, fault RESET, settings), TIME DISPLAY + TIME MARKER, PEAK MARKER, dB LABELS, AUTO SNAP,
  BASELINE / WF GAIN / WF ZERO per band. Also: window size / position remembered, own resize grips
  (the Pi's Openbox runs ~/.config/openbox/rpd-rc.xml = theme only, no side handles).
  Not ported: Reset configuration, identity request, RF power report. **Not tested on the air yet**
  (2 TONE, PHASE, QRP save, SWR meter, smoothing).
  **2026-11-06, Remote Digital on the air (1.6.80 to 1.6.86): Windows host (servers on Ron's PC),
  Avalonia + WSJT-X on the Pi, two FT8 QSOs.** Stew to check the RF (Ron can't see it).
  1.6.80 Remote devices saved by name ("Name|HostApi"), Phones and Digital separately
  (REMOTE_PLAY_NAME, REMOTE_MIC_NAME, REMOTE_DIGI_PLAY_NAME, REMOTE_DIGI_MIC_NAME); before, Digital
  always went back to VirtualA / VirtualB.monitor and Phones kept a PortAudio index that shifts.
  1.6.81 Remote CAT watches the line in comm-port.ini PIN= (1 = CTS = WSJT-X RTS, 2 = DCD = DTR)
  every 10 ms and keys like CAT TX; (ms-sdr Pin_Check_Thread does the same locally). WSJT-X PTT =
  RTS or CAT both work. Linux raises RTS when a port opens: watch for a short key at WSJT-X start.
  1.6.82 Digital default mic = MSCC_Remote_TX.monitor (PortAudio lists the internal name, not
  "Monitor of ..."); Remote window 520x720 with the log always visible; closing the main window
  closes the Remote window and exits (ShutdownMode OnMainWindowClose).
  1.6.83 / 1.6.84 installer: user service `mscc-avalonia-remote-audio.service` (systemctl --global
  enable) runs /usr/share/mscc-avalonia/bin/mscc-avalonia-remote-audio.sh (pactl null sinks,
  name = description, 48 kHz, leaves existing ones alone). 1.6.83 also put ALSA names in
  /etc/alsa/conf.d; the Pi has no pipewire-alsa, so they were broken and a same-named ALSA
  MSCC_Remote_TX hid the real one from WSJT-X. Removed in 1.6.84 (postinst deletes the file).
  1.6.85 Remote Phones / Digital choice saved (REMOTE_DIGITAL). 1.6.86 title bar version from the
  build (was a hard-coded 0.6.75). 1.6.87 (2026-11-07, works, Ron): keep-alive lost pop-up as WPF
  ("MSCC - Server Not Responding", Yes = wait again, No = close MSCC, X = Yes); before, only status text. 1.6.88 (2026-11-07, Ron: "Much better"): spectrum draws every frame
  (SpectrumRefreshDivisor 3 -> 1); at 3200 bins ~12 frames/s arrive (99 pkts/s, 8 per frame), it drew ~4/s. 1.6.89: spectrum banner
  ("Center ... MHz | bins | zoom | click to tune", from Stew's Avalonia, not in WPF) removed: covered the pass band.
  Learned: Avalonia's Remote Digital needs PTT from WSJT-X (CAT or RTS); there is no VOX. WSJT-X on
  the Pi (Qt5, PulseAudio plugin): Input MSCC_Remote_RX.monitor, Output MSCC_Remote_TX, Rig
  TS-2000, Serial /dev/ttyUSB11 (= tnt1; Avalonia holds tnt0). The pair comes back by itself
  after a Pi reboot (Ron checked). Windows ms-sdr stops all servers ~45 s after the
  client leaves (Flusher_thread "Client FAILED" falls through to Stop_all; the Pi sets
  servers_failed = 0). Start the Windows servers again before each connect. Note for Stew (list item 11):
  `.mscc-coord/NOTE-FOR-STEW-WINDOWS-MSSDR-CLIENT-GONE-2026-11-06.md`. Pi ms-sdr PIN=2
  (DCD) never keys: it checks fDCD, which is never set (only fRLSD is read).
- **2026-11-01: Pi ms-sdr 3.187: WiFi SWR meter fault to the client, BUILT on the Pi 15:35 (~/mscc/ms-sdr),
  NOT committed, running only after mscc stop/start.** Meter is UDP unicast: at a Pi station it talks
  to ms-sdr only, which relayed SWR/FWD/REF but never the fault (client got Fault=false; the 0x30
  "SWR METER FAULT" text is dropped by the client). New extended sub 0x0F CMD_SET_SWR_FAULT:
  [fault][online][meter IPv4 4 bytes], sent at once on a fault change, with each update (<= 2/s),
  and offline; not gated by SWR_METER_TO_GUI. Files: swr_wifi_meter.c, usbavrcmd.h, version.h (187,
  synced back from the Pi). Client side = Stew: `.mscc-coord/NOTE-FOR-STEW-SWR-FAULT-TO-CLIENT-2026-11-01.md`.
  Windows ms-sdr has no meter code (client listens directly there).
- **2026-11-01 17:00: Pi ms-sdr 3.188 + trans 3.150 BUILT and RUNNING on the Pi (Ron restarted, old client 10.31.x, no calibration until 11.1.0;
  committed): port of Stew's Windows round 1 (ms-sdr 3.189 / trans 3.154 / client 11.1.0, 49d8c90).**
  0xA2 slider = live only; 0xA2 | 0x8000 (POWER_CAL_SAVE_FLAG) = save: ms-sdr writes the radio, trans
  writes power_cal.ini (trans reads the flag from buf[2] bit 7; value stays buf[1]). Startup copy from
  the radio to trans sent WITH the flag. New 0x2D CMD_SET_CAL_STATUS ms-sdr -> client: int32 kind (1 RX
  IQ, 2 TX IQ, 3 power) | flags<<8 (bit0 store) | mask<<16; sent at Session_Claim, end of
  Radio_cal_load_iq, after each radio write (masks reread). Includes the 3.187 SWR fault (0x0F).
  **Needs client 11.1.0** (old client never sends the save flag = QRP cal not saved). New trans with an
  old ms-sdr never saves power_cal.ini (Stew's trans comment says otherwise; code saves on flag only).
  Files: ms-sdr power_calibration.c, main-controller.c, usbavrcmd.h, extern.h, swr_wifi_meter.c,
  version.h (188); trans udp_thread.c, extern.h (150).
- **2026-11-01 19:41: Pi trans 3.151 (2 TONE, port of Stew's Windows 3.155 on 0x88) + ms-sdr 3.189 (0x2E
  CMD_SWR_METER_RESET: client RESET -> HTTP reset to the meter in a detached thread) BUILT on the Pi, NOT
  running until mscc stop/start, NOT committed.** Stew pulled 11-01: client 11.1.5 (0x0F handled, installer
  R11-1-5), Windows trans 3.157 (2 TONE + voice ALC 0.89; 0.45 was a bad StarTech USB card). Note:
  `.mscc-coord/NOTE-FOR-STEW-2TONE-PI-AND-0x2E-2026-11-01.md`. Running since 19:46. **Fault test PASSED 19:51
  (Ron, client 11.1.5):** meter fault latched -> client showed fault + RESET, Ron pressed the RESET
  button ON THE METER ITSELF (corrected 2026-11-03; this line used to say the client's RESET) -> meter
  cleared, ms-sdr relayed it. The client's own RESET was first tested 2026-11-03 (client HTTP straight
  to the meter, after fixing a typo in MSCC_Client.ini SWR_HF_IP=192.156.12.222 -> 192.168.12.222): worked.
  Not tested yet: client RESET from a remote client (Tailscale) - the client sends the reset straight
  to the meter's LAN IP, and doesn't use ms-sdr's 0x2E reset yet. **2 TONE PASSED 20:12 (Ron, 20 m USB, full power; button is on the RX/TX tab;
  panadapter stops in TX so tones/IMD not seen).** Note: `NOTE-FOR-STEW-2TONE-PI-TESTED-2026-11-01.md`.
  Open: 0x2E RESET path (needs Stew's client send).
- **SWR meter firmware (Ron 2026-11-01):** auto-tune on 30 m runs longer than the meter's time limit, so
  the meter faults. Firmware source is NOT in the tree (only swr-meter/ Python app + summary); Ron asks
  Stew to add it. 11-01 evening tests: trips ~1 s into a 15 m auto-tune (tune power 43: fwd 61 ref 25.00
  swr 4.55; 34: fwd 38 ref 25.00 swr 9.82); 30 m tunes 5.5-7.4 s did not trip. Ron: meter "Fault Trip
  Count" is already at max and is too short for a slow tuner match (firmware limit).
  Stew added the source 11-01 (67fa644, swr-meter/HF_SWR_R4_1). Cause: 1 count = one FWD+REF pair ~2.4 ms,
  max 24 = ~58 ms; REF table tops at 25.00 W (clipped). **FIXED in our copy swr-meter/ron/HF_SWR_R4_1
  (git-excluded via .git/info/exclude until done): TUNE ALLOWANCE** (tuneGraceSec 0..10 s, default 5;
  SWR not counted for the first N s of a new TX = RF after >= 2 s with none; web field + prefs + saved list;
  no touch control). Built with arduino-cli (IDE bundled; esp32 3.3.11, lvgl 9.4.0, TFT_eSPI 2.5.43, Stew's
  User_Setup.h/lv_conf.h copied into the libs) -> swr-meter/ron/build/; Ron OTA-loaded it (http://meter/update)
  20:43. Test PASSED 20:45 (Ron): 20/15/30/15 m tunes, no fault, client showed readings.
  **21:00 allowance now ONLY FOR TUNES (Ron):** meter /api?tune=1|0 flag (RAM, self-clears after
  tuneGraceSec; JSON "tune"); 2 s idle rule removed. **Pi ms-sdr 3.191** (running): Tune_button sends
  tune=1 (waits <= 300 ms for the answer) BEFORE keying, tune=0 after (thread); skipped if meter
  off/offline. Also reset fix: ms-sdr asked /reset (no such page, 404 taken as OK) -> now only
  /api?action=reset, result read from the answer ("fault":0). Test PASSED 21:06 (Ron): 15 m tune 16 s,
  tune=1 OK ~20 ms before key, no fault. Old firmware ignores tune=1 (logged "no tune flag"), harmless.
  ms-sdr committed. Firmware committed 2026-11-04 (Ron; no longer git-excluded), with mDNS added (meter
  answers as hf-swr-meter.local; tested from Firefox, Ron). Note for Stew about the firmware changes: `.mscc-coord/NOTE-FOR-STEW-SWR-METER-FIRMWARE-CHANGES-2026-11-06.md` (2026-11-06; tune allowance, mDNS, WiFi retry, PEAK hold, AVG, JSON keys, 25 W REF table limit).
  21:18: touch control "Tune Allow (s)" on the meter SETTINGS page (rows respaced to 52 px:
  52/104/156/208/260; -/+ 0..10, 0 = OFF, saved on Back). Ron: fits and works. Client 11.1.5 dropped
  TX 2 ms after the relayed fault (good). Then find the limit and fix. Also pending Ron's decision: should ms-sdr itself stop
  TX on a meter fault (backstop; WSJT-X on the Pi keys via CAT, so the client can't stop it).
  **2026-11-04: WiFi join retry** (012852c): after the 15 s timeout the meter retries every 30 s
  (NO NET-RETRY / JOIN FAIL-RETRY / NO WIFI-RETRY). Loaded, works; the retry itself (router off,
  meter restarted) NOT tested yet.
  **2026-11-06: PEAK hold + AVG mode, TESTED (Ron).** Old peak was not a hold: fixed 600 ms (comment
  said 5 s), hold restarted only on a reading >= the peak, decay x0.95/10 ms floored at the live
  reading (looked live), REF had no timer. Now: FWD and REF each have a hold timer; a reading within
  10% of the peak restarts it; hold = peakHoldSec (0.5..5 s, default 2, web field "Peak Hold (s)",
  pref "peakHold"; no touch control, Ron); then x0.97/20 ms, no floor. The PEAK/VOLTS button now
  steps VOLTS -> PEAK -> AVG (cyan / amber / green; label VOLT: / PEAK: / AVG:); pref "auxMode"
  (old "peakMode" read once). AVG = FWD/REF watts over a 1 s window (50 x 20 ms slots, pauses
  counted); fresh window when RF starts after > hold with none; shown only once the window is full
  (first 1 s of a TX shows the held value or 0, Ron: steadier for WSJT-X); AVG label redraw <= 4/s;
  held for peakHoldSec after RF stops, then 0. JSON adds "peakRef", "peakHold", "avg", "avgRef".
  Firmware ~1508 KB, bin in swr-meter/ron/build/.
  **2026-11-06 later: PEAK SWR on the main screen, WORKS (Ron).** "SWR:" moved x 70 -> 5, value
  270 -> 195 (64 px font kept; "10.00" ends ~x 370); new "PEAK" caption (montserrat 18, 385,44) and
  value (montserrat 32, 385,66), always shown, amber / red over the alarm threshold. peakSWR follows
  the averaged swr (the big number), only while FWD >= MIN_POWER_THRESHOLD; hold = peakHoldSec,
  restarted by a reading whose (SWR - 1) is within 10% of the peak's; after the hold it drops to the
  live SWR (1.00 unkeyed), no decay. NOT in the JSON / web page (Ron). Cross-needle mode unchanged.
  Firmware ~1509 KB.
- **Parked:** NR warble fix (Ron 2026-11-01: leave it, NR works well; lower level = less
  warble). Sync AM (off the list). Complex Goertzel for FREQ CAL and FM transmit (future ideas,
  see "Ron confirmed" below). CW ramp length by speed (Ron 2026-11-02, just a thought): the key
  delay (20-40 ms) already shows each element's length ahead, so the ramp could be chosen per
  element (longer normally, shorter only for very short elements/spaces), any key, no speed
  needed. Pill: easy (table + step count), only if 5 ms proves a problem on the analyzer. PSoC:
  only after the own-I/Q fix (doesn't fix the staircase).
  Pill TX underrun while keyed (2026-11-02): `audio_i2s_tx_block()` plays a silent block at once
  on a USB underrun (hard drop = click). TESTED 11-02 with `~/utils/audio-stats.py mark` / `since`:
  2.1 min CW non-QSK = out_under 0, out_drop 0, out level steady 1552; 1 underrun in 6.5 h before.
  Fix (repeat the last good block or ramp down while keyed) only if one ever shows up.

Old history (resume points before 2026-10-31 afternoon, dated change logs, closed items) is
in `rpi/CLAUDE-history.md`. Grep it for earlier work on a topic; don't read it whole.

**2026-10-31 (16:39-16:43 local): LSB filter fix CONFIRMED on Ron's rig (client 10.31.1, Pi recv 3.150).** recv log: in LSB, USB and AM every LO CUT press arrived (75 / 100 / 200 / 300 / 500) and every HI CUT press (5500 / 2400 / 2700 / 3000 / 4000), each cut moving alone. Ron: LSB and USB on an AM station now sound the same. LSB / USB recheck CLOSED. Open: Ron's I/Q test (TX I/Q from CW, PHASE slider, APPLY; server code ms-sdr 3.185 / trans 3.147 / recv 3.150 not committed until then); SSB voice / ALC meter test.
**2026-10-31:** Ron will do the TX I/Q test later (needs setting up). Test: (1) CW mode, start TX I/Q calibration on a band: it must transmit; (2) move PHASE: phase changes; (3) APPLY; (4) Claude checks ms-sdr.log for `Radio_cal_store_iq ... queued` then `Radio_cal_send_iq ... sent`. Then commit ms-sdr 3.185 / trans 3.147 / recv 3.150.
**2026-10-31 (12:56 local): Pi ms-sdr 3.186 = power start order as Windows 3.185 (built, running, NOT committed, goes with the I/Q-in-radio port after Ron's TX I/Q test).** `Radio_cal_load_power`: bands the radio does not hold now KEEP the power_cal.ini value (before: factory value, which overwrote a calibrated file on the first start with a radio holding nothing). Radio values still win where held. `Factory_power_cal_values()` no longer called (left in factory_seed.c, as Stew did). Log on Ron's rig: 160-10 m `radio N, file N, same`, 630 / 2200 m `not in radio, file 0 kept`, 0 changed; I/Q 22 bands from radio; no errors. Stew's open item #3 (power start order) DONE on the Pi. Pi now: recv 3.150, trans 3.147, ms-sdr 3.186, pill 3.12.
**2026-10-31 (13:00-13:10 local): recv NR (noise reduction) found defective; Ron: replace it with a spectral-subtraction NR (option B), PARKED for now.** Ron: "the audio becomes really distorted" with NR on. Code: `denoise()` in `SDRcore-recv-linux/sources/dsputils.c` = LMS adaptive line enhancer at 96 kHz after the AGC (48 taps, 32-sample delay, mu 0.002, wet 0.35-1.0 by level, step normalised by ONE sample's power averaged over ~12 samples, no leakage, no output limit). Measured: Ron listens on Remote Phones (UDP to the PC, port 9100, 48 kHz mono s16 after a 16-byte "MSA1" header; local C-Media output is muted then, so recording its monitor gives silence). Captured with `sudo tcpdump -i any -w X.pcap "udp dst port 9100"` and decoded (scratchpad pcap2wav.py). Fair A/B on one rag chew, NR toggled during a 21 s capture (recv log times are UTC): with NR ON (level 50) a ~3.8 kHz whistle at the top of the 4 kHz filter (strongest component every second), >3 kHz energy from about -15 dB to -4 dB of total, peaks 0.33-0.37 vs 0.20 off (0.366 repeated = clipping), speech band 300-1000 Hz down up to 10 dB. Cause: the ALE boosts anything correlated over the delay; noise through recv's sharp audio filter is strongly correlated at the filter edge -> whistle; per-sample normalisation over-corrects at quiet moments -> overshoot / spikes. A Python copy of the algorithm on an upsampled NR-off capture did NOT reproduce it (the earlier NR-on / NR-off captures were at different moments; only the same-moment A/B shows it). Options: A = repair (NLMS over all taps, leakage, output never above input), B = replace with frequency-domain spectral subtraction (as most SDRs). **Ron chose B, parked.** When done: same change in the local copy (sync rule), Stew's Windows recv likely has the same NR, retest with the same A/B capture.
**2026-10-31 (13:15 local): recv NB (noise blanker) checked: WORKS, makes no ticks of its own.** Same Remote Phones A/B capture (21 s, NB toggled; switch times from recv log `CMD_GET_SET_NB_ENABLE`), ticks = samples far outside their 10 ms neighbourhood after a first difference (scratchpad ticks.py): NB off 3.8 ticks/s (biggest 18-415x local, usually 30-50x), NB on 1.1/s (12-22x), two seconds with none; ticks back at once each time NB went off (real band impulse noise, irregular spacing 14 ms to hundreds of ms); no clicks at the switch moments; threshold 0.00999 throughout (3.147 thread fix holds). Ron finds NB hard to adjust. Why (blanker.c): threshold is an ABSOLUTE amount above the running envelope (envelope + threshold), so the same setting means different things on quiet / noisy bands; the envelope attack / release is 0.05 ms, so it nearly follows the pulse itself; no feedback on what it does; width is in raw 96 kHz samples (30 = 0.3 ms). **PARKED (Ron): automatic NB.** recv only: running level + spread over ~50-100 ms, blank only short samples far outside the spread (~5-6x), self-protection (less sensitive if blanking more than ~100/s); NB = on/off for the user (threshold knob could become Low / Medium / High: Stew). Includes "relative threshold" and "slower envelope"; pulse blanking (gain ramp) unchanged. Optional "blanking meter" (recv sends blanks per second, client shows it) needs Stew. When done: same change in the other recv copy, retest with the tick capture.
**2026-10-31 (evening): SSB / WSJT output power low (about 40 W of 100), fixed in trans 3.149 (BUILT on the Pi over ssh, NOT committed).** Cause: the 3.146 ALC ceiling 0.45 x drive (from Stew's Windows 3.149) held peaks to 0.45 / 0.8 (TUNE level) = about 32% of TUNE power. `ALC_CEILING` moved to `extern.h` (udp_thread.c CMD_SET_ALC_MULTIPLIER uses it, was a hard 0.45). 3.148 = 0.80: WSJT tone 89 W vs TUNE 109 W (ATU-100); 3.149 = 0.89: 95 W (PA near compression). Ron then recalibrated power (QRP and QRO): output power now right. Open (Stew, Ron 2026-10-31: no SSB voice test by Ron): SSB voice + splatter check on his analyzer with the 0.89 ceiling (ask, worded as a question).
**2026-10-31 (22:30): TX I/Q test PASSED (Ron).** CW, PHASE slider 11 -> 1 (TX), APPLY: ms-sdr.log (~/.local/mscc/ms-sdr.log) `Radio_cal_store_iq . TX band 4: offset -26, phase 1 (queued)`, 10 s later `Radio_cal_send_iq ... sent (radio stores it)`, client read back phase 1, no errors. Server code committed (ms-sdr 3.186, trans 3.149, recv 3.150), not pushed. Ron's open list is now empty; SSB voice + splatter check is Stew's.
**2026-10-31 (late): one ALC ceiling for voice and digital (Ron).** Stew's Windows trans 3.152 (e7e4056) uses 1.0 x drive for digital, 0.45 for voice. Ron: WSJT and voice are both just audio feeding the rig; the ALC only stops overdrive, so the ceiling = full calibrated power and mic gain sets the drive. Pi stays on 3.149 (0.89 for both; a 3.150 port of Stew's split was made and undone, never built). Note for Stew: `.mscc-coord/NOTE-FOR-STEW-ALC-ONE-CEILING-2026-10-31.md` (question + options, asks for a 0.89 voice check on his Signal Hound).
**2026-10-31 (late night): NR replaced (option B), recv 3.151, BUILT on the Pi over ssh 23:16 (~/mscc/sdrcore-recv, no warnings), NOT COMMITTED. ssh from git-bash can't resolve `proficio`; PowerShell `ssh -i $HOME\.ssh\proficio_ed25519 ron@192.168.12.199` works.** `denoise()` in dsputils.c = spectral subtraction: 2048-pt FFT at 96 kHz (21 ms, 47 Hz bins), 50 % overlap, sqrt-Hann, per-bin noise = smoothed power min-tracking (rises 3 dB/s), Wiener gain with decision-directed SNR (anti musical noise), gain 0..1 only (never louder). Level 1..100: floor -6..-20 dB, oversubtraction 1..2. Passes audio unchanged for the first ~0.1 s after ON. Adds 21 ms delay while ON. `nr_delay()` removed. Offline test (scratchpad nrtest/t.c, white noise + 1 kHz tone 0.3 s on/off): noise-only -5.8 / -12.1 / -17.3 dB at level 1 / 50 / 100, tone kept (corr 0.98-1.00), output peak never above input. On-air A/B 23:25 local (40 m rag chew, level 50, NR toggled 3x in a 39 s Remote Phones capture, scratchpad nrtest/nrab.py): background between words -40.8 -> -53.6 dB (about -12.7 dB, as the offline test), talk level -29.6 -> -30.7 dB (about 1 dB less), talk-to-gap 11 -> 23 dB, >3 kHz share -13.6 -> -19 to -23 dB, peaks 0.07-0.10 on vs 0.11-0.20 off (no overshoot / clipping), no whistle (top tone 316-469 Hz = voice). Ron: "just a touch of warble, not bad ... much better than it was". 3.151 stays on the Pi. PARKED (Ron): warble fix = smooth the gain over neighbouring bins (+/-2 bins, ~+/-100 Hz), floor unchanged; alt: higher floor. Committed and pushed with note for Stew `.mscc-coord/NOTE-FOR-STEW-RECV-NR-SPECTRAL-2026-10-31.md` (Windows recv has the identical old ALE). Windows recv (mscc-ui/windows-work-tree/SDRcore-recv) has the same old ALE -> note for Stew after the on-air test. sdrcore-recv-DRM has a different NR, already at 3.151 (version clash only if both run; not touched).
**Resume point 2026-10-31 (night), the list:** Pi runs recv 3.150, trans 3.149 (ALC ceiling 0.89, voice and digital), ms-sdr 3.186, pill 3.12 (22 I/Q bands stored). All server code committed and pushed (1e3ecb4, note 836c33f); local `rpi/si5351-drive.md` edits not needed (Ron 2026-10-31: pill runs fine), left out. Ron's open list: empty (TX I/Q test passed, power fixed). Parked: (3) NR: DONE as recv 3.151 (spectral subtraction, on the Pi); warble tweak (gain smoothing across bins) parked; (4) automatic NB (blanking meter optional); (5) sync AM (off the list); (5a) Windows Operator's Guide rebuild: the 60-page PDF reads out of order and mixes terms; no editable source in the repo. Proposed order: what MSCC is + glossary, before you start, install and first start, operating, calibration as one chapter (APPLY stores I/Q in the radio), settings, digital modes, appendices. Open questions: Windows guide first? write it for the client as it is, with a note to Stew about confusing names? Stew: (2) SSB voice + ALC meter on the air, splatter check with a 0.89 ceiling; (2a) one ALC ceiling question (NOTE-FOR-STEW-ALC-ONE-CEILING-2026-10-31.md); (6) blanking meter / NB Low-Med-High display if wanted; (7) heat sink temperature 0x06 in the client; (8) Pi-side ports noted in his CLAUDE.md, new scripts into utility-scripts.

**2026-11-01: Windows Operator's Guide reviewed in full (high effort): `rpi/GUIDE-REVIEW-WINDOWS-2026-11-01.md`** (read it, not this line). Main points: cal now lives in the radio (QRP / TX IQ / RX IQ), so the Save settings text is wrong; TX IQ RESET ALL and Reset configuration don't stick (radio values win at next start: Stew code issue); TX IQ PHASE missing; RX IQ RESET ALL is now APPLY ALL; POTENTIA / QSK described wrongly (Ron's 9/29 text never applied); firmware table old; pictures have no number callouts; source (WeasyPrint HTML/MD) not in repo, ask Stew. Not committed.
**2026-11-01: new Windows guide written (Ron chose Word): `docs/manuals/MSCC-Operators-Guide-Windows-Release-1-DRAFT.docx`**, 43 pages, for client 10.31.2, reorganized as in the review (sections F), pictures reused from the Release 0 PDF, control lists keyed by the printed label (no number callouts needed). Gray DRAFT boxes + Appendix C list what to confirm (firmware table, cal survives FW update, ALC target, Windows NR, reset-to-factory, AMP CAL target) and pictures to retake. Built with python-docx from a light-markup text (scratchpad, session-only); from now on edit the .docx itself. TOC prefilled from a LibreOffice render; Word asks to update fields on open (Yes). Not committed.

**2026-11-01: automatic NB, recv 3.152, BUILT on the Pi (last build 08:53, copied to V:), committed and pushed with notes for Stew.** Files: `SDRcore-recv-linux/sources/blanker.c` (rewritten), `sdrcore.h` (blanker_state), `extern.h` (3.152). No client / ms-sdr change: NB button = on/off; threshold slider = sensitivity K 4 (slider max) .. 8 (slider min), default 6 (Ron's slider sat at min -> K 8); width as before. Pulse = |I/Q| > K x running mean (2 ms, pulses left out); a "pulse" longer than the width (max 2 ms) is a signal: un-marked before the gain falls (delay = width/2 + max pulse, 0.47 ms at width 30). Gain: 0 within +/- width/6 of any pulse sample, raised-cosine back to 1 at width/2 (no restart jump). Self-protection: >400 pulses/s raises K (up to 4x), <200/s lowers it. 50 ms learn after each (re)start. Log every 10 s while on: `Blanker . N pulses/s, K ...`. Offline test (scratchpad nbtest/t.c, WSL): noise only 0 false at K 6 / 8 (1.3/s at K 4); 30x pulses -27 dB at K 6; 40 dB SSB with instant start 0 false at K 6 (first version locked up: 60 % blanked; fixed). Next: build on the Pi, Ron listens, tick capture A/B (scratchpad ticks.py from 10-31 is gone; redo). On-air 08:30 (20 m USB, K 5.95, width 40, NB toggled): ticks >18x only with NB off (28-58x), NB on 0.8/s max 18x vs off 1.2-1.5/s; blanker 1-2 pulses/s; no holes. Slider test 08:34: every step restarted the blanker (ring zeroed = up to 4 ms silence at width 400): fixed 08:38, slider steps keep level and audio (K only = no change; width = ring resized, oldest dropped or oldest stretch repeated); log `Blanker . setting: K, width`. Offline 69 steps, 0 holes. Width now read as MICROSECONDS (client slider 10..510 is labelled uS; recv used samples = 10.4x longer, up to 5.3 ms); 200 us default; built 08:42. Threshold range unchanged (K 4..8, Ron OK). Note for Stew: Windows recv also reads width as samples. On-air 08:47 (200 us, K 6): big clicks (>25x) off 0.41/s, on 0.05/s (~9 of 10 removed); one 54x passed with NB on, cause unknown (too long >0.2 ms / busy band, mean 9x higher than 08:18 / not noise). Built 08:53: log counters per interval + summary when NB turned off: `N pulses/s (biggest Nx), too long N/s (biggest), near N/s (0.67K..K, biggest)`. Decide fix from data (maxrun fixed 1 ms if too long; linear-prediction detector if busy band). Ron to restart MSCC. On-air 08:55 (60 s NB on, 200 us, K 6): no audio tick >26x, blanked 1-2/s (biggest 50-78x), too long 0 in every interval (cause 1 ruled out, maxrun change not needed), near 0.3-3/s at 5.4-6.0x (option: threshold to ~5x). DRM copy (git-excluded, local only) ported 2026-11-01: blanker.c + sdrcore.h copied, extern.h 3.152, WSL syntax OK, NOT built on the Pi (V:\SDRcore-recv-DRM still has the old blanker.c / sdrcore.h / extern.h). Notes for Stew: `.mscc-coord/NOTE-FOR-STEW-RECV-NB-AUTOMATIC-2026-11-01.md` (port blanker.c + sdrcore.h; client questions: Pulse min 50 us, threshold Low/Med/High = 1/500/750, 1001..1009 oddity, send NB state on connect) and `.mscc-coord/NOTE-FOR-STEW-NR-LOW-MED-HIGH-2026-11-01.md` (NR slider -> Off/Low/Med/High = 0/15/50/85, client only). DRM BUILT on the Pi 09:08 (`~/mscc-drm/sdrcore-recv`, 3.152). CAUTION: DRM `make clean` without BINDIR deletes ~/mscc/sdrcore-recv (normal recv): happened 09:08, normal recv rebuilt from the same source at once; warning added to DRM-NOTES.md. Always `make clean BINDIR=$HOME/mscc-drm`. NR warble fix: Ron 2026-11-01 leave it (NR works well; lower level = less warble). Then port to `rpi/sdrcore-recv-DRM` (same old blanker.c) and a note for Stew (Windows recv, maybe Low/Med/High labels).

**2026-11-01: Remote guide rewritten (Word, same style as the Windows draft), WINDOWS ONLY (Ron: Windows client + Windows servers; Pi/Linux remote differs and needs its own guide, not written yet): `docs/manuals/MSCC-Operators-Guide-Remote-Windows-Release-1-DRAFT.docx`**, 20 pages, client 10.31.2, server names as Windows .exe; session lock and ports also checked in Windows ms-sdr-MKII / SDRcore-recv / SDRcore-trans; pictures from the Release 0 PDF. Corrections from code: Phones / Digital chosen inside the Remote Audio window (main-window Phones / Digital turn Remote off); Remote off at each start; one client at a time (ms-sdr session lock, freed by Stop or ~45 s); Remote Digital CAT = client SETTINGS COM Port; closing the window keeps audio. 7 DRAFT items in Appendix C (Windows-host steps with the session lock, ipconfig picture, fixed host address, what a refused client shows, retake chapter 3 pictures, mic-level wording vs Windows ALC 0.45, WSJT-X audio picture). Built with python-docx from the Windows draft (scratchpad, session-only); edit the .docx itself from now on. Not committed.

**2026-11-01: `docs/manuals/MSCC-Configurations-DRAFT.docx` (13 pages, Word, same style): client-server explained (ch 1: client, the 3 servers, ship bridge / engine room picture, always client-server even on one PC per Ron), then the five setups in plain English** (Ron): 1 Windows local, 2 Windows client PC1 + Windows servers PC2, 3 Windows client + Pi/Linux servers, 4 Pi/Linux local, 5 Pi/Linux client + Pi/Linux servers. Each: diagram, who for, what runs where, client settings, audio, digital programs, which guide. Key rule (Ron): Windows host = client owns the COM port (SETTINGS → COM Port; setup 2 WSJT-X on the client with Remote Digital); Pi/Linux host = host owns everything (mscc-init sets COM ports /dev/tnt + audio, WSJT-X runs on the Pi, reached by VNC; Windows client only operates + Remote Phones). 4 DRAFT items (setup 2 host keeping servers alive; setup 3 mscc-init port/device names + what the client does for Pi digital audio; setups 4/5 MSCC UI steps, wait for the Avalonia review). Next (Ron): the Avalonia client, then a Pi/Linux remote guide. Not committed.
**2026-11-06: Configurations draft, setup 6 added (Ron: "looks fine"):** Pi / Linux client + Windows host (section 3.6, diagram, DRAFT box: tested with MSCC Avalonia 1.6.86, MSCC UI 0.6.x lacks the devices / saved choices / RTS, Windows servers stop ~45 s after the client leaves). "five" -> "six" throughout, 2.1 table row, 2.2 sentence, chapter 4 column 6, Appendix A item 2. Edited with python-docx (no Word on this PC); LibreOffice users: Tools > Update > Update All for the contents page numbers.

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

