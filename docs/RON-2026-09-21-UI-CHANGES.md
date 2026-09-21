# MSCC UI changes for Ron — 2026-09-21

**Audience:** Ron (Win11 WPF client → Pi servers)  
**Windows client:** **R9-21-7** / Client **9.21.7** (`installers/windows/mscc-net9-R9--21-7-install.exe`)  
**Pi servers (pair with this client):** **mscc 1.0.44** (`installers/rpi/` or `rpi/Rpi-installers/`)  
**Pi / Ubuntu Avalonia UI (optional):** **0.6.59**

This is what changed in the **UI** (and closely related remote-audio host bits) since the **R9-17-4** baseline you have. Upgrade steps: [`RON-UPGRADE-R9-17-4-to-R9-21-7.md`](RON-UPGRADE-R9-17-4-to-R9-21-7.md).

---

## What you’ll notice in WPF (Client 9.21.x)

### Window / radio identity
- **Title bar** shows the **product name from firmware major** (Geminus / Proficio / Ultimus lines), including ATU/PTT wording where that applies — not a generic placeholder.

### Idle / Start / Stop
- With the radio **stopped / idle**: VFO shows **freq 0**, and **band / mode chrome stays clear** (no leftover 40m / USB from an old session).
- Last-used band/freq is only saved while the radio is **actually running** with a real frequency — stops an old “poison” last-used from writing itself back on Stop.

### DIG-U / mode stickiness (important for digi)
- **DIG-U** (and other last-used modes) **stick across restart** instead of collapsing to USB on load.
- After Start, if your last-used mode for that band was **DIG-U**, the UI **restores DIG-U** after the startup frequency lands (no USB flash-and-stick).
- Band **last-used mode wins** over a stale HF “last mode” memory — fixes DIG-U ↔ USB **ping-pong** when flipping bands / restarting.

### Connect / FW
- If **FW / Core** don’t show up after Connect, the client **asks** whether to re-request identity (Yes = ask again; No = keep what you have).

### LAST HF / LF connect defaults
- Fresh / empty LAST banks lean toward popular digi: **14.074 MHz** (HF) and **474.2 kHz** (LF/MF), instead of bare SSB park freqs.

### FREQ CAL
- Starting a new **Auto / Check / Reset** run **clears** the old all-green progress bar so you don’t think a prior run is still current.

### Remote Digital mic drive (new today)
- **Remote Digital** mic **slider is enabled** again (was forced to 100% and locked).
- Default **100%**; value persists as **`REMOTE_DIGI_MIC_VOL`**.
- Use this to **turn down** a hot VAC / WSJT chain without touching host gain pots.

### S/W + bands (already in the 9.21 line)
- **S/W follows band** with separate **HF** and **LF/MF** banks (Geminus LF/MF usable when FW allows).
- Bands **gray out** from firmware major (manual override still available).
- **2200m / 630m** last-used keys are kept (not wiped on HF radios).

---

## Avalonia (Pi / Ubuntu UI) — if you use it

**0.6.59** tracks the WPF polish for this block:
- Idle UI (freq 0 / clear band-mode)
- DIG-U band-wins rules
- FW-driven window title
- FW major → band gate + S/W bank (same idea as WPF)

Remote Digital **mic slider** on Avalonia is **not** in 0.6.59 yet (WPF-only for now).

---

## Related Pi server bits (not UI chrome, but you’ll feel them)

Ship these with the client upgrade (**mscc 1.0.44**):

| Item | Why it matters |
|------|----------------|
| **Remote phones headroom + REMOTE_AUDIO line gain** | Cleaner remote phones AF; less clipping on the remote path |
| **`remote_mic` stream reset on REMOTE / REMOTE_DIGITAL open** | Soft-resets the host mic ring / resampler when you enable Remote Digital — reduces “mushy TUNE until I recycle the Pi servers” after client swaps / SESSION IN USE fights |

**Still good practice:** after a **SESSION IN USE** fight or swapping which PC owns the session, **recycle Pi servers once** before chasing VAC.

---

## What did *not* change for you today
- PortAudio / init-gui package versions (leave them unless Stew says otherwise).
- Avalonia Remote Digital mic slider (deferred).
- PSoC firmware flash (separate path; only if Stew sends you a new `.cyacd`).

---

## Quick smoke after upgrade
1. WPF **About / title** shows **9.21.7** (or R9-21-7 installer).
2. Stop → idle shows **0** / no band-mode ghost; Start restores your band + **DIG-U** if that was last-used.
3. Remote Digital: slider moves; TUNE into WSJT looks clean on the SA; Stop/Start Remote Digital without recycling Pi still OK.
4. Optional: Connect with servers stopped → FW ask dialog behaves as expected.
