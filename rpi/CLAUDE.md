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
| `swr-meter/ron/HF_SWR_R4_2/` | **Our copy of the WiFi SWR meter firmware** (R4.2 since 2026-11-21; folder + sketch renamed from HF_SWR_R4_1 so the build file name matches the version) (ESP32 Arduino; copied 2026-11-01 from Stew's `swr-meter/HF_SWR_R4_1/` @ 67fa644). In git since 2026-11-04: the source plus `swr-meter/ron/build/HF_SWR_R4_2.ino.bin` (the file to OTA-load at http://hf-swr-meter.local/update); other build output stays local. Claude edits here; Stew's copy is read-only. |
| Rest of `C:\Users\Ron\.grok\worktrees` | Read-only reference unless Ron says otherwise. |

Git repo root: `C:\Users\Ron\.grok\worktrees`.

## Open list (Ron, updated 2026-11-01) - read first on resume

Finished entries (releases R11-10-14 to R11-14-2, done work, old resume points) moved to `rpi/CLAUDE-history.md` on 2026-11-14 (last section there).

**HOLD (Ron 2026-11-17): no server or client changes** (Pi + Windows ms-sdr / recv / trans, WPF + Avalonia)
until Stew sorts out the remote audio issue (Windows client digital transmit bursts:
`.mscc-coord/REPORT-FOR-RON-WINDOWS-CLIENT-DIGITAL-TX-BURSTS-2026-11-17.md` + our reply
`NOTE-FOR-STEW-DIGITAL-TX-BURSTS-REPLY-2026-11-17.md`). Firmware (pill, SWR meter, keyer) is not on hold.
Reading / investigating is fine.
**Since 2026-11-18 (Ron): the Pi servers are open again; Windows servers + clients still on hold.** At "finished": Windows installer yes (for the pill / keyer firmware), but never change or rebuild the Windows servers or client (Stew's pushed builds go in as they are). Pi server changes are not ported to Windows; each release lists them in a note for Stew (`.mscc-coord/NOTE-FOR-STEW-PI-SERVER-CHANGES-NOT-ON-WINDOWS-2026-11-18.md`).

**Current list (updated 2026-11-06, after bb31561):**
- **Client tooltips = Stew's (Ron 2026-11-19), note `.mscc-coord/NOTE-FOR-STEW-CLIENT-TOOLTIPS-2026-11-19.md`,
  on the With Stew list.** Ron's original entry (2026-11-15), WPF AND Avalonia: none of the 208 WPF text tooltips has a line
  break, so long ones show as one wide strip (64 over 70 chars; longest: two-tone test 264, PADDLE 207,
  Save settings 193, FARNSWORTH 162, AMP on 155, Launch Servers 147, Reset configuration 145). Split at
  sentence breaks with `&#x0a;` (lines ~50-70 chars). Fix wrong text: WPF `MainWindow.xaml` Reset
  configuration says calibration reloads from factory (it keeps all cal since 11-10); RX IQ APPLY ALL
  "PC files only" unchecked. Avalonia: same texts (several differ today, e.g. PADDLE shows 0x9D / cw.ini).
  Not started.
- **"VAC" wording = Stew's (Ron 2026-11-19), same note as the tooltips, on the With Stew list.**
  Ron's original entry (2026-11-18): Remote Audio window says "VAC", WPF AND Avalonia.
  Ron uses two audio adapters wired back to back, not VAC; the text is hard-coded. Change:
  "DIGITAL RX (VAC)" -> "DIGITAL RX", "DIGITAL MIC TX (VAC)" -> "DIGITAL MIC TX", "Mute VAC play" ->
  "Mute digital play" (WPF `RemoteAfWindow.xaml.cs` ~117/118/137; Avalonia `RemoteAfWindow.axaml.cs`
  ~72/73/92), and the Avalonia Digital tooltip `RemoteAfWindow.axaml` ~23 "VAC for WSJT..." ->
  "Audio devices for WSJT...". Not started.
- **QRO cal in the radio: PSoC = Stew's (Ron 2026-11-19), note
  `.mscc-coord/NOTE-FOR-STEW-QRO-CAL-IN-PSOC-2026-11-19.md`, on the With Stew list.** When he reports the
  first PSoC version per radio, add the PSoC line to the Pi `Radio_has_qro_store()`. The pill part shipped in R11-19-0 (pill 3.24,
  Pi ms-sdr 3.214, trans 3.162; history). For the PSoC: same types (8 = int16 QRO per band, 9 = its
  valid mask, read only; type 5 stays 6 bytes). PSoC MKII (ATU/PTT 3.238+) EEPROM 2 KB = 128 rows; used
  rows 0, 1, 8-19 (cal row per band is full, byte 14 only spare); plan: 12 QRO rows 20-31, same
  one-row-per-band write. Then a second line in ms-sdr `Radio_has_qro_store()`: PSoC and minor >= the
  new PSoC version. QRP stays as is (Ron 2026-11-19): file values are NOT copied into the radio (no -99
  marker, cal belongs to one radio); no version check needed.
- **PSoC 12 kHz CW edge comb:** Stew's, PARKED on his side (3.244 / 3.245 failed; needs real USB vs DAC
  slot timing). Ron: leave it to Stew. Details + Ron's own-I/Q idea in `CLAUDE-history.md` (2026-11-15).
- **Stand-alone keyer (Ron 2026-11-05, started; `worktrees/keyer-standalone`, pushed c46d5f2).**
  The PIC keyer as its own 9 V box (from Ron's Rev 2 schematic, N8VET): modern rigs only (VO1400
  key out, isolated), PTT / amp output fitted, rig audio pass-through kept (RX path normally
  closed CPC1117: no current), LM386 on switched 5 V, MCP1703 LDO, AO3401A reverse protection,
  speed knob 5-60 wpm, FN + paddle menu, 1590B-size aluminium box. Draft 1: spec, netlist, BOM
  (LCSC numbers), reference drawing, Eagle schematic in Rev 2 style (Ron imported it into
  EasyEDA: "much better"). **Next:** (1) confirm the (check) items; (2) LCSC footprints in
  EasyEDA, then send the exported .net for a check against NETLIST.md; (3) firmware: stand-alone
  build of keyer-buffer (menu, speed pot, battery check, sleep), PC bench first.
- **Opcodes:** the current free list is the AVAILABLE OPCODES block at the top of the Pi ms-sdr
  `source/usbavrcmd.h` (no opcode is free in every layer now: 0x9D, 0x2F, 0x9F were taken 11-05 / 11-10 / 11-12;
  use the reclaimed list there). The 2026-11-05 survey is in `CLAUDE-history.md` (2026-11-15, part 2).
- **Waiting on Stew (list refreshed 2026-11-16 = the "With Stew" list in e2d88a9b, R11-16-1; notes in `.mscc-coord/`).**
  Remote audio: Windows client digital transmit bursts (his report + our reply, 2026-11-17); the hold above waits on it.
  Black Pill daughter board spin: 22-47 ohm series resistors on the bit clock and word clock (`NOTE-FOR-STEW-DAUGHTER-BOARD-CLOCK-RESISTORS-2026-11-18.md`; real fix for the receive slips, pill 3.22 is the backstop).
  Avalonia overwrites the server's settings at connect (`NOTE-FOR-STEW-AVALONIA-CONNECT-OVERWRITES-SERVER-2026-11-17.md`).
  0. Run a PSoC radio on Windows ms-sdr 3.214 (`NOTE-FOR-STEW-TEST-MS-SDR-3.214-PSOC-2026-11-16.md`).
  The 2026-11-01 list (14 items) was cleared: NB / spectral NR / NR Low-Med-High / heat sink / hard
  resets / client-gone fix / Windows SWR meter / radio cal read at connect were done by Ron 2026-11-10;
  the amd64 Avalonia build was done by Stew (f09daed); the ALC ceiling was settled 11-10.
  1. Debian / Ubuntu amd64 packages (`NOTE-FOR-STEW-PI-SERVERS-BUILD-ON-AMD64-2026-11-13.md`).
  2. Original firmware check (zip sent 11-11; missing: one Proficio, Geminus Legacy, all three Ultimus).
  5. Option: git-ignore the Visual Studio build bookkeeping files in `mscc-ui/windows-work-tree`.
  6. FYI notes: shared rules (`NOTE-FOR-STEW-SHARED-CLAUDE-RULES-2026-11-13.md`), Avalonia-Migration
     frozen (`NOTE-FOR-STEW-AVALONIA-MIGRATION-FROZEN-2026-11-13.md`), Claude builds PSoC firmware
     from the command line (`NOTE-FOR-STEW-PSOC-COMMAND-LINE-BUILD-2026-11-13.md`; his Claude told
     him it can't).
  **Not on the 11-13 list** (5 older Stew items: non-QSK amp delay, Legacy CW 1.236, PSoC CW top speed, guide
  reviews, SWR meter firmware): ask Ron before raising them; list in `CLAUDE-history.md` (2026-11-15, part 2).
- **Pill audio stats cron (2026-11-08):** `*/10 * * * * /usr/bin/python3 /home/ron/utils/audio-stats.py log`
  -> `~/.local/mscc/audio-stats.log`; check `grep '<<<' ~/.local/mscc/audio-stats.log`. Since 2026-11-17 the line also has `frame_err N` (pill I2S
  receive frame errors; > 0 = receive slip watch, see the blackpill CLAUDE.md).
  A few gaps / in_drop / out_under in a window with TUNE, band or mode changes are expected (trans
  stream reopen); Ron dropped that fix 2026-11-16 (no user impact; `CLAUDE-history.md`).
- **Rules (Ron 2026-11-01):** assume Stew makes MSCC UI (Avalonia) match the current Windows client,
  and the Ubuntu scripts/utilities match the Pi. No separate Avalonia review; document the Pi
  versions for both Linux hosts. Linux guide Release 0 reviewed (old versions, Appendix G Pi install
  wrong for today's kit, cal now in the radio).
- **Waiting on Stew (2026-11-19):** he attaches his Ubuntu amd64 .debs to our releases himself
  (`gh release upload` + version table lines; Ron 2026-11-19, in `CLAUDE.md` step 5 and `AGENTS.md`,
  reply note `.mscc-coord/NOTE-FOR-STEW-MSCC-STATUS-AND-RELEASES-REPLY-2026-11-19.md`). Our notes say
  "Ubuntu packages: added by Stew". Release layout (all installers, version table, keep newest two) is in the
  repo `CLAUDE.md` step 5.
- **Next:** a Pi / Linux remote guide, when Ron says.
- **Parked:** NR warble fix (Ron 2026-11-01: leave it, NR works well; lower level = less
  warble). Sync AM (off the list). Complex Goertzel for FREQ CAL and FM transmit (future ideas,
  see `CLAUDE-history.md`, 2026-11-15). CW ramp length by speed (Ron 2026-11-02, just a thought): the key
  delay (20-40 ms) already shows each element's length ahead, so the ramp could be chosen per
  element (longer normally, shorter only for very short elements/spaces), any key, no speed
  needed. Details + pill TX underrun while keyed (fix only if one shows up): `CLAUDE-history.md` (2026-11-15, part 3).

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
cd rpi/SDRcore-recv-linux  && make clean BINDIR=/tmp/msbuild && make BINDIR=/tmp/msbuild
cd ../SDRcore-trans-linux  && make clean BINDIR=/tmp/msbuild && make BINDIR=/tmp/msbuild
cd ../ms-sdr-linux         && make clean BINDIR=/tmp/msbuild && make BINDIR=/tmp/msbuild
```

Without `BINDIR`, `make` installs to `~/mscc/` (replaces the running binary) and `make clean`
DELETES the installed server (happened 2026-11-14, recv). Ron builds for real himself.
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

