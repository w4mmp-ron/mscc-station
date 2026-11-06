# Keyer CQ memory — what the GUI does and what it sends on UDP

**Purpose:** Describe the **client GUI behavior** for CQ / keyer memories and the exact **UDP packets** it emits.  
**Reference UI:** Avalonia MSCC client (0.6.36+) — CW tab “CQ MEMORY” block.  
**Shared protocol stack:** `MSCC.Core` (`UdpRadioService` / `IRadioService`).  
**WPF port:** Same Core APIs; when WPF gets the CW memory UI it should match this document.  
**Backend protocol detail:** `Linux work Tree/keyer/KEYER-MEMORY.md`, `keyer/RESUME.md`.

---

## 1. Product intent (operator view)

| Action | Button | What the operator expects |
|--------|--------|---------------------------|
| Edit message | Text boxes (4) | Type CQ / exchange text for that slot (max 48 chars) |
| **R** | Record / store | Program that slot into the **keyer EEPROM** (not “audio record”) |
| **P** | Play | Key that slot **once** as CW (sidetone + RF via keyer path) |
| Abort play | Paddle | On the radio / PIC — **no client abort opcode** |

Rules Ron defined:

- **Store** and **Play** are **separate** — no auto-play after R.  
- Text is **client sticky** only — the GUI does **not** read memories back from the radio.  
- In **CW** mode, host **PTT is not used** for memory play (Proficio keys CW PA via the keyer line, same as paddles).

---

## 2. GUI layout (CW tab)

Under existing CW controls (mode, spacing, paddle, weight, pitch, speed, hold, phones, QSK):

```
CQ MEMORY
  [ Slot 0 text box ..................... ]  n/48  [ R ] [ P ]
  [ Slot 1 text box ..................... ]  n/48  [ R ] [ P ]
  [ Slot 2 text box ..................... ]  n/48  [ R ] [ P ]
  [ Slot 3 text box ..................... ]  n/48  [ R ] [ P ]
  status line (e.g. "Stored slot 0 (12 chars)")
```

| Control | Binding / role |
|---------|----------------|
| Text box *n* | Slot **n** (0..3), `MaxLength` 48 |
| Count | `Sanitize(text).Length/48` (printable only) |
| **R** | `RecordKeyerMem(slot)` → store sequence |
| **P** | `PlayKeyerMem(slot)` → play sequence |
| Busy | While store runs: panel R/P disabled (`KeyerMemBusy`) |
| Connected | R/P enabled only when session is connected |

### Client-side text rules (before any UDP)

1. Take the text box for that slot.  
2. Keep only printable ASCII **`0x20`–`0x7E`** (space through `~`).  
3. Truncate to **48** characters.  
4. Empty string after sanitize is allowed (store clears/replaces length on the PIC).

Sticky persistence (Avalonia): `KEYER_MEM0` … `KEYER_MEM3` in client settings ini.  
These are **not** sent as bulk over UDP — only used to fill the text boxes next launch.

---

## 3. UDP framing (all keyer memory packets)

Same as every other MSCC command to ms-sdr:

```text
UDP datagram to ms-sdr (default host:port from connection settings, often 127.0.0.1:8888)

  Byte 0:     opcode
  Bytes 1–2:  int16 payload, little-endian
```

For keyer memory:

| Field | Value |
|-------|--------|
| Opcode | **`0x9C`** (`CMD_SET_KEYER_MEMORY`) |
| Payload low byte | **one** parameter (see table below) |
| Payload high byte | **0** (client sends `(short)param`) |

**Critical:** **one UDP packet = one parameter.**  
Never put the whole message string in a single packet.

ms-sdr uses the **low byte** as the param and (on Linux) paces USB to Proficio.

### Parameter meanings (`0x9C` low byte)

| Param | Constant | Meaning |
|------:|----------|---------|
| `0` | `KEYER_MEM_PLAY` | Play **current sticky slot** once |
| `1` | `KEYER_MEM_STORE_BEGIN` | Start store; clear builder for current slot |
| `2` | `KEYER_MEM_STORE_END` | Commit RAM → EEPROM for current slot |
| `3` | `KEYER_MEM_SELECT` | Next param is **slot index 0..3** (sticky on PIC) |
| `0x20`–`0x7E` | (ASCII) | Append one character to the builder |

There is **no** “get memory” / read-back param.

---

## 4. What the GUI does on **R** (store)

### UI sequence

1. Require connected; ignore if already busy.  
2. Sanitize text for that slot.  
3. Set busy / status: `Storing slot n…`.  
4. Call Core: `KeyerMemoryStoreAsync(slot, text)`.  
5. Save sticky text to client settings.  
6. Clear busy; status e.g. `Stored slot n (k chars)`.

### UDP sequence emitted (ordered, awaited one after another)

Example: store slot **1**, text `"CQ DE N8VET"`:

| # | Opcode | Payload (LE short) | Low byte | Meaning |
|---|--------|--------------------|----------|---------|
| 1 | `0x9C` | `03 00` | `3` | SELECT |
| 2 | `0x9C` | `01 00` | `1` | slot = 1 |
| 3 | `0x9C` | `01 00` | `1` | STORE BEGIN |
| 4 | `0x9C` | `43 00` | `'C'` | append |
| 5 | `0x9C` | `51 00` | `'Q'` | append |
| 6 | `0x9C` | `20 00` | space | append |
| 7 | `0x9C` | `44 00` | `'D'` | append |
| 8 | `0x9C` | `45 00` | `'E'` | append |
| … | … | … | … | each remaining char |
| last | `0x9C` | `02 00` | `2` | STORE END |

Generic form:

```text
0x9C, 3          // SELECT
0x9C, n          // slot 0..3
0x9C, 1          // STORE BEGIN
0x9C, ch…        // each sanitized char (0x20–0x7E), max 48
0x9C, 2          // STORE END
```

### What **R** does **not** send

- No `0xBA` (host PTT)  
- No play (`0`) after store  
- No bulk string / multi-char payload  
- No Farnsworth `0x76` as part of store  

### Core implementation (reference)

`MSCC.Core` → `UdpRadioService.KeyerMemoryStoreAsync`:

1. `KeyerMemorySelectAsync(slot)` → SELECT + slot  
2. `STORE_BEGIN`  
3. foreach printable char → one `0x9C`  
4. `STORE_END`  

Each step is `await`ed (no parallel flood). Host-side USB pacing is ms-sdr’s job.

### Debug log lines (client)

Typical monitor / log messages:

```text
Keyer mem R slot n: "…" (k chars)
Send KEYER_MEM 0x9C SELECT
Send KEYER_MEM 0x9C param=n   (or numeric label)
Send KEYER_MEM 0x9C STORE_BEGIN
Send KEYER_MEM 0x9C 'C'
…
Send KEYER_MEM 0x9C STORE_END
Keyer memory store slot n complete
Keyer mem R slot n: store sequence sent OK
```

“Sent OK” means **UDP left the client**, not that PIC EEPROM committed or RF sounded.

---

## 5. What the GUI does on **P** (play)

### UI sequence (Avalonia 0.6.36 reference)

1. Require connected.  
2. If mode is **CW**: **do not** assert host PTT (`0xBA`).  
   - Comment in code: Proficio keys CW PA only via keyer line in CW mode.  
3. If mode is **not** CW: optional host PTT for voice-path TX (Avalonia has this branch; **WPF can omit** if CW-only product path).  
4. Call Core: `KeyerMemoryPlayAsync(slot)`.  
5. Status: play sent; paddle aborts on radio.

### UDP sequence emitted

Example: play slot **2**:

| # | Opcode | Payload (LE short) | Low byte | Meaning |
|---|--------|--------------------|----------|---------|
| 1 | `0x9C` | `03 00` | `3` | SELECT |
| 2 | `0x9C` | `02 00` | `2` | slot = 2 |
| 3 | `0x9C` | `00 00` | `0` | PLAY |

Generic form:

```text
0x9C, 3          // SELECT (every play so UI row matches sticky slot on PIC)
0x9C, n          // slot 0..3
0x9C, 0          // PLAY once
```

### What **P** does **not** send (CW path)

- **No** `CMD_SET_TX_ON` / `0xBA` host PTT when mode is CW  
- **No** re-store of text  
- **No** client “stop play” opcode  

### Core implementation (reference)

`KeyerMemoryPlayAsync(slot)`:

1. `KeyerMemorySelectAsync(slot)` → `0x9C,3` then `0x9C,n`  
2. `SendKeyerMemParamAsync(PLAY)` → `0x9C,0`  

### Debug log lines (client)

```text
Keyer mem P: CW mode — host PTT does not key PA; play uses keyer line
Keyer mem P slot n: SELECT + PLAY (0x9C)
Send KEYER_MEM 0x9C SELECT
Send KEYER_MEM 0x9C …
Send KEYER_MEM 0x9C PLAY
Keyer mem P slot n: play sequence sent OK (est. … ms)
```

Again: “sent OK” = UDP path from GUI; RF/sidetone depends on ms-sdr → Proficio → PIC.

---

## 6. Path after the GUI (for context only)

```text
GUI  --UDP 0x9C-->  ms-sdr  --USB 0x9C [param,seq]-->  Proficio  --I2C-->  PIC keyer
                                                              PLAY  -->  same KEY_0A path as paddle
```

| Layer | Responsibility |
|-------|----------------|
| **GUI** | Sequences above; sticky text; no bulk string |
| **ms-sdr** | Session, forward `0x9C`, pace USB, settle after STORE_END |
| **Proficio** | USB vendor OUT, seq queue, I²C to PIC |
| **PIC** | EEPROM slots, play timing, paddle abort |

Known issue (handoff): Avalonia sequences can be correct while **Play still produces no sidetone/RF** if firmware/path is incomplete — that is **not** fixed by reordering GUI packets if logs already show SELECT + PLAY.

---

## 7. Optional: Farnsworth text WPM (not on memory mockup)

Not part of R/P buttons; separate control if implemented later.

| Opcode | Payload low byte |
|--------|------------------|
| **`0x76`** `SET_MEM_TEXT_WPM` | `0` = off; `5`–`60` = text/overall WPM for **memory play gaps only** |

- Character element speed remains SPEED / `0x7B`.  
- Does **not** affect paddle keying.  
- Core: `SetKeyerMemTextWpmAsync`. Avalonia has API; CW-tab control optional.

---

## 8. Other CW opcodes (not memory, but same tab)

Existing CW controls keep their own opcodes (unchanged by memory UI):

| Area | Examples |
|------|----------|
| Speed | `0x7B` WPM |
| Spacing | `0x75` |
| Paddle / weight / hold / QSK / mode | `0x73`, `0x77`, `0x7A`, `0x72`, … |

Memory R/P only adds **`0x9C`** (and optionally **`0x76`**).

---

## 9. Summary table — GUI event → UDP

| GUI event | UDP packets (opcode, low-byte params in order) |
|-----------|--------------------------------------------------|
| Edit text only | **None** (local sticky only) |
| **R** on slot *n* | `0x9C: 3, n, 1, char…, 2` |
| **P** on slot *n* (CW) | `0x9C: 3, n, 0`  — **no** `0xBA` |
| Farnsworth change (if UI) | `0x76: T` once when control changes |

---

## 10. File map

| What | Where |
|------|--------|
| This document | `Avalonia-Migration/KEYER-MEMORY-GUI-UDP-BEHAVIOR.md` |
| Ron UI guide | `Avalonia-Migration/KEYER-MEMORY-GUI-GUIDE.md` |
| Avalonia handoff / issues | `Avalonia-Migration/KEYER-MEMORY-HANDOFF.md` |
| PIC / protocol | `Linux work Tree/keyer/KEYER-MEMORY.md`, `RESUME.md` |
| Core send helpers | `mscc-mscc/mscc-new/src/MSCC.Core/Services/UdpRadioService.cs` |
| Avalonia CW UI | `Avalonia-Migration/src/MSCC.Avalonia/Views/MainWindow.axaml` |
| Avalonia R/P handlers | `…/ViewModels/MainViewModel.cs` (`RecordKeyerMem`, `PlayKeyerMem`) |

---

## 11. Acceptance check for a GUI (Avalonia or WPF)

With a connected session and debug log open:

1. Type a short string in slot 0 → **no** UDP.  
2. Press **R** → log shows SELECT, slot 0, STORE_BEGIN, each char, STORE_END.  
3. Press **P** → log shows SELECT, slot 0, PLAY only (CW: no PTT on).  
4. Reopen app → text still in boxes (sticky).  

If (2) and (3) match and Play still is silent on air, hand off to ms-sdr / Proficio / PIC — not to “fix GUI payload packing.”

---

*Document describes current Avalonia + MSCC.Core behavior as the reference for the WPF memory UI port.*
