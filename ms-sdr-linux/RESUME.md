# ms-sdr Linux — keyer memory handoff / resume

**Date:** 2026-08-08  
**Tree:** `worktrees/ms-sdr-linux`  
**Related:** `keyer/KEYER-MEMORY.md`, `keyer/RESUME.md`, `opcodes.txt`

Build/run basics: `README-LINUX.md`.

---

## What landed (Linux)

### `CMD_SET_KEYER_MEMORY` (`0x9C`) — **live path**

| Piece | Location / behavior |
|-------|---------------------|
| Define | `source/usbavrcmd.h` |
| USB pack | `Radio_send_parameters`: **2 bytes** `[param, seq]`, rolling seq 1–255 |
| Pace | After each successful `0x9C`: `KEYER_MEM_USB_GAP_MS` (**40**); after STORE_END also `KEYER_MEM_END_SETTLE_MS` (**100**) |
| Helpers | `Keyer_Memory_Param` / `Select` / `Play` / `Store(slot, text)` in `main-controller.c` |
| UDP case | One UDP → one helper param; logs PLAY / BEGIN / END / SELECT / CHAR |
| Decl | `extern.h` |

**Only `0x9C` is paced** that way. Other opcodes unchanged.

Client (and UDP jig) should send **one opcode+param per transfer** (walk the string). Full string never one payload.

### `SET_MEM_TEXT_WPM` (`0x76`) — USB to Proficio; PIC apply later

| Piece | Behavior |
|-------|----------|
| Define | `usbavrcmd.h` (was `SET_MEMORY_TYPE`) |
| `cw_record.text_wpm` | `extern.h` |
| UDP set | Store + `Radio_send` if keyer installed + `Update_CW_ini` |
| Init | Sends `0x76` with `cw_record.text_wpm` |
| Proficio | `E_mem_text_wpm` + I²C pair (flash MKII-PTT with 0x76 support) |
| PIC | Farnsworth memory-play gaps implemented (flash keyer) |
| `cw.ini` | `CW_Mem_Text_WPM=0;` |

---

## Windows ms-sdr (Stew)

Mirror Linux for a working CQ memory path:

1. `CMD_SET_KEYER_MEMORY 0x9C` in command headers.
2. `Radio_send` (or equivalent): **2-byte** `[param, seq]` for `0x9C` only — not `sizeof(int)` alone.
3. Sleep/gap after each successful `0x9C` (~40 ms; extra ~100 ms after param `2` store end).
4. Optional: same `Keyer_Memory_*` helpers.
5. `SET_MEM_TEXT_WPM 0x76`: define + ini/GUI if desired — **do not USB** until firmware supports it.

Without (2)–(3), Proficio will not get reliable keyer memory traffic.

---

## Test without full client

```bash
# On PC or Pi; ms-sdr running; no other MSCC session
python3 path/to/keyer/keyer-mem-udp-test.py --host 192.168.12.199
# or --host proficio
```

See `keyer/KEYER-MEMORY.md`.

---

## Deploy reminder

Rebuild `ms-sdr` on the Pi → copy to `mscc-binaries` / package as usual.  
Rebuild deb only if shipping a release that should include this binary + updated seed `cw.ini`.

---

## Hardware family (`mscc.ini`)

| Key | Meaning |
|-----|---------|
| **`PROFICIO-MKII=1`** (default if missing) | MKII — PTT sense thread; full PIC keyer USB config |
| **`PROFICIO-MKII=0`** | Legacy — **no** PTT sense thread; **only** `SET_TX_HOLD` among keyer USB ops (no WPM/paddle/0x9C/…) |

Global: `G_proficio_mkii`. Parsed in `Parse_mscc_record` / `initialize_mscc`.

## Resume prompt (paste)

> ms-sdr Linux: keyer 0x9C + 0x76; `mscc.ini` **PROFICIO-MKII=0/1** gates PTT sense thread. See RESUME.md.
