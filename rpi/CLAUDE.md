# CLAUDE.md — sdrcore (Pi servers)

Claude's notes for the Pi servers `sdrcore-recv`, `sdrcore-trans` and `ms-sdr`
(Proficio / MSCC station). Owner: **Ron (W4MMP)**. Grok follows `AGENTS.md`; this
file is for Claude only.

**Shared rules for Ron and Stew are in `../CLAUDE.md` (repo root) and are not repeated here:**
keywords ("build" / "update the notes" / "finished" / "stop"), pull first, nothing in git before
"finished", ask before editing, one commit with specific paths, trees in step, versions, the
"finished" checklist, build notes. This file adds only Ron's extras.

## Working with Ron (extras)

- Server trees in step = `rpi/*-linux` (also copied to V:), Windows `mscc-ui/windows-work-tree/`,
  Debian `linux/*-linux` (since 2026-11-13, Stew copied the Pi trees there). Ron's Avalonia is
  `worktrees/Avalonia`.
- Ron does **not** `git pull` on the Pi. He copies the changed source files from Windows to the
  Pi and builds there. Give him the file list, not git steps.
- After a pull, report Stew's Windows and `linux/` changes and whether the Pi needs a matching port.

## "Finished": Ron's extras to the shared checklist

- Step 1: the Pi copy is `V:/ms-sdr-linux/source/version.h` (V: -> repo only; never repo -> V:).
- Windows: the server sln build bumps ms-sdr `version.h` even with no code change; put it back if
  ms-sdr didn't change. Commit the rebuilt server exes in `mscc-ui/Release/windows-wpf`.
- Pi .deb: from the Pi build's binaries; the new .deb goes to `installers/rpi/` and V:\.
- Docs: also this file. **Keep this file short (Ron 2026-11-14):** the release entry and the
  entries it closes go straight to the end of `rpi/CLAUDE-history.md`, never into the Open list;
  only open items, rules and current setup stay here.
- Release: `gh release create R.. -R w4mmp-ron/mscc-station ...`.

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

**Current list (updated 2026-11-06, after bb31561):**
- **WiFi SWR meter (Ron's copy `swr-meter/ron/HF_SWR_R4_1/`):** tune allowance, mDNS, WiFi retry, PEAK hold,
  AVG, PEAK SWR all built and tested (details in `CLAUDE-history.md`, 2026-11-15 section). Open: test the
  WiFi retry (router off, meter restarted); Ron to decide if ms-sdr itself stops TX on a meter fault
  (backstop for WSJT-X on the Pi keying via CAT). Build: arduino-cli (FQBN + options in the history).
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
- **PSoC 12 kHz CW edge comb:** Stew's, PARKED on his side (3.244 / 3.245 failed; needs real USB vs DAC
  slot timing). Ron: leave it to Stew. Details + Ron's own-I/Q idea in `CLAUDE-history.md` (2026-11-15).
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
- **Pill audio stats cron (2026-11-08):** `*/10 * * * * /usr/bin/python3 /home/ron/utils/audio-stats.py log`
  -> `~/.local/mscc/audio-stats.log`; check `grep '<<<' ~/.local/mscc/audio-stats.log`.
  **Open fix (Ron OK'd after the hold, not started):** transmit underrun at every band change and tune
  key-up = sdrcore-trans `udp_thread.c` CMD_SET_MAIN_MODE reopens the stream even for the same mode.
  Better fix (Ron): in DUAL STREAM (Ron's Pi) never reopen the I/Q stream, only stop / ignore the mic
  stream in T / C; keep the reopen for single full-duplex. Probably also removes the receive gaps /
  in_drop (not verified; pill ruled out by code read). Full investigation in `CLAUDE-history.md` (2026-11-15).
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
- **Parked:** NR warble fix (Ron 2026-11-01: leave it, NR works well; lower level = less
  warble). Sync AM (off the list). Complex Goertzel for FREQ CAL and FM transmit (future ideas,
  see `CLAUDE-history.md`, 2026-11-15). CW ramp length by speed (Ron 2026-11-02, just a thought): the key
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
FREQ CAL saturation fix + #5 fixes (2026-09-27), complex Goertzel idea + limit wording (Ron OK'd 09-29),
FM transmit plan (future, 6 steps) and the DC blocker (done 2026-10-04): moved word for word to
`CLAUDE-history.md` (2026-11-15 section).

