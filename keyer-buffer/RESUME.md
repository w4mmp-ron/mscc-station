# Keyer — handoff / resume

## I2C bootloader (started 2026-11-12) — update the PIC from the Pi, no PICkit

**2026-11-12, Ron: "No step 3 for now."** Steps 1 + 2 done and shipped; step 3 (PSoC
pass-through) PARKED, don't start unasked. Until then PSoC radios update the keyer with the PICkit.
**Status (history):** step 1 of 3. `bootloader/` written and builds (551 of 1,024 words); not yet on a chip.
Design and protocol: `bootloader/README.md`. Bootloader address 0x5C (Ron's choice; 0x40-0x4F
avoided, the Potentia power sensor is also 0x40 on the PSoC bus). Daughter-board radios only,
both pill and PSoC.
**Steps:** (1) bootloader + keyer app at code offset 0x400 + opcode 0x9F 0x5C (enter
bootloader), bench test with the PICkit; (2) pill I2C pass-through command + Pi
`keyer-flash.py`; (3) same pass-through in the PSoC (`Proficio-Ultimus-MKII-PTT`).
**Done (step 1, not yet on a chip):** app built at code offset 0x400 (production config;
`Makefile-production.mk` patched by hand with `-mcodeoffset=0x400` — MPLAB's command-line
makefile generator renames the output after the folder, so don't use it). Opcode 0x9F 0x5C
(free in every layer per ms-sdr `usbavrcmd.h`; 0x9E is CMD_SET_FM_POWER there) sets EEPROM
0xFF and resets. `copy-hexfile.bat` now makes two files: `Release\Keyer-MCU-full-DATE.hex`
(bootloader + app + EEPROM defaults, PICkit 3) and `Release\Keyer-MCU-app-DATE.hex` (app
only, Pi utility). First pair: 20261112 (checked: layout, config words match, EEPROM 0xFF blank).
**In the rig since 2026-11-12:** `Keyer-MCU-full-20261112.hex` (PICkit, `-Z`); backup before it
`Release\Keyer-MCU-backup-before-boot-20261112.hex` (= 11-10 stream build). Ron: all tested, keyer
works fine. **Step 1 done.**
**Step 2 TESTED 2026-11-12:** PIC has bootloader v2 (PICkit, `-Z`), pill 3.16 flashed via the
Pi; `keyer-flash.py firmware/Keyer-MCU-app-20261112.hex` on the Pi: 179 rows written +
checked in 22 s, app back. PICkit read-back: all code = full file, EEPROM unchanged, 0xFF
clear. Pi files: `~/utils/keyer-flash.py`, `~/utils/firmware/` (the Pi's scripts live in
`~/utils`, not `~/utilities` as the utility-scripts README says). GUI added 2026-11-12:
`keyer-flash-gui.py` (tkinter, Pi + Windows, uses keyer-flash.py from the same folder; layout
like the Pi's psoc-bootloader.gui). Run on the Pi desktop by Ron 2026-11-12 (mscc 1.0.83 install): works. Ron 2026-11-12: no
keyer updates while the servers run: both tools check first and, if any server is running,
say "stop the servers" and quit (GUI: message, window closes; also re-checked before each
button). They no longer stop / start mscc themselves. Tested on the Pi with mscc up: refused.
**Windows tested 2026-11-12 (Ron, MKII on the PC, servers closed):** `keyer-flash.py --check`
OK, then the GUI: Program, 179 rows, "Keyer updated and running". Both Pi (CLI) and Windows
(GUI) update paths proven. Windows Load File opens the script folder (no keyer firmware folder
on Windows yet; at "finished": installer payload "Keyer Firmware" like Black Pill / PSoC).
GUI: "Leave Bootloader" became "Start Keyer", greyed out unless the keyer is found waiting in
its bootloader. **Pi install (Ron: ~/utils is his own, don't use it):** scripts in
`/usr/share/mscc/bin`, links `/usr/local/bin/keyer-flash` + `keyer-flash.gui`, menu
`/usr/share/applications/mscc-keyer-flash.desktop` ("Keyer Firmware Update"), hex in
`/usr/share/mscc/firmware/keyer/` (Load File opens there). Put there by hand 2026-11-12; my
files removed from ~/utils.
**Shipped 2026-11-12 (Ron: "finished"):** sources moved to `firmware-programmers/keyer-flash/`
(README, build-exe.cmd); Windows .exe (PyInstaller) in `firmware-programmers/windows/keyer-flash/`.
Windows installer R11-12-1: `C:\mscc-net9\Keyer Firmware\` (2 .exe + app/full hex, Start-menu
"Keyer Firmware Update"; stage.ps1 + MSCC.aip). Pi mscc 1.0.83: scripts, links, menu entry,
`/usr/share/mscc/firmware/keyer/` (build-deb.sh, postinst, prerm). Both carry Black Pill 3.16.
1.0.83 built + compared with 1.0.82 (only the keyer files and 3.16 differ); Installed on the Pi
2026-11-12 (Ron asked). R11-12-2 (same day): Windows GUI .exe no longer flashes console windows
(tasklist with CREATE_NO_WINDOW); Pi files unchanged in function, mscc 1.0.83 kept. **Next: step 3 (PSoC).**
Bootloader version 2: row sent in 16-byte pieces
(`05 off d0..d15`, then `02 sum`) so each I2C transfer is ~2 ms. The rig still has version 1
(`Release\Keyer-MCU-full-20261112-boot1.hex`); `Keyer-MCU-full-20261112.hex` is now version 2
(same app) and goes on once more with the PICkit. Pill 3.16: `CMD_KEYER_BOOT 0x9F`
(`keyer_boot.c`, beater case 3; blocking HAL OK, Ron: servers are stopped during an update).
8 MHz tree synced (keyer_boot.c/.h copied by hand: the sync script copies tracked files
only). Script: `keyer-flash.py` (now in `firmware-programmers/keyer-flash/`). `usbavrcmd.h`:
0x9F marked taken (copied to V:).

## keyer-buffer: buffered Farnsworth (Ron's design, 2026-11-05) — read first

**Tree:** `worktrees/keyer-buffer` (copy of `worktrees/keyer` at d40a7ba; work here).
**In the rig since 2026-11-05:** `Release/Keyer-MCU-buffer-20261105.hex` (PICkit 3, EEPROM kept).
Backup of the PIC before it: `Release/Keyer-MCU-backup-before-buffer-20261105.hex`.
**Ron, 2026-11-05: all tested on the rig and working; "the speed sounds spot on".**

**2026-11-10: streamed (in the rig, `Release/Keyer-MCU-buffer-stream-20261110.hex`; backup of
the PIC before it `Keyer-MCU-backup-before-stream-20261110.hex`). Ron: works, a held paddle
starts right away.** Was: a letter went on the air only once closed (2-dit pause or 16
elements), so a held paddle waited ~2 s (dit) / ~4 s (dah) and came out in 16-element chunks.
Now the buffer holds 1-byte symbols (dot / dash / letter end / word space, 128 entries) and the
player starts each element as it is keyed; letter / word gaps unchanged (the wait for the
letter to close counts towards the gap). No 16-element limit on the paddles; STP also at 120
entries waiting (guard). EEPROM layout unchanged (hex EEPROM data identical). PC bench
(scratchpad, not kept): held dit / dah key in 1 ms, steady; E-gap-E = 302 ms at 18/15; STP at
20 letters, drain, back to idle. The rest of this section describes the 11-05 letter buffer;
it still holds except where this paragraph says otherwise. Pi cw.ini is
`~/.local/mscc/cw.ini` (not the Avalonia host copy).

**Idea:** key at whatever speed feels right; the sidetone follows your hand; each finished
letter goes into a buffer and the transmitter sends it with Farnsworth spacing (letters at
SET_WPM, gaps at the 0x76 text WPM). Too far ahead -> STP on the sidetone, paddles off,
buffer drains, RES, paddles back once released.

**Switch:** `SET_PADDLE_FARNSWORTH 0x9D` = 1 (buffered) / 0 (direct, old behaviour).
Pi: `cw.ini` `CW_Paddle_Farnsworth=` (ms-sdr 3.192+, sent at keyer start-up), pill 3.13+
passes it to the PIC. Not in the PIC EEPROM (a new `__eeprom` variable moves the settings
layout; XC8 does not keep source order). Client never sees it.

**How it is built (main.c):**
- 1 ms tick: TMR2 (Fosc/4 8 MHz, 1:64, PR2 124). ISR only counts (`keyer_tick_isr`).
  **4 lines added by hand in `mcc_generated_files/interrupt_manager.c`; re-add if MCC
  regenerates it.** Main loop: one `keyer_step()` per tick, then one I2C command.
- No busy-wait loops left. State machines: paddles (`paddle_tick`, iambic A/B + straight,
  live sidetone), player (`seq_t player`, keys KEY_0A from the buffer or a CQ memory),
  announcer (`seq_t announcer`, sidetone only: STP / RES), `mem_tick` (CQ start / end).
- Element times are real ms: dot = 1200 / WPM (old keyer: loop counts, speed off).
- Buffer: 64 entries (letter = up to 16 elements as bits, or word space), STP at
  **20 letters waiting** (`BUF_LIMIT`, Ron 2026-11-05). Letter closes after 2 dits open,
  word space after 5 dits open. STP starts after a 7-dit pause.
- Player gaps: letter gap = max(3 dits, ARRL letter gap), word extra = max(4 dits, ARRL);
  idle time counts towards a word space. CQ memories use the same gaps (min 3 dits now;
  old keyer used 2).
- CQ abort: the abort press only aborts (no element), the element on the air finishes.
- EEPROM layout identical to the old keyer (checked against the .map files).

**Tests:** PC test bench (real main.c, PIC pins stubbed, 1 ms steps, operator model):
direct 18 wpm (66 / 198 ms), buffered 18/13 (on air 399 ms letter gaps, 931 word),
overflow STP / drain / RES, CQ play + abort, straight key. Bench was in Claude's scratchpad
(`ksim/`), not kept in the repo. On the rig (Ron): STP at 30 wpm, paddles off until the
buffer cleared, RES, paddles back; CQ, straight, direct all fine.

**PIC reset kink: FIXED 2026-11-10 (pill 3.14).** A PIC reset with the radio on (e.g. reflash)
loses 0x9D (PIC RAM only, starts at 0). The pill used to forward 0x9D only when it changed;
3.14 passes every 0x9D from ms-sdr on, so after a PIC reset one `mscc restart` (or client
reconnect) puts it back. Tested by Ron: PIC re-flashed, one restart, held paddle buffered again.
(Ron's choice over a 2 s periodic re-send: 0x9D only comes at start-up or when the option changes.)

**Ideas, not started:** keyboard CW (client text into the player; client is Stew's).
**Stand-alone version (2026-11-05):** hardware draft in `worktrees/keyer-standalone` (9 V box);
its firmware will be a `STANDALONE` build of this tree (see that folder's spec, section 6).

**Build:** MPLAB X project in this folder (paths point at keyer-buffer). Command line:
`make -f nbproject/Makefile-production.mk SUBPROJECTS= .build-conf CONF=production` with
MPLAB's GnuWin32 `make.exe` on PATH; the post-build `copy-hexfile` step fails from bash
(run `copy-hexfile.bat` from cmd; since 2026-11-12 it also builds the bootloader and makes
the full + app release files). The app hex alone starts at 0x400 and will not run without
the bootloader.
Flash: `ipecmd -TPPK3 -P16F18326 -F<hex> -M -Z -Y -OL` (`-Z` keeps the EEPROM, rig powers
the PIC); read a backup first with `-GF<file>`.

---

# Keyer CQ memory

**Date:** 2026-08-08 (status updated 2026-11-02)  
**Tree:** `worktrees/keyer`  
**Detail protocol:** `KEYER-MEMORY.md` (read that first for opcodes and test scripts)

---

## Status

| Item | State |
|------|--------|
| PIC firmware | **Done** — 4 slots × 48 chars, select/store/play, paddle abort edge |
| Proficio | **Done** for `0x9C` — USB 2-byte `[param,seq]`, ring queue (80), I²C one byte at a time |
| ms-sdr (Linux) | **Done** for `0x9C` path + helpers + pacing; see `ms-sdr-linux/RESUME.md` |
| ms-sdr (Windows) | **Done** (Stew) — `0x9C` in `ms-sdr-MKII/source/main-controller.c` (~line 804) |
| MSCC client (WPF) | **Done** (Stew) — CW tab, **CQ MEMORY**: 4 slots, text box + R (store) + P (play); Farnsworth for memory play (`MainWindow.xaml` ~912, `MainViewModel.cs` ~5484) |
| Farnsworth `0x76` | **Full path** — ms-sdr + Proficio + PIC memory-play gaps |

End-to-end proven: store/play slot 0–3 via USB script and via UDP → ms-sdr → Proficio → PIC.

---

## Protocol (wire)

```
Client / test  →  ms-sdr (UDP :8888)  →  Proficio USB  →  I2C  →  PIC16F18326
```

### `CMD_SET_KEYER_MEMORY` `0x9C` — one param per transfer

| Param | Action |
|-------|--------|
| `0` | Play current slot |
| `1` | Store begin (clears builder length — new message replaces old) |
| `2` | Store end → EEPROM |
| `3` | Select — **next** param is slot `0..3` (sticky) |
| `0x20`–`0x7E` | Append ASCII (max 48) |

USB (Proficio): vendor OUT `bRequest=0x9C`, `wValue=0x071B`, **2 data bytes** `[param, seq]` (seq 1–255, never 0).  
Not zero-length OUT. Not bulk text.

### `SET_MEM_TEXT_WPM` `0x76`

- Farnsworth **memory play only** — text/overall WPM for inter-letter/word gaps; elements stay on `SET_WPM`.
- Param: `0` = off; `5–60` = text WPM; if text ≥ char → no stretch.
- PIC: EEPROM `cw_mem_text_wpm`; **ARRL/PARIS** gaps (`S=(50C−31T)/T`, letter 3/19, word 4/19).
- Paddle ELEMENT/LETTER spacing unchanged.

---

## Spacing notes (ops)

- Memory play uses **standard PARIS** packing at set WPM (not Farnsworth).
- UI **ELEMENT / LETTER** is paddle auto letter-space only — does **not** change memory play.
- ARRL practice below ~18 often uses Farnsworth; keyer memory does not (yet).

---

## Test jigs (this tree)

| Script | When |
|--------|------|
| `keyer-mem-test.py` | **ms-sdr stopped**; direct USB to Proficio (needs `python3-usb`) |
| `keyer-mem-udp-test.py` | **ms-sdr running**; UDP host default `192.168.12.199:8888` (or `--host proficio`) |

UDP default message (timing check): `O O O O O O O O O O O O O O O` → slot 0 → play.  
Handshake `0xFE,1`, drain startup, no STOP on exit. Single-session lock — no other MSCC client.

---

## Build / flash

1. **PIC:** MPLAB X → `keyer` → program PIC16F18326 (hex under `Release/` when built)
2. **Proficio:** PSoC Creator → `Release-Proficio-MKII-PTT` MKII-PTT only
3. **ms-sdr Linux:** rebuild on Pi; deploy binary as usual

---

## GUI (Stew)

Built in the WPF client (see Status). Original client guide: **`mscc-client/docs/KEYER-MEMORY-GUI-GUIDE.md`** (mockup: 4 slots, R=store, P=play).

## Resume prompt (paste)

> Keyer CQ memory handoff. Protocol in `keyer/KEYER-MEMORY.md` + `keyer/RESUME.md`. PIC, Proficio, Linux and Windows ms-sdr, and the WPF client CQ MEMORY section are all done. Open: sidetone first-key pop (`hardware-fix/SIDETONE-FIRST-KEY-POP.md`).
