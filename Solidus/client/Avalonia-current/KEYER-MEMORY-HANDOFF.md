# Keyer CQ memory — handoff (Avalonia UI → Ron / Build)

**Date:** 2026-08-13  
**Operator:** N8VET (Stew)  
**UI tree:** `Avalonia-Migration` (MSCC Avalonia client)  
**Latest client package:** `mscc-ui_0.6.36_arm64.deb`  
**Audience:** Ron / Build (firmware, Proficio, ms-sdr, end-to-end CW memory play)

---

## 1. One-line status

**Avalonia UI implements Ron’s CQ-memory protocol (`0x9C` store/play) and is considered correct on the client side.**  
USB-PTT works in voice modes; CW paddles work; **memory Play does not produce sidetone/RF.**  
Next work is **radio path** (PIC / Proficio / ms-sdr), not Avalonia commands.

---

## 2. What works on N8VET’s rig

| Check | Result |
|--------|--------|
| Servers start; Avalonia connects (local + remote) | OK |
| Voice modes: host **PTT** keys radio | OK |
| **TUN** | OK |
| Mode **CW**: **paddles** key / sidetone | OK |
| CW tab: edit 4 slots, sticky text, **R** store sequence “sent OK” | OK |
| CW tab: **P** play sequence “sent OK” | OK (client log only) |
| CW **P**: host PTT red / RF / keyer sidetone | **No** (expected: host PTT not used for CW PA) |
| Memory play actually keys like paddle | **Not yet** |

ALSA “Unknown PCM …” noise in `sdrcore-*.stdout` at startup is **unrelated** (device enumeration).  
Earlier `ms-sdr.stdout` capture was **empty** — use ms-sdr’s real log file for `0x9C` / `KEYER_MEMORY` if available.

---

## 3. Client version & deliverable

| Item | Value |
|------|--------|
| App version | **0.6.36** |
| Deb | `Avalonia-Migration/mscc-ui_0.6.36_arm64.deb` |
| Install on Pi | `sudo apt install -y ./mscc-ui_0.6.36_arm64.deb` then `mscc-ui` |
| Core project | `mscc-mscc/mscc-new/src/MSCC.Core` (shared by Avalonia) |
| UI project | `Avalonia-Migration/src/MSCC.Avalonia` |

**0.6.36 behavior note:** In **CW** mode, **P** does **not** assert host PTT (`0xBA`). That matches Proficio firmware: CW PA is keyed by the keyer line, not `TX_Main()` / USB-PTT.

---

## 4. Protocol the UI sends (must match)

UDP to ms-sdr (port 8888), same framing as all other ops:

```text
[ opcode : 1 byte ][ payload : int16 little-endian ]
```

ms-sdr uses the **low byte** of the payload as the param.  
**One param per packet** — never bulk string.

### 4.1 `CMD_SET_KEYER_MEMORY` = `0x9C`

| Param | Meaning |
|-------|---------|
| `0` | PLAY current sticky slot once |
| `1` | STORE BEGIN |
| `2` | STORE END → EEPROM |
| `3` | SELECT — **next** param is slot `0..3` |
| `0x20`–`0x7E` | Append ASCII (max **48** per slot) |

### 4.2 Record (**R**) — store slot *n* (no auto-play)

```text
0x9C, 3          // SELECT
0x9C, n          // slot 0..3
0x9C, 1          // STORE BEGIN
0x9C, 'C' …      // each printable char
0x9C, 2          // STORE END
```

### 4.3 Play (**P**) — play slot *n*

```text
0x9C, 3          // SELECT (every play so UI row matches radio)
0x9C, n          // slot
0x9C, 0          // PLAY
```

**No** `0xBA` (host PTT) in either sequence for CW.  
**No** client abort opcode — paddle open→close edge aborts on PIC.

### 4.4 Optional Farnsworth (API only; no CW-tab control yet)

| Opcode | Param |
|--------|--------|
| `SET_MEM_TEXT_WPM` / `0x76` | `0` = off; `5`–`60` = text/overall WPM for **memory play gaps only** |

Core already has `SetKeyerMemTextWpmAsync`. UI control deferred.

---

## 5. Avalonia UI (what Build can ignore for RF, but is done)

CW tab (under existing keyer mode/speed/etc.):

- **4 text boxes** — slots 0..3, max 48, `n/48` counts  
- **R** — store that slot  
- **P** — play that slot  
- Status line + main log / debug monitor (`Send KEYER_MEM 0x9C …`)  
- Sticky strings: `KEYER_MEM0`…`3` in client `mscc-avalonia.ini`  
- Busy lock during store  

Docs / mockup in this tree:

- `KEYER-MEMORY-GUI-GUIDE.md` (Ron’s guide, 2026-08-13)  
- `mscc.jpg` (R/P mockup)  
- This handoff: `KEYER-MEMORY-HANDOFF.md`

---

## 6. How RF is supposed to work (firmware sync)

Three different TX paths — do not mix them:

```text
VOICE PTT:   UI 0xBA → ms-sdr Set_PTT → USB 0x50 → TX_Request → TX_Main() → PA
             (Proficio main: TX_Main only when host mode != 'C')

CW paddle:   paddle → PIC → KEY_0A (and related) → Manage_Paddles_Port() → PA
             (Proficio main: only when host mode == 'C')

CW memory:   UI 0x9C PLAY → … → PIC keyer_play_message() → same KEY_0A path
             → Manage_Paddles_Port() → PA
             Host PTT is NOT in this chain.
```

**Implication:** “PTT does not activate on CW R-P” is **correct product/firmware behavior**.  
Success for **P** = PIC sidetone + CW RF like paddle, **not** red host PTT.

PIC play uses the same `set_keyer_out()` as paddle keying (KEY_0A low = mark, NCO sidetone).  
Empty EEPROM slot → play returns immediately (no RF, no tone).

Primary trees:

- `Linux work Tree/keyer/` — PIC (`main.c`, `KEYER-MEMORY.md`, `RESUME.md`, test scripts)  
- `Linux work Tree/Release-Proficio-MKII-PTT/` — Proficio USB `0x9C`, `Configure_CW`, I²C  
- `Linux work Tree/ms-sdr-linux/` — UDP `0x9C`, 2-byte USB pack, pacing  

---

## 7. Client Core API (already implemented)

`MSCC.Core`:

- `Opcodes.CMD_SET_KEYER_MEMORY` (`0x9C`) + `KEYER_MEM_*` constants  
- `Opcodes.SET_MEM_TEXT_WPM` (`0x76`)  
- `IRadioService` / `UdpRadioService`:  
  - `KeyerMemorySelectAsync`  
  - `KeyerMemoryStoreAsync`  
  - `KeyerMemoryPlayAsync`  
  - `SetKeyerMemTextWpmAsync`  

Debug: each `0x9C` param logged via `DebugMonitor` (`PLAY` / `STORE_BEGIN` / chars / etc.).

---

## 8. What Build / Ron should pick up

### Goal

End-to-end: Avalonia **R** then **P** in CW produces **same keying/sidetone behavior as paddle** for the stored message.

### Suggested verification order

1. Confirm flashed **PIC** is memory build (e.g. late hex under `keyer/Release/`) and **Proficio** is MKII-PTT with `0x9C` + seq + queue.  
2. Confirm **ms-sdr Linux** has 2-byte `[param,seq]` for `0x9C` and pacing after each transfer (+ settle after STORE_END).  
3. With UI **disconnected**, optional jigs:  
   - `keyer/keyer-mem-udp-test.py --host <pi>`  
   - or USB `keyer-mem-test.py` with ms-sdr stopped  
4. With Avalonia connected: short message → **R** → **P**; watch for PIC sidetone + PA.  
5. If silent: log ms-sdr for `CMD_SET_KEYER_MEMORY` / `KEYER_MEMORY` / failures; confirm EEPROM not empty; confirm seq changes on USB; confirm I²C not NACK-dropping PLAY after STORE_END.

### Likely failure zones (if paddles work but P does not)

| Zone | Why |
|------|-----|
| Empty slot on PIC | Store never committed / wrong slot |
| PLAY dropped after STORE_END | EEPROM busy / I²C NACK / insufficient settle |
| Seq not advancing | Proficio never queues new params |
| Old PIC/Proficio | `0x9C` ignored; paddles still work |
| Host PTT expectation | Red herring for CW RF |

### Out of scope for Avalonia (unless protocol changes)

- Reading memory back from radio (no get-memory opcode)  
- Client-side play abort opcode  
- Host PTT to key CW PA (Proficio does not apply `TX_Request` in CW)  
- Windows ms-sdr packing (Pi path is Linux ms-sdr)

---

## 9. Operator notes (N8VET)

- CW operator path is Ron’s domain; Stew is not deep CW.  
- Terminal UDP jigs were tried earlier; prefer proving path via **Avalonia** + firmware once UI commands were confirmed.  
- If Build needs a clean “client is done” statement: **yes — treat Avalonia 0.6.36 as the reference client for R/P sequences.**

---

## 10. Resume prompt (paste into Build)

> Keyer CQ memory handoff from N8VET Avalonia work. Client **0.6.36** implements Ron’s CW-tab UI (4×48, R store, P play) and sends correct `0x9C` sequences (SELECT/BEGIN/chars/END; SELECT/PLAY); sticky text client-side; no host PTT on CW Play (matches Proficio: CW PA via keyer line only). Voice USB-PTT and CW paddles work; **memory Play does not key yet**. Protocol: `keyer/KEYER-MEMORY.md`, UI guide: `Avalonia-Migration/KEYER-MEMORY-GUI-GUIDE.md`, this status: `Avalonia-Migration/KEYER-MEMORY-HANDOFF.md`. Pick up: PIC/Proficio/ms-sdr so PLAY drives same KEY_0A path as paddle. Optional later: Farnsworth `0x76` control on CW tab (Core API already present).

---

## 11. File map (quick)

| Area | Path |
|------|------|
| This handoff | `Avalonia-Migration/KEYER-MEMORY-HANDOFF.md` |
| Ron UI guide | `Avalonia-Migration/KEYER-MEMORY-GUI-GUIDE.md` |
| Mockup | `Avalonia-Migration/mscc.jpg` |
| Client deb | `Avalonia-Migration/mscc-ui_0.6.36_arm64.deb` |
| Core opcodes/API | `mscc-mscc/mscc-new/src/MSCC.Core/` |
| CW tab UI | `Avalonia-Migration/src/MSCC.Avalonia/Views/MainWindow.axaml` |
| VM R/P | `Avalonia-Migration/src/MSCC.Avalonia/ViewModels/MainViewModel.cs` |
| PIC + docs | `Linux work Tree/keyer/` |
| Proficio | `Linux work Tree/Release-Proficio-MKII-PTT/` |
| ms-sdr Linux | `Linux work Tree/ms-sdr-linux/` |
