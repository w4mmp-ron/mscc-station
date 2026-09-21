# CW sidetone — pop on first paddle after idle

**From:** Ron  
**To:** Stew  
**Date:** 2026-09-20  
**About:** Side Tone Generator sheet (Rev 6.0) — LM386 + VO1400 switching  
**Schematic file name may say “LPF & Amp”; the sheet title is Side Tone Generator.**

---

## The problem

After the keyer has been **idle for a while** (about **30 seconds or more**, maybe longer), the **first** paddle press makes a slight **pop** in the audio. After that, keying sounds fine until it sits idle again long enough for the pop to come back.

So this is not bad sidetone tone or bad paddles. It is a **first-key-after-rest** click.

---

## What the circuit is doing (short)

1. The PIC keyer makes **sidetone** with its NCO on the **`SIDE_TONE`** line.  
2. That goes through a small filter / level pot into an **LM386** (`U5`).  
3. **VO1400** chips (`U6`, `U7`) act as audio switches, steered by **`RX_CW`** / **`TX_CW`**, so sidetone (and RX audio) get into the phones/jacks at the right time.  
4. The LM386 output uses a large coupling cap (**220 µF** on pin 5) into the listen path.

---

## Why a long idle causes a pop

While everything sits quiet, DC voltages on high‑impedance audio nodes can **slowly drift**, or the big **220 µF** coupling cap can **slowly discharge**.

The **first** paddle after that rest often does two things at once:

- Turns **sidetone** on (NCO), and  
- Flips the **CW opto switches** when TX becomes active (`RX_CW` / `TX_CW`).

If two sides of a switch were at different DC, or the 220 µF was “empty,” that first connection dumps a little charge — you hear a **pop**. A moment later the circuit has settled, so the rest of the QSO is clean.

That “needs ~30+ seconds to come back” timing points to **slow analog drift / capacitor discharge**, not a one-shot firmware glitch.

---

## What to try (best chance first)

### 1) Highest chance — bleed across the VO1400 FET switches (try first)

On **U6** and **U7**, the switch is between **pin 3** and **pin 4** (not the LED pins 1 and 2).

**Add one resistor across pins 3–4 on U6, and one across pins 3–4 on U7.**

- **Start with 1 MΩ** each.  
- If the pop is still there, try **470 kΩ**.  
- If you hear a faint sidetone when not keying, go back toward **1 MΩ** or a bit higher.

**Why this might work:** With the FET open, the two sides can sit at different DC. The resistor keeps them nearly equal while idle, so the first close does not thump.

**Do not** put these resistors on pins **1–2** (LED / `RX_CW` / `TX_CW` drive). The **1.8 V zeners** on the LED side are for protecting the LED drive; they are separate from this fix.

### 2) Next — PIC firmware / `SIDE_TONE` idle (if #1 is not enough)

Sidetone comes from the PIC (**NCO on RC2 / `SIDE_TONE`**). Today the code turns the NCO **on** for key-down and **off** for key-up, and sets `RX_CW` / `TX_CW` when a keying session starts and ends.

Firmware ideas:

- When sidetone is off, make sure **`SIDE_TONE` is firmly quiet** (known off level), not in a odd state.  
- Optionally change **order**: start/stop tone vs flipping the optos so the first edge is gentler.  
- Optionally soften only the **first** element after a long idle.

This is a good second bet; the long idle time still makes the **hardware bleed (#1)** the better first experiment.

### 3) Also possible — the 220 µF on the LM386 output

There is already a **220 µF** on **U5 pin 5**. That cap can discharge over tens of seconds and thump on the first audio. If #1 helps only partly, add a **high-value bleed across that 220 µF** (e.g. **100 kΩ–470 kΩ**), or a few ohms in series with the jack. Do **not** remove the 220 µF.

---

## Suggested order for Stew

1. **1 MΩ across U6 pins 3–4 and U7 pins 3–4.**  
2. Idle **a minute or more**, first paddle — listen for the pop.  
3. If still there → PIC sidetone idle / ordering (**#2**).  
4. If still there → bleed across the **220 µF** (**#3**).

---

## Note

Ron has **not** confirmed a fix on the bench yet; this is the working theory and the preferred trial order.
