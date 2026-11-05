# CW sidetone — pop on first paddle after idle

**From:** Ron  
**To:** Stew  
**Date:** 2026-09-20 (updated)  
**About:** Side Tone Generator sheet (Rev 6.0) — LM386 + VO1400 switching  
**Schematic file name may say “LPF & Amp”; the sheet title is Side Tone Generator.**

---

## The problem

After the keyer has been **idle for a while** — on the order of **minutes** (sometimes **many** minutes) — the **first** paddle press makes a slight **pop** in the audio. After that, keying sounds fine until it sits idle again long enough for the pop to come back.

Short gaps between characters do **not** bring it back. Only a **long rest** does. While discussing this, several minutes of idle often were not enough for the pop to return — wait long enough when testing.

This is not bad sidetone tone or bad paddles. It is a **first-key-after-long-rest** click.

---

## What the circuit is doing (short)

1. The PIC keyer makes **sidetone** with its NCO on the **`SIDE_TONE`** line.  
2. That goes through a small filter / level pot into an **LM386** (`U5`).  
3. **VO1400** chips (`U6`, `U7`) act as audio switches, steered by **`RX_CW`** / **`TX_CW`**, so sidetone (and RX audio) get into the phones/jacks at the right time.  
4. The LM386 output uses a large coupling cap (**C19, 220 µF** on **U5 pin 5**) into the listen path.

---

## Why a long idle causes a pop

While everything sits quiet for **minutes**, something in the analog path **slowly loses its settled DC**:

- The big **220 µF (C19)** can lose charge / the far side can float so the cap is no longer properly biased, and/or  
- The two sides of an open **VO1400** switch can drift to different DC.

The **first** paddle after that rest turns sidetone on and often flips the CW optos when TX becomes active. That first event dumps a little charge — **pop**. Then the path is settled for the rest of the session.

**Many minutes** of idle points strongly at a **large capacitor / slow drift**, not a quick firmware glitch.

---

## What is most likely (current ranking)

| Rank | Suspect | Why |
|------|---------|-----|
| **1 (top)** | **C19 220 µF** on LM386 out | Minutes-long idle matches a large coupling cap losing bias |
| **2** | **U6 / U7** FET first close | First paddle after rest also switches `RX_CW`/`TX_CW` once |
| **3** | PIC **`SIDE_TONE`** start | Still possible; weaker match for “many minutes” |

---

## Fixes to try

### 1) Top suspect — keep C19 biased (do this carefully)

**C19** stays in the circuit. Do **not** remove it.

**Wrong:** a resistor **across** C19 (both ends of the cap). That would **discharge** the cap faster and could make the pop come back **sooner**.

**Right:** hold the **jack / audio side** of C19 at a known DC (ground) so the cap **stays charged** while idle:

- One end of a new resistor → **far side of C19** (toward **`AUDIO_OUT` / phones / `SIDE_TONE_OUT`**, not U5 pin 5)  
- Other end → **GND**  
- Value: try about **100 kΩ–470 kΩ** (start near **220 kΩ**)

Optional if a little pop remains: a few ohms (**4.7–10 Ω**) in series with the jack.

### 2) Next — equalize the VO1400 switches

On **U6** and **U7**, the audio FET is between **pin 3** and **pin 4** (not LED pins 1 and 2).

Add **1 MΩ** from **pin 3 to pin 4** on U6, and the same on U7.  
If needed, try **470 kΩ**. If you hear faint sidetone when idle, go back toward **1 MΩ** or higher.

**Do not** put these on pins **1–2** (LED / `RX_CW` / `TX_CW`). The **1.8 V zeners** on the LED side are only LED protection.

### 3) If still needed — PIC firmware / `SIDE_TONE` idle

Sidetone is PIC NCO on **RC2 / `SIDE_TONE`**. Key down enables NCO; key up disables it. `RX_CW` / `TX_CW` flip when a keying session starts/ends.

Ideas: force **`SIDE_TONE` firmly quiet** when off; tweak order of tone vs optos; soften only the first element after a long idle.

---

## Suggested order for Stew

1. **C19 bias resistor:** far side of **C19 → GND** (~220 kΩ). Wait **many minutes**, first paddle — listen.  
2. If still popping → **1 MΩ on U6 and U7, pins 3–4**.  
3. If still popping → PIC **`SIDE_TONE`** idle / ordering.

---

## Note

Ron has **not** confirmed a fix on the bench yet. This is the working theory after clarifying that idle is **minutes**, and that bleeding **across** C19 would be the wrong move.
