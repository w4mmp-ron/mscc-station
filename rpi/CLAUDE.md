# CLAUDE.md — sdrcore (Pi servers)

Claude's notes for the Pi servers `sdrcore-recv`, `sdrcore-trans` and `ms-sdr`
(Proficio / MSCC station). Owner: **Ron (W4MMP)**. Grok follows `AGENTS.md`; this
file is for Claude only.

**Shared rules for Ron and Stew (keywords, "finished" checklist, keeping trees in step): `../CLAUDE.md` (repo root).** This file adds Ron's machine-specific notes. Debian `linux/` is in the "trees in step" rule since 2026-11-13 (Stew copied the Pi server trees into `linux/*-linux`).

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
- Every server change goes into `rpi/`, Windows and `linux/` (Debian / Ubuntu amd64) in the
  same step. After a pull, report Stew's Windows and `linux/` changes and whether the Pi needs
  a matching port.

## "Finished" checklist (Ron, 2026-11-13)

Nothing is committed, pushed or published until Ron says "finished" ("build ..." = build only).
On "finished", in this order:

1. **Pi ms-sdr version.h:** copy `V:/ms-sdr-linux/source/version.h` -> `rpi/ms-sdr-linux/source/version.h`
   (V: -> repo only; never repo -> V:).
2. **Installers** for what changed (nothing changed = reuse the last one, marked "unchanged"):
   - Windows: build servers (MSBuild, Configuration=MFC; ms-sdr Win32, recv / trans x86) and/or client;
     `pwsh mscc-ui/installer/windows/stage.ps1`; set `AppVersion` + `SetupFileName` in `MSCC.aip`
     (keep its one UTF-8 BOM); `actinst.exe /S MSCC.aip` **from PowerShell** -> `installers/windows/`.
     If actinst sits at 0 % CPU, click "No" on its hidden newsletter box.
   - Pi: new server binaries into `rpi/mscc-binaries/` (from the Pi build), bump `Version:` in
     `rpi/mscc-deb/packaging/DEBIAN/control`, build in WSL Debian from a /tmp copy (CR stripped from text,
     not from .hex / .cyacd), compare file by file with the last .deb -> `installers/rpi/` + V:\;
     old .deb to `installers/rpi/old/`. `mscc-avalonia` only if the client changed.
   - Debian amd64 .debs: only once Stew's Debian work is done.
3. **Docs:** Software Versions Addendum (`docs/manuals/`, .md + `python docs/manuals/make-addendum-pdf.py`,
   2 pages), `installers/windows/CHANGELOG.md`, `installers/windows/INSTALL.md`, `installers/rpi/INSTALL.md`,
   this file. Guides carry no version numbers (they point to the Addendum).
   **Keep this file short (Ron 2026-11-14, claude-clean up):** the finished release entry and the
   entries it closes go straight to the end of `rpi/CLAUDE-history.md` (last section), not into the
   Open list here; only open items, rules and current setup stay in this file.
4. **One commit** (specific paths; not the Visual Studio .tlog / .recipe / .iobj / .ipdb / .lastbuildstate
   files), message ends with the "With Stew" list; **push**.
5. **GitHub Release** `R<m>-<d>-<n>`: `gh release create ... --notes-file`, assets = Windows .exe + Pi .deb(s).
   Notes: what changed, "Downloads: which file do I need?" (one line per file, "(unchanged)" where so),
   then the links to the Windows / Pi install guides and the Addendum PDF (links, not repo paths).
   Check the uploaded sizes against the local files.

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

Finished entries (releases R11-10-14 to R11-14-2, done work, old resume points) moved to `rpi/CLAUDE-history.md` on 2026-11-14 (last section there).

- **RX IQ START order fixed (client):** WPF `StartRxIq` and Avalonia `StartRxIqAsync` now send
  0x55 RX before 0x58 band (ms-sdr iq.c routes 0x58 by the last 0x55; band first after a TX I/Q
  session took the TX path: retune to the TX cal freq + trans TUNE). WPF 11.11.1, Avalonia
  1.6.95 (csproj + DEBIAN/control), both built, not tested on the rig. Test: TX I/Q cal, then
  RX IQ START: rig stays on the receive freq, trans not in TUNE.
- Stew note `.mscc-coord/NOTE-FOR-STEW-RX-IQ-TOOLS-2026-10-14.md`: option 4 (client AUTO
  button) removed (Ron).
**Current list (updated 2026-11-06, after bb31561):**
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
- **Waiting on Stew (list refreshed 2026-11-13 = the "With Stew" list in 08e53eb; notes in `.mscc-coord/`).**
  The 2026-11-01 list (14 items) was cleared: NB / spectral NR / NR Low-Med-High / heat sink / hard
  resets / client-gone fix / Windows SWR meter / radio cal read at connect were done by Ron 2026-11-10;
  the amd64 Avalonia build was done by Stew (f09daed); the ALC ceiling was settled 11-10.
  1. Debian / Ubuntu amd64 packages (`NOTE-FOR-STEW-PI-SERVERS-BUILD-ON-AMD64-2026-11-13.md`).
  2. Original firmware check (zip sent 11-11; missing: one Proficio, Geminus Legacy, all three Ultimus).
  3. Keyer update test on a PSoC radio with the daughter board (`radio-psoc-firmware/STEW-KEYER-UPDATE-TEST.md`).
  4. USB product name "Multus IQ" in all 8 radio images (his, `NOTE-FOR-STEW-WINDOWS-FIRMWARE-MULTUS-IQ-2026-11-13.md`);
     after that the Pi `mscc` package needs the new images.
  5. Option: git-ignore the Visual Studio build bookkeeping files in `mscc-ui/windows-work-tree`.
  6. FYI notes: shared rules (`NOTE-FOR-STEW-SHARED-CLAUDE-RULES-2026-11-13.md`), Avalonia-Migration
     frozen (`NOTE-FOR-STEW-AVALONIA-MIGRATION-FROZEN-2026-11-13.md`), Claude builds PSoC firmware
     from the command line (`NOTE-FOR-STEW-PSOC-COMMAND-LINE-BUILD-2026-11-13.md`; his Claude told
     him it can't).
  **Not on the 11-13 list; ask Ron if still wanted before raising them:** MKII PTT 3.234 non-QSK
  amp key delay scope check (`STEW-NON-QSK-AMP.md`); Legacy CW 1.236 on a Legacy rig
  (`Proficio-Legacy - Copy/STEW-LEGACY-NON-QSK-AMP.md`); PSoC non-QSK CW at 45 / 55 wpm, do the
  spaces go quiet (`NOTE-FOR-STEW-PSOC-CW-TOP-SPEED-2026-11-03.md`; users: "clean up to at least
  39 wpm"); reviews of the Windows Operator's Guide / Configurations / Remote drafts
  (`NOTE-FOR-STEW-USER-DOCUMENTS-2026-11-01.md`); taking our SWR meter firmware changes into his
  copy (`NOTE-FOR-STEW-SWR-METER-FIRMWARE-CHANGES-2026-11-06.md`).
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
  PSK31 / RTTY (maybe one character); FT8 / WSPR / SSB / CW: none.
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
- **Rules (Ron 2026-11-01):** assume Stew makes MSCC UI (Avalonia) match the current Windows client,
  and the Ubuntu scripts/utilities match the Pi. No separate Avalonia review; document the Pi
  versions for both Linux hosts. Linux guide Release 0 reviewed (old versions, Appendix G Pi install
  wrong for today's kit, cal now in the radio).
- **Next:** a Pi / Linux remote guide, when Ron says.
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

