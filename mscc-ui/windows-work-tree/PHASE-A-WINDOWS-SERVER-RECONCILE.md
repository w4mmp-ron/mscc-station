# Phase A — Windows servers ↔ Linux servers reconcile

**Date:** 2026-09-06  
**Repo:** `mscc-station` (`w4mmp-ron/mscc-station`)  
**Goal:** Full **Windows standalone** parity with Linux appliance behavior, then any client/server mix (Win↔Linux).  
**Method:** Inventory only (this doc). **No ports until Stew/Ron green-light implementation waves.**

---

## Context (agreed)

| Who / how | Setup |
|-----------|--------|
| Stew day-to-day | Client + servers on **Windows** |
| Ron | Heavy **remote** (often WPF → Pi) |
| Stew remote | Windows client → RPi when needed |
| Avalonia test plan | Avalonia + **Windows servers locally**, then Avalonia → Pi / Win mixes |
| STM32 Proficio | Out of scope |

**North star:** Same opcodes/behavior whether cores run on Windows or Linux.

---

## Trees compared

| Component | Windows | Linux (reference) |
|-----------|---------|-------------------|
| ms-sdr | `mscc-ui/windows-work-tree/ms-sdr-MKII/` | `ms-sdr-linux/` |
| sdrcore-recv | `mscc-ui/windows-work-tree/SDRcore-recv/` | `SDRcore-recv-linux/` |
| sdrcore-trans | `mscc-ui/windows-work-tree/SDRcore-trans/` | `SDRcore-trans-linux/` |

Related (client/helper, not this phase’s C ports):  
`mscc-ui/Avalonia-Migration/`, `mscc-ui/windows-work-tree/mscc-mscc/`, `mscc-remote-audio/`.

---

## Executive summary

| Priority | Gap | Blocks |
|----------|-----|--------|
| **P0** | Windows **ms-sdr** missing live **`0x9C`** keyer memory (2-byte USB + pace + UDP) | CQ memory R/P on Windows standalone |
| **P0** | Windows **ms-sdr** missing / incomplete **`0x76`** Farnsworth path | Farnsworth UI vs Windows servers |
| **P1** | Windows **sdrcore-recv/trans** missing **`REMOTE_AUDIO` (2)** + MSA1 remote phones/mic | Remote Audio when cores are on Windows |
| **P1** | Windows **ms-sdr** GUI egress / keep-alive / `SO_RCVBUF` behind Linux | Spectrum / KA under load / remote GUI |
| **P2** | Trans NULL-input TUNE/CW + digi bounds (Linux fixes) | Digi/TUNE quirks OS-dependent |
| **P3** | WiFi SWR ingest, dual-stream/Oboe/ALSA | Mostly Pi / ask Ron |

**Already aligned enough for Phase A:** session lock, basic CW keyer opcodes (mode/WPM/etc.), pan bin protocol 0/1/2, much DSP/TX opcode surface, `CMD_SET_AUDIO_DEVICE` **0/1** (not **2**).

---

## Wave 1 — ms-sdr-MKII (must for keyer + Farnsworth)

### 1.1 `CMD_SET_KEYER_MEMORY` `0x9C` — **MUST PORT**

| | |
|--|--|
| **Linux** | `usbavrcmd.h` defines; `Radio_send_parameters` sends **2 bytes** `[param, seq]` (seq 1–255); `KEYER_MEM_USB_GAP_MS` (40); `KEYER_MEM_END_SETTLE_MS` (**400 in header**, RESUME says **100** — ask Ron); helpers; UDP `case` → `Keyer_Memory_Param`; fail path does not `Stop_all` for mem |
| **Windows** | **No** `CMD_SET_KEYER_MEMORY` in live `usbavrcmd.h` (only commented legacy); `Radio_send_parameters` always `sizeof(int)` OUT + generic fail → `Stop_all` |
| **Files** | `source/usbavrcmd.h`, `main-controller.c`, `extern.h`, `opcodes.txt` |
| **Risk if skipped** | Avalonia/WPF R/P against Windows ms-sdr cannot drive Proficio/PIC memory correctly |

**Port checklist (from Linux + Ron RESUME):**

1. Add defines: `CMD_SET_KEYER_MEMORY`, `KEYER_MEM_*`, gap/settle constants.  
2. Special-case `Radio_send_parameters` for `0x9C` only: 2-byte pack + seq + paces.  
3. UDP handler: one param per packet → helper.  
4. Optional helpers: `Keyer_Memory_Param/Select/Play/Store`.  
5. Gate on MKII + keyer installed (as Linux).

### 1.2 `SET_MEM_TEXT_WPM` `0x76` — **MUST PORT** (USB gated by Ron)

| | |
|--|--|
| **Linux** | Define `SET_MEM_TEXT_WPM`; `cw_record.text_wpm`; `cw.ini` `CW_Mem_Text_WPM`; UDP set + ini; GUI push; init USB when keyer installed |
| **Windows** | `SET_MEMORY_TYPE` / `0x76` commented; no live Farnsworth path |
| **Files** | `usbavrcmd.h`, `extern.h`, `main-controller.c` (ini parse/update, UDP, `initialize_keyer`, GUI push) |
| **Risk if skipped** | Farnsworth UI/reconnect diverge on Windows servers |

**Ask Ron:** RESUME historically said “define/ini OK, **don’t USB** until FW”; current Linux **does** USB. Confirm FW status → Windows should mirror Linux USB or stay ini/UDP-only first.

### 1.3 GUI egress + keep-alive + `SO_RCVBUF` — **MUST PORT** (reliability)

| Item | Linux | Windows | Risk |
|------|-------|---------|------|
| `Gui_send_param` destination | Prefer bound `dll_s` + `G_session_client` | Largely `gui_s` / `si_gui` | Remote GUI / pan delivery |
| Keep-alive | ~1 s while session; cheap KA (no Sleep on `0xF4`) | Heavier Sleep patterns | False KA loss / disconnect |
| `SO_RCVBUF` | Enlarged (~4 MiB) on command socket | Not mirrored | Dropped pan/KA under load |

**Ask Ron:** Required for **local** Windows GUI too, or primarily remote-client-to-Windows-ms-sdr?

### 1.4 WiFi SWR (`swr_wifi_meter.c`) — **OPTIONAL / ASK RON**

Linux-only ingest. Meter opcodes exist both sides for other paths. Port only if Windows ms-sdr must feed UI from WiFi SWR the same way.

---

## Wave 2 — SDRcore-recv / SDRcore-trans (remote audio + robustness)

### 2.1 Opcode `REMOTE_AUDIO` = **2** — **MUST PORT** (both cores)

| | Recv | Trans |
|--|------|-------|
| **Linux** | `commands.h` `REMOTE_AUDIO 2`; treat like Phones speaker + enable MSA1 **out** (~9100) | `REMOTE_AUDIO 2`; `G_audio_mode`; MSA1 **mic in** (~9101) |
| **Windows** | `CMD_SET_AUDIO_DEVICE` handles **0/1 only** | Same — **0/1 only** |
| **Client contract** | Avalonia/WPF send **2** when Phones + Remote Audio (`STEW-REMOTE-AUDIO.md`) | |

**Risk if skipped:** On Windows standalone/remote-to-Windows-PC, Remote Audio checkbox is a no-op at the cores (device 2 ignored).

**Minimum port:** define `REMOTE_AUDIO 2` + handler (recv: alias Operator path; trans: set mode + Phones stream).  
**Full port:** MSA1 modules `remote_phones.c` / `remote_mic.c` (or shared) for real remote AF when cores are Windows-hosted.

**Ask Ron:** For Windows-local radio, is full MSA1 required, or is opcode-2-as-Operator enough until remote-to-Windows-PC is tested?

### 2.2 Trans digi / TUNE safety (Linux fixes) — **SHOULD PORT**

| Item | Why |
|------|-----|
| NULL `inputBuffer` still runs tune modulate | Windows: digi idle can kill TUNE/CW carrier |
| Digital device index bounds / fallback | Avoid OOB / dead streams |

Low cost; reduces “works on Pi, fails on Windows” TX bugs.

### 2.3 Pan GAIN/BASE stubs — **LOW**

Linux logs only; Windows may lack cases. Low functional risk.

### 2.4 Do **not** blindly port

Dual PortAudio + digi ring + Oboe resampler, ALSA dual-stream, Linux path/logging shims — **platform-only** unless Windows hits mixed-API/rate pain.

---

## Already aligned (do not rework in Wave 1)

- Single-session GUI lock / `CMD_CHECK_GUI_STATUS`  
- Core CW keyer UDP (mode, paddle, spacing, weight, hold, WPM, QSK)  
- `CMD_SET_AUDIO_DEVICE` **0 = Digital, 1 = Operator/Phones**  
- Pan refresh → 800/1600/3200 bins  
- Large shared DSP/TX opcode surface  
- Session / appliance / headless themes present on both ms-sdr trees  

---

## Ask Ron (before or during Wave 1)

1. **`KEYER_MEM_END_SETTLE_MS`:** header **400** vs RESUME **100** — which for Windows?  
2. **`0x76` USB on Windows now?** Mirror Linux USB, or ini/UDP first?  
3. **WiFi SWR** on Windows ms-sdr — in or out of Phase A?  
4. **MSA1 on Windows cores** — full remote_phones/mic, or opcode **2** alias first?  
5. **Pan/KA egress** — required for local Win GUI or mainly remote GUI to Win ms-sdr?  

---

## Suggested implementation order

| Step | Work | Verify |
|------|------|--------|
| **A1** | Port `0x9C` to Windows ms-sdr | Avalonia/WPF R/P + UDP jig vs Windows ms-sdr + radio |
| **A2** | Port `0x76` (per Ron USB answer) | Farnsworth control / ini round-trip |
| **A3** | GUI egress + KA + `SO_RCVBUF` | Spectrum stable; no false KA drop under pan |
| **A4** | `REMOTE_AUDIO 2` on Win recv+trans (+ MSA1 if agreed) | Remote Audio checkbox with Windows cores |
| **A5** | Trans NULL-input / digi bounds | TUNE/CW on Digital path |
| **A6** | Rebuild Windows server binaries / install path | Document versions next to `mscc-net9` / Launch Servers |

Then **Phase B** smoke (Avalonia + Windows servers), then **Phase C** Avalonia↔WPF UI parity.

---

## Out of scope this phase

- Avalonia Farnsworth **UI** polish (Core may already send `0x76` — UI after servers)  
- STM32 Proficio replacement  
- Replacing Linux dual-stream design on Windows  
- Client-only WPF/Avalonia feature ports (Phase C)

---

## Resume prompt

> Phase A Windows server reconcile punchlist: `mscc-ui/windows-work-tree/PHASE-A-WINDOWS-SERVER-RECONCILE.md`. Linux is reference for `0x9C` (2-byte USB+pace), `0x76`, GUI/KA egress, and sdrcore `REMOTE_AUDIO=2` + MSA1. Windows ms-sdr lacks live keyer memory path; Windows sdrcore lacks device 2. Implement Wave 1 ms-sdr first, then Wave 2 cores. Ask Ron on settle ms, 0x76 USB, MSA1-on-Windows, WiFi SWR.

---

## Implementation status (2026-09-06)

| Step | Status | Notes |
|------|--------|--------|
| **A1** `0x9C` on Windows ms-sdr | **Done** | 2-byte USB pack, pacing, helpers, UDP case |
| **A2** `0x76` on Windows ms-sdr | **Done** | ini / UDP / GUI push / init USB (mirrors Linux) |
| **A3** GUI egress / KA / SO_RCVBUF | Deferred | Not in this drop |
| **A4** `REMOTE_AUDIO=2` + MSA1 on Win recv+trans | **Done** | Opcode 2 + Winsock `remote_mic` / `remote_phones`; binaries in `C:\mscc-net9` |
| **A5** Trans digi/TUNE NULL-input | Deferred | |
| **Build → `C:\mscc-net9`** | **Done** | `ms-sdr-MKII.exe`, `mscc-recv.exe`, `Mscc-trans.exe` (+ pdb/dlls) |

Restart Launch Servers after copy. Smoke with Avalonia local against Windows servers (CQ R/P, Farnsworth if UI sends `0x76`, Remote Audio checkbox).

---

## Next action

**Stew:** Restart Windows servers from `C:\mscc-net9`, smoke Avalonia + local servers.  
**Later:** Phase C Avalonia ← WPF UI parity; optional MSA1 on Windows cores; A3/A5.
