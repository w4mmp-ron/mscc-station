# Keyer CQ memory (CMD_SET_KEYER_MEMORY `0x9C`)

**Handoff:** see **`RESUME.md`** in this tree and **`ms-sdr-linux/RESUME.md`**.

## Path
```
Client / test  →  ms-sdr (UDP)  →  Proficio USB (usbvend01)  →  I2C  →  PIC keyer
```

## Parameter (one byte)
| Value | Action |
|-------|--------|
| `0` | **Play** current slot once (paddle aborts) |
| `1` | **Store begin** (clear builder for current slot) |
| `2` | **Store end** (RAM → EEPROM for current slot) |
| `3` | **Select slot** — **next** param is slot **0..3** (sticky) |
| `0x20`–`0x7E` | **Append** ASCII (max **48** chars per slot) |

**Slots:** 4. Default after reset: **0**. **Proficio:** no change.

## Select + store + play
```
0x9C, 3          // select
0x9C, 1          // slot 1
0x9C, 1          // begin
0x9C, 'C' …
0x9C, 2          // end
0x9C, 0          // play slot 1
```

## Play current slot
```
0x9C, 0
```

## Files touched
| Tree | Files |
|------|--------|
| `keyer/` | `main.c` |
| `Release-Proficio-MKII-PTT/` | `usbvend.h`, `usbvend01.c`, `cw.c`, `main.c`, `basic-plus.h` |
| `ms-sdr-linux/` | `usbavrcmd.h`, `main-controller.c` |
| `mscc-client/` | `Opcodes.cs` (constants only; no UI yet) |

## Build / flash
1. **PIC:** MPLAB X build `keyer` project → program PIC16F18326  
2. **Proficio:** PSoC Creator build / bootload MKII-PTT firmware  
3. **ms-sdr:** rebuild on Pi, copy to `mscc-binaries`, optional deb rebuild  

## Bench test without MSCC client (standalone USB)
On the Pi, with Proficio USB connected and **ms-sdr stopped**:

```bash
sudo apt install -y python3-usb
cd /path/to/keyer
sudo python3 keyer-mem-test.py
# default message: CQ CQ CQ DE W4MMP W4MMP W4MMP KN  then play

sudo python3 keyer-mem-test.py --play-only
sudo python3 keyer-mem-test.py --store-only
sudo python3 keyer-mem-test.py -m "TEST DE W4MMP"
```

USB: vendor OUT `bRequest=0x9C`, `wValue=0x071B`, `wIndex=0`,
**2 data bytes** `[param, seq]` (seq increments each transfer).
Proficio `Configure_CW` sees seq change → I2C to PIC. ~80 ms gap in test script.

## UDP test via ms-sdr (full path)
**ms-sdr running**, no other MSCC client on the session. From PC or Pi:

```bash
python3 keyer-mem-udp-test.py
# default host 192.168.12.199:8888
# default message: O O O O … (slot 0, WPM timing check) then play

python3 keyer-mem-udp-test.py --host proficio
python3 keyer-mem-udp-test.py --play-only
python3 keyer-mem-udp-test.py --store-only
```

Handshake `0xFE,1`, drain startup UDP, then one `0x9C` per param over UDP.
ms-sdr paces USB (`KEYER_MEM_USB_GAP_MS`). Does **not** send STOP.

### Set Farnsworth text WPM (`0x76`)
```bash
python3 keyer-mem-text-wpm.py 10              # text/overall 10 WPM
python3 keyer-mem-text-wpm.py 0               # off
python3 keyer-mem-text-wpm.py 10 --host proficio
python3 keyer-mem-udp-test.py --play-only     # hear gaps
```



## Farnsworth text WPM (`SET_MEM_TEXT_WPM` `0x76`)
Param: **0** = off; **1–4** → off; **5–60** = overall/text WPM.
Character elements stay on `SET_WPM`. If text ≥ char → off.

**ARRL / PARIS** gap math (memory play only):

- Word = **50** units: **31** element units @ char WPM + **19** spacing units  
- Spacing budget (char-dit equivalents): `S = (50×C − 31×T) / T`  
- Letter gap ∝ `3/19 × S`, word gap ∝ `4/19 × S` (so letter+word ≈ 7-unit word space)  
- Example: **C=18, T=10** → much more open than simple `×C/T` (overall ≈ text 10)

```bash
python3 keyer-mem-text-wpm.py 10   # text 10, char stays CW_Speed / SET_WPM
python3 keyer-mem-udp-test.py --play-only
```

## Note
**ms-sdr** paces each `0x9C` USB OUT with `KEYER_MEM_USB_GAP_MS` (40 ms) in
`Radio_send_parameters`, plus `KEYER_MEM_END_SETTLE_MS` (**400 ms**) after
STORE_END so PIC EEPROM can finish before PLAY (else I2C NACK drops play).
Helpers: `Keyer_Memory_Param` / `Select` / `Store` / `Play`.
Client may still send one UDP param at a time (same path). Bench script uses ~80 ms.

Proficio: USB writes 2 bytes `[param,seq]` → ring queue (80) → `Configure_CW` I²C.
PIC: abort play only on paddle open→close edge after play starts.


