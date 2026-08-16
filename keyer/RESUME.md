# Keyer CQ memory — handoff / resume

**Date:** 2026-08-08  
**Tree:** `worktrees/keyer`  
**Detail protocol:** `KEYER-MEMORY.md` (read that first for opcodes and test scripts)

---

## Status

| Item | State |
|------|--------|
| PIC firmware | **Done** — 4 slots × 48 chars, select/store/play, paddle abort edge |
| Proficio | **Done** for `0x9C` — USB 2-byte `[param,seq]`, ring queue (80), I²C one byte at a time |
| ms-sdr (Linux) | **Done** for `0x9C` path + helpers + pacing; see `ms-sdr-linux/RESUME.md` |
| ms-sdr (Windows) | **Needs Stew** — mirror Linux: opcode, 2-byte USB pack, gap after each `0x9C` |
| MSCC client | Constants in `Opcodes.cs` only — **no CQ memory UI yet** |
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

Client guide: **`mscc-client/docs/KEYER-MEMORY-GUI-GUIDE.md`** (mockup: 4 slots, R=store, P=play).

## Resume prompt (paste)

> Keyer CQ memory handoff. Protocol in `keyer/KEYER-MEMORY.md` + `keyer/RESUME.md`. PIC + Proficio + Linux ms-sdr done; Windows ms-sdr needs 0x9C packing; client UI guide for Stew: `mscc-client/docs/KEYER-MEMORY-GUI-GUIDE.md`.
