# MSCC WPF — Keyer CQ Memory GUI (guide for Stew)

**Date:** 2026-08-13  
**Audience:** Stew (Windows client / GUI)  
**Mockup:** Ron `Downloads/mscc.jpg` (CW tab annotations)  
**Backend protocol:** `worktrees/keyer/KEYER-MEMORY.md`, `keyer/RESUME.md`  
**Linux host:** `worktrees/ms-sdr-linux/RESUME.md`  
**Farnsworth delay:** §3 of this document (`0x76` / memory play gaps)  

---

## 1. Mockup → UI mapping (CW tab)

From the mockup (green boxes / yellow callouts):

| UI element | Role | Backend |
|------------|------|---------|
| **4 message text boxes** (rows) | Slot **0..3** message text (max **48** chars each) | Stored to PIC via `0x9C` |
| **R** button (per row) | **Record / store** that slot to the keyer | Multi-step `0x9C` (not “audio record”) |
| **P** button (per row) | **Play** that slot once | Select slot (if needed) + `0x9C` play |

Existing CW controls (MODE / SPACING / PADDLE / WEIGHT / PITCH / SPEED / HOLD / PHONES / QSK) stay as today; they use other opcodes (`0x71`, `0x75`, `0x73`, `0x77`, `0x7B`, `0x7A`, `0x72`, …).

**Product rules (Ron):**

- **Store** and **Play** are **separate** user actions (no auto-play after store).  
- **R** = program keyer EEPROM for that slot.  
- **P** = play that slot (paddle aborts on the radio/PIC).  
- Message text is **ASCII printable** only for the keyer (`0x20`–`0x7E`).  

---

## 2. Wire protocol (what the client must send)

UDP to ms-sdr (same as all other ops):

```text
[ opcode : 1 byte ][ payload : int16 little-endian ]
```

ms-sdr uses the **low byte** of the payload as the param.

### 2.1 CQ memory — `CMD_SET_KEYER_MEMORY` = **`0x9C`**

Already in `Opcodes.cs`:

```csharp
public const byte CMD_SET_KEYER_MEMORY = 0x9C;
public const int KEYER_MEM_PLAY = 0;
public const int KEYER_MEM_STORE_BEGIN = 1;
public const int KEYER_MEM_STORE_END = 2;
public const int KEYER_MEM_SELECT = 3;
```

| Param (byte) | Meaning |
|--------------|---------|
| `0` | **Play** current sticky slot once |
| `1` | **Store begin** (clears builder for current slot) |
| `2` | **Store end** (commit RAM → EEPROM) |
| `3` | **Select** — **next** param is slot **0..3** (sticky) |
| `0x20`–`0x7E` | **Append** one ASCII character (max **48** per slot) |

**Critical:** one transfer = one param. **Do not** send the whole string in one packet.

### 2.2 Farnsworth text WPM — **`0x76`** (see §3)

| Opcode | Name | Param (low byte) |
|--------|------|------------------|
| **`0x76`** | `SET_MEM_TEXT_WPM` | `0` = off; `5`–`60` = text/overall WPM |

Add to `Opcodes.cs` if missing:

```csharp
/// <summary>
/// Memory-play Farnsworth text/overall WPM (was SET_MEMORY_TYPE).
/// 0=off; 5–60=text WPM. Character elements stay on CMD_SET_CW_WPM (0x7B).
/// </summary>
public const byte CMD_SET_MEM_TEXT_WPM = 0x76;
```

---

## 3. Farnsworth delay (memory play only)

### 3.1 What it is

Two speeds (ARRL-style Farnsworth):

| Setting | Opcode | Role |
|---------|--------|------|
| **Character WPM** | `CMD_SET_CW_WPM` **`0x7B`** (existing SPEED control) | How fast each letter is *formed* (dits, dahs, gaps **inside** the letter) |
| **Text / Farnsworth WPM** | **`0x76`** `SET_MEM_TEXT_WPM` | How fast the *message goes by* — extra silence **between** letters and words on **CQ memory play only** |

Example: char **18**, text **10** → letters sound like 18 WPM; inter-letter/word spacing opened so overall rate is about **10** WPM.

### 3.2 What it does **not** affect

| Path | Farnsworth `0x76`? |
|------|---------------------|
| CQ memory **play** (`0x9C` play) | **Yes** — gaps use text WPM when active |
| Paddle keying | **No** |
| ELEMENT / LETTER spacing (`0x75`) | **No** — paddle letter-space assist only |
| Store (R) sequence | **No** — only timing of play |

### 3.3 Param rules (client should enforce)

| Param | Meaning |
|-------|---------|
| **`0`** | **Off** — standard PARIS packing at character WPM only |
| **`1`–`4`** | Invalid → treat as **0** (off); keyer also maps these to off |
| **`5`–`60`** | Text/overall WPM |
| **`> 60`** | Clamp to **60** |

**Runtime (radio/PIC):**

- If text WPM **`0`** or **≥ character WPM** → no stretch (same as off).  
- If **`5 ≤ text < char`** → ARRL/PARIS gap math on the PIC (31 element units @ char speed + 19 spacing units expanded to text speed).

### 3.4 Client send (when UI changes Farnsworth)

```text
0x76, T          // T = 0 (off) or 5..60
```

Same UDP framing as other CW params: `SendAsync(0x76, (short)T)`.

- Send when the user changes the control (not on every Play).  
- Host (ms-sdr) saves **`CW_Mem_Text_WPM`** in **`cw.ini`** and forwards to Proficio/PIC.  
- Character speed remains whatever SPEED / `0x7B` already is.

Suggested API:

```csharp
/// <param name="textWpm">0=off; 5–60=text WPM (clamped). Ignored for stretch if >= current char WPM.</param>
Task SetKeyerMemTextWpmAsync(int textWpm, CancellationToken ct = default);
```

### 3.5 Suggested UI (not on mockup yet)

Optional control on CW tab or SETTINGS, e.g.:

- Label: **“Memory text WPM”** / **“Farnsworth”**  
- Values: **Off** + **5…60** (or Off + spinner)  
- Hint: *“Character speed = SPEED. Text WPM opens gaps on memory play only.”*  
- Disable or grey when text ≥ SPEED (or auto-clamp / show “off while ≥ char speed”).

Persist client-side if desired; radio/`cw.ini` is source of truth after send.

### 3.6 Operator expectation

| User sets | Hears on **P** (play) |
|-----------|------------------------|
| SPEED 18, Farnsworth **Off** | Normal 18 WPM packing |
| SPEED 18, Farnsworth **10** | Letters at 18; much more open letter/word spacing (~overall 10) |
| SPEED 18, Farnsworth **18** or higher | Same as off |

ARRL practice files use the same idea (char speed vs effective/text speed). PIC implements PARIS-based gaps; ear may still differ slightly from a given ARRL MP3 — fine-tune later if needed.

### 3.7 Bench test (no GUI)

```bash
# Set text WPM 10, then play (char WPM already set on radio, e.g. 18)
python3 keyer-mem-text-wpm.py 10 --host <pi>
python3 keyer-mem-udp-test.py --host <pi> --play-only

python3 keyer-mem-text-wpm.py 0 --host <pi>   # off
```

---

## 4. Sequences the GUI must implement

### 4.1 Record (R) — store slot *n* from text box *n*

```text
0x9C, 3          // SELECT
0x9C, n          // slot 0..3
0x9C, 1          // STORE BEGIN
0x9C, 'C'        // each char of message (skip non 0x20-0x7E)
0x9C, 'Q'
...
0x9C, 2          // STORE END
```

- Truncate UI text to **48** characters before send.  
- After BEGIN, old longer messages are replaced (length-based on PIC).  
- Prefer `await` each send in order (no parallel flood). Host paces USB (~40 ms per `0x9C`; longer settle after END).  
- UI: disable R/P for that row (or whole panel) while store is in progress; show busy status.

### 4.2 Play (P) — play slot *n*

```text
0x9C, 3          // SELECT (recommended every play so row matches radio)
0x9C, n          // slot
0x9C, 0          // PLAY
```

- Do **not** require a store immediately before play if the slot was programmed earlier.  
- Abort is **paddle on the radio**, not a client opcode.  
- Play can take a long time at slow WPM; do not block the UI thread — fire-and-forget async is OK.

### 4.3 Suggested C# helper (MSCC.Core)

```csharp
// Pseudocode API to add on IRadioService / UdpRadioService

Task KeyerMemorySelectAsync(int slot, CancellationToken ct = default);
Task KeyerMemoryPlayAsync(int slot, CancellationToken ct = default);
Task KeyerMemoryStoreAsync(int slot, string text, CancellationToken ct = default);
Task SetKeyerMemTextWpmAsync(int textWpm, CancellationToken ct = default); // 0x76 Farnsworth
```

Implementation sketch for store:

```csharp
public async Task KeyerMemoryStoreAsync(int slot, string text, CancellationToken ct = default)
{
    slot = Math.Clamp(slot, 0, 3);
    text ??= "";
    if (text.Length > 48) text = text[..48];

    await SendKeyerMemAsync(Opcodes.KEYER_MEM_SELECT, ct);
    await SendKeyerMemAsync(slot, ct);
    await SendKeyerMemAsync(Opcodes.KEYER_MEM_STORE_BEGIN, ct);
    foreach (var ch in text)
    {
        int o = ch;
        if (o < 0x20 || o > 0x7E) continue;
        await SendKeyerMemAsync(o, ct);
    }
    await SendKeyerMemAsync(Opcodes.KEYER_MEM_STORE_END, ct);
}

Task SendKeyerMemAsync(int param, CancellationToken ct)
    => _transport.SendAsync(Opcodes.CMD_SET_KEYER_MEMORY, (short)(param & 0xFF), ct);
```

Same pattern as existing `SetCwWpmAsync` / `SendAsync(opcode, short)`.

---

## 5. Client-side state vs radio state

| Data | Where it lives |
|------|----------------|
| Message strings in the 4 text boxes | **Client** (INI recommended) — radio does not read back text |
| What is in PIC EEPROM | **Radio** after successful R |
| Sticky slot on PIC | Last SELECT; client should SELECT on each R/P |

**Recommend:** persist four strings in client settings, e.g.  
`%LocalAppData%\MSCC-NET9\` → `MSCC_Client.ini` or `cw_messages.ini`:

```ini
KeyerMem0=CQ CQ CQ DE W4MMP W4MMP W4MMP KN
KeyerMem1=...
KeyerMem2=...
KeyerMem3=...
```

On load: fill text boxes. On R: save INI + send store sequence.  
There is **no** “read memory from keyer” opcode — UI cannot refresh from radio.

---

## 6. Validation & UX

| Rule | Detail |
|------|--------|
| Max length | 48; show count `n/48` |
| Allowed chars | Prefer filter to `0x20`–`0x7E`; reject or strip others |
| Empty store | Allowed (BEGIN + END) → empty slot; play no-ops on PIC |
| Busy | Disable R/P while store runs; optional toast “Stored slot n” |
| Errors | If not connected / keyer not installed, log + status line (ms-sdr ignores `0x9C` if keyer off) |
| CW mode | User should be in CW for on-air keying; GUI may still allow store anytime when connected |

---

## 7. Host / Windows ms-sdr notes (Stew if touching backends)

Linux ms-sdr already:

- Packs **`0x9C` as 2 USB bytes** `[param, seq]`  
- Paces ~**40 ms** per `0x9C`; **~400 ms** after STORE_END  
- **`0x76`** `SET_MEM_TEXT_WPM` → USB + `cw.ini` `CW_Mem_Text_WPM`  

**Windows ms-sdr must mirror that**, or store/play will fail or drop PLAY.  
See `ms-sdr-linux/RESUME.md` (Stew checklist).

Client does **not** need long sleeps if host paces; sequential `await SendAsync` is enough. Optional small delay after store before enabling Play is fine.

---

## 8. Suggested implementation order

1. **UI** on CW tab: 4× `TextBox` + 4× **R** + 4× **P** (match mockup).  
2. **Client INI** load/save for the four strings.  
3. **`IRadioService`** methods + `UdpRadioService` send loop for `0x9C`.  
4. Wire **R** → `KeyerMemoryStoreAsync(slot, text)`.  
5. Wire **P** → `KeyerMemoryPlayAsync(slot)`.  
6. Status/logging via existing monitor text.  
7. **Farnsworth delay UI** (§3): control + `SetKeyerMemTextWpmAsync` / `0x76` (Off + 5–60).  
8. (Stew backend) Confirm Windows ms-sdr `0x9C` packing/pacing + `0x76` if Launch Servers uses Windows binaries.

---

## 9. Quick test without full GUI

```bash
# Against Linux ms-sdr (Pi), no other client session:
python3 keyer-mem-udp-test.py --host <pi> -m "CQ CQ CQ DE W4MMP W4MMP W4MMP KN"
python3 keyer-mem-udp-test.py --host <pi> --play-only

# Farnsworth text WPM then play:
python3 keyer-mem-text-wpm.py 10 --host <pi>
python3 keyer-mem-udp-test.py --host <pi> --play-only
python3 keyer-mem-text-wpm.py 0 --host <pi>
```

Same byte sequences the GUI must produce.

---

## 10. File touch list (client)

| Area | Likely files |
|------|----------------|
| Opcodes | `MSCC.Core/Protocol/Opcodes.cs` — add `CMD_SET_MEM_TEXT_WPM = 0x76` if missing |
| Service API | `IRadioService.cs`, `UdpRadioService.cs` |
| Transport | `UdpRadioTransport.SendAsync` (reuse) |
| CW tab UI | `MainWindow.xaml` / CW controls region |
| VM | `MainViewModel.cs` — `RecordKeyerMem`, `PlayKeyerMem`, Farnsworth text WPM |
| Settings | client INI helper for 4 strings (+ optional text WPM) |

---

## 11. Resume prompt (for Stew)

> MSCC WPF keyer CQ memory GUI. Mockup: 4 message rows + R (store) + P (play) on CW tab. Protocol: **0x9C** one param per UDP; SELECT+slot, BEGIN, chars, END; play SELECT+slot+PLAY. Max 48 ASCII. **Farnsworth delay:** **0x76** text WPM (0=off, 5–60); char speed stays **0x7B**; memory play only — see guide §3. No read-back from radio — persist strings client-side. Doc: `mscc-client/docs/KEYER-MEMORY-GUI-GUIDE.md`. Windows ms-sdr needs same 0x9C packing as Linux.

---

## 12. Mockup callouts (verbatim intent)

- **MESSAGE TEXT. 4 SLOTS** — four editable lines  
- **RECORD BUTTONS** — green **R** per slot → store to keyer  
- **PLAY BUTTONS** — green **P** per slot → play from keyer  

Sample text in mockup:  
`CQ CQ CQ DE W4MMP W4MMP W4MMP KN` (fits in 48 chars).
