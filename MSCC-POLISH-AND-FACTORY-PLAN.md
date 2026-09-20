# MSCC — Polish & Factory Calibration Plan

**Status:** Design / backlog capture (2026-09-19…20). **No implementation until Stew clears** (Ron remote-digital ALC work first, then this list).  
**Audience:** Build Commander + Grok Build agents.  
**Repo:** `mscc-station`  
**Author capture:** Stew + Build Commander working session.

This document freezes decisions from the 2026-09-19 cal / settings discussion and adds Remote Digital mic-slider work from 2026-09-20. It does **not** embed full numeric lookup tables; those live in measured `*.ini` snapshots and will be filled into factory trees at implement time.

---

## 1. Goals (priority order)

1. **TX IQ factory by radio line (highest)**  
   Uncalibrated image is ~−30 dBc (unacceptable). Amplitude factory curves from on-hand radios should put units **in legal range** out of the box despite build variance. User can fine-tune.  
   Phase adjust is **back-burner** (amplitude is legal; phase buries residual SA spikes like PowerSDR/HDSDR).

2. **Freq (PPM) factory — ballpark only**  
   Legacy crystal can be hundreds of Hz off (auto-tune fails ~400 Hz). Target: **within ~50 Hz** OOB so auto FREQ CAL works; user fine-tunes.  
   MKII TCXO+Si5351: typically within ~2 Hz → factory **~0**.

3. **QRP / `power_cal` — ballpark, per-band**  
   **Not** a single flat default (e.g. “37 everywhere” can overdrive some bands). Per-band ship values a few points **below** measured, by radio line; user fine-tunes up.

4. **UX / identification polish**  
   Correct FW display, Multus log string, band gating from major, S/W HF vs LF/MF banks, sticky operate settings, Remote Digital mic slider, etc.

---

## 2. Firmware major = product identity

Shared USB VID/PID (`16C0:05DC`). **Product ID is firmware major** after `srGetVersion` (not the stale “Proficio found” string).

| Major | Product | Notes |
|------:|---------|--------|
| 1 | Proficio Legacy | Shipping |
| 2 | Geminus MKII | Shipping |
| 3 | Proficio MKII PTT | Same IQ/freq/QRP factory tables as major 4 |
| 4 | Proficio MKII ATU | Shared tables with 3 |
| 5 | Geminus Legacy | Done 2026-09-20 (`224` → `5`, minor `120`) |
| 6 | Ultimus Legacy | Done 2026-09-20 (seed from Proficio Legacy) |
| 7 | Ultimus MKII ATU | Done 2026-09-20; shared cal tables with major 8 |
| 8 | Ultimus MKII PTT | Done 2026-09-20; shared cal tables with major 7 |
| TBD | Maximus | HF+LF in one radio — later |

**PTT vs ATU:** distinct majors for client header; **same** factory `iq.ini` / `freq_cal` / `power_cal` per product line.

**Keil / PSoC builds:** Shack only (sole license).

**Repo layout:** `radio-psoc-firmware/` on Shack — Proficio / Geminus / Ultimus trees (flat `*-MKII-PTT` / `*-MKII-ATU`), plus `release/<radio>/` for shipping `.cyacd` / `.hex`.

---

## 3. Factory vs live files (core pattern)

Unchanged runtime model: DSP and UI keep using **one live file per concern**. Factory tables only **seed** or **Reset**.

```
On server start / radio identify
        │
        ▼
  FW major (CMD_GET_VERSION / packed 0xB2)
        │
        ├── map major → product line (PTT/ATU collapse)
        │
        ▼
  For each cal domain (IQ, freq, power):
        │
        ├── if live file missing OR Reset-to-factory OR radio line changed
        │         copy/seed from factory tree for that line
        │         → write live file
        │
        └── else
                  keep existing live file (user cal wins)
        │
        ▼
  Rest of program unchanged (reads live iq.ini / freq_cal.ini / power_cal.ini)
```

Remote mode: radio + live ini live on the **host**; client follows. Changing radio type → reboot server/client to re-identify major and reseed if needed.

---

## 4. TX IQ — load tree

### Live file (unchanged consumer)
- Path (Linux example): `~/iq.ini` (or platform AppData / `.local/share/mscc` equivalent)
- Format today: `RECORD=n,BAND=n,IQ_OFFSET=…` per band (amplitude only)
- Applied via `IQ_calc` → `iMult`/`qMult` (gain balance, no phase yet)

### Factory tree (new / to wire properly)
Today repo has `rev-*` folders and PCB-keyed defaults; target is **FW-major → product-line factory table**:

```
factory/
  iq/
    proficio-legacy/     ← major 1 (+ Legacy Proficio seeds)
    proficio-mkii/       ← majors 3,4 (shared)
    geminus-legacy/      ← major 5 (provisional: from Geminus MKII + pad)
    geminus-mkii/        ← major 2
    
    
```

(Exact folder names flexible; could also extend existing `rev-*` with a major→rev map.)

### Load sequence
```
srGetVersion → major
    → product_line = MapMajor(major)
    → factory_iq = factory/iq/<product_line>/iq.ini
    → if need_seed:
          copy factory_iq → live iq.ini
    → init_IQ_structure() reads live iq.ini as today
```

**Priority:** IQ factory is the main FCC/legal win. Exact band numbers come from 2026-09-19 on-hand batch (see §10); do not hardcode dumps in this doc.

**Back-burner:** per-band `IQ_PHASE` + DSP cross-term + UI slider (after amplitude factory ships).

---

## 5. Frequency cal — load tree

### Live file
- `freq_cal.ini`: `PPM_INT`, `PPM_DEC`, `DELTA`, `CALIBRATION_TEMPERATURE`
- User FREQ CAL (auto/manual) updates live file

### Today vs target
- Today: `Create_PPM_ini()` seeds once from **`G_pcb_version`**; `freq-cal/{proficio,geminus,…}` stubs are mostly zeros
- Target: seed from **FW major → product line**, same pattern as IQ

```
factory/
  freq/
    <product_line>/freq_cal.ini
```

```
major → product_line
    → if live freq_cal.ini missing OR Reset:
          seed factory/freq/<line>/freq_cal.ini → live
    → Init_PPM() reads live as today
```

**Targets:** MKII lines ~0; Legacy ballpark so residual ≲50 Hz (auto-tune usable). Sign/value per line from on-hand batch (§10).

**UI polish:** Auto FREQ CAL progress bar must **clear/reset** when starting a new Auto run (today can stay all-green from prior run).

---

## 6. QRP / power_cal — load tree

### Live file
- `power_cal.ini`: per-band `POWER_LEVEL`

### Factory
```
factory/
  power/
    <product_line>/power_cal.ini
```

**Rules:**
- Per-band values, a few points **below** measured for that line
- **No flat global default**
- PIN-diode-modded Legacy Proficio QRP is **not** the stock Legacy table (elevated drive); use stock no-PIN unit for ship QRP
- User fine-tunes upward from factory

Same seed/Reset/live pattern as IQ and freq.

---

## 7. User settings (not radio-dependent, except S/W + GEN)

### One band-keyed memory (extend HF → include MF/LF)
- Last-used freq/mode/filters: already 12 bands incl. 630/2200 on server; Avalonia `BandLastUsedStore` already includes them
- Favorites: one flat list with band field (client); server favs missing 630/2200 if still used
- Band stack: extend if needed
- **Do not** duplicate last-freq/favs for Geminus vs Proficio
- Stop wiping LF last-used slots on non-Geminus hosts (keep; just gray UI)

### Modes / filters / step / sliders
- Global sticky in client settings (not per radio type)
- Fix **USB-on-load**: restore per-band mode/filters first; keep digi modes; don’t collapse to USB

### What *is* banked by personality
- Spectrum S/W (waterfall/grid) — HF vs LF/MF
- GEN coverage index

---

## 8. Radio UI — bands & HF / LF-MF

**Decision (Option A + fallback button):**
- Show all band buttons; **gray** unavailable bands from FW major (or last-used personality before major known)
- **S/W bank follows active band** (HF band → HF bank; LF/MF → LF bank)
- Keep **HF / LF-MF button** as manual fallback override
- Last-used personality as fallback before major arrives
- Maximus later: all bands lit; auto band-mask off; S/W follows band or last-used

Rename mental model: control is **HF vs LF/MF display/gating**, not product marketing names (product stays in header via FW major).

---

## 9. Spectrum S/W ship defaults

- **Two banks only** at ship: one HF, one LF/MF (not per-band at ship)
- Goal: new user not washed out on LF/MF; same numbers = **Reset-to-defaults**
- Stew LF/MF working candidate (hold for final lock): baseline near right edge; −40; −100; gain 40; zero 0
- HF: keep existing field-tuned defaults
- Optional later: sparse per-band S/W overrides (perf fine; defer)

---

## 10. On-hand measurement batch (2026-09-19) — summary only

Measured on Shack via live `%LocalAppData%\MSCC-NET9\` after Stew FREQ / TX IQ / QRP. Full numbers retained in chat/memory; implementers re-read those inis or Stew’s notes when filling factory trees.

| Radio | Role for factory |
|-------|------------------|
| Geminus MKII | MKII freq ~0; IQ/QRP seed for Geminus MKII (+ pad → Legacy Geminus provisional) |
| Proficio MKII | MKII HF IQ curve; QRP per-band |
| Ultimus Legacy | Legacy-scale PPM; IQ/QRP for Ultimus Legacy line |
| Ultimus MKII | MKII PPM ~0; IQ/QRP; noted −50 dBc residual (phase later) |
| Proficio Legacy PIN-mod | IQ/freq useful; **QRP excluded** (diode loss) |
| Proficio Legacy stock | **Stock** Legacy Proficio IQ + QRP + PPM ballpark |
| Geminus Legacy | **No unit** — provisional PPM ~22–23; IQ/QRP from Geminus MKII + pad |

**IQ is the critical ship deliverable**; PPM/QRP only need ballpark.

---

## 11. Client / server identification fixes

1. **Windows `ms-sdr`:** send FW to client as **one packed `0xB2`** word (like Linux), not major then minor (fixes WPF showing `151.0` instead of `2.151`).
2. **Log string:** `Proficio found` → **`Multus radio found`** (shared VID/PID).
3. After majors reliable: auto-set HF/LF-MF personality from major (with last-used fallback); button remains manual override.

---

## 12. Remote Digital mic drive (2026-09-20)

**Context:** `mscc-remote-audio/STEW-REMOTE-DIGITAL-ALC-GAIN.md` (Ron).  
VB-Audio is a functional **1:1** pipe. Hot paths (e.g. Ron’s dual USB adapters) need attenuation. Do **not** add a global mic boost across all modes.

### Decision
1. **Do first:** **Enable Remote Digital mic drive slider** (today forced to 100% / disabled in digi).  
   - Default **100%** → VB / unity path unchanged  
   - User turns **down** for &gt;1:1 chains  
   - Same control pattern as Remote Phones (WPF + Avalonia)
2. **Optional later:** Mic boost checkbox (Remote Digital only, off by default, fixed +dB) only if unity users still cannot reach ALC after Digital mic gain + slider@100%.

Wire client capture gain (`RemoteAf.MicVolume`) from that slider in digi mode instead of hardcoding `1.0`.

---

## 13. Backlog checklist (implement after Ron clear)

### Firmware / identity (Shack Keil where noted)
- [x] `radio-psoc-firmware/` repo layout (Proficio / Geminus / Ultimus; flat MKII PTT/ATU; `release/` drops) — Shack 2026-09-20
- [x] Geminus Legacy major `224` → `5`, minor → `120` (rebuild + release drop)
- [x] Ultimus Legacy major `6` (USB product string Ultimus; Creator project name still Proficio-Legacy — rename deferred)
- [x] Ultimus MKII ATU major `7` + Ultimus MKII PTT major `8` (USB Ultimus; Creator names still Proficio-MKII-* — rename deferred)
- [ ] Windows ms-sdr packed `0xB2` (**cmd-009** on NEW-HP)
- [ ] Log: Multus radio found (**cmd-009**)
### Factory cal (server + factory trees)
- [ ] FW major → product line map
- [ ] Seed/Reset live `iq.ini` from factory/iq/&lt;line&gt;
- [ ] Seed/Reset live `freq_cal.ini` from factory/freq/&lt;line&gt;
- [ ] Seed/Reset live `power_cal.ini` from factory/power/&lt;line&gt; (per-band, below measured)
- [ ] Fill factory tables from 2026-09-19 batch (IQ first)

### Client UX
- [ ] WindowTitle ATU/PTT block from FW major (3/8=PTT, 4/7=ATU; **WPF in cmd-009**; Avalonia later)
- [ ] Gray bands from major / last-used; S/W follows band; HF/LF-MF button fallback
- [ ] S/W ship HF + LF/MF defaults (+ Reset); lock LF numbers when Stew confirms
- [ ] Last-used/favs/stack: keep MF/LF; stop LF wipe on non-Geminus
- [ ] USB-on-load / digi mode restore fix
- [ ] Auto FREQ CAL progress bar reset
- [ ] Enable Remote Digital mic slider (default 100%)

### Back-burner
- [ ] TX IQ phase per band
- [ ] Mic boost checkbox (only if needed)
- [ ] Sparse per-band S/W overrides
- [ ] Maximus band/S/W behavior

---

## 14. Explicit non-goals (for this pass)

- Changing how TX uses live `iq.ini` after load (seed only)
- Flat QRP default for all bands
- Global mic boost for all audio modes
- Implementing while Ron’s remote-digital ALC investigation is still in flight (unless Stew says otherwise)

---

## 15. Related docs

- `MSCC-FACTORY-CAL-NUMBERS.md` — per-radio / per-band measured values for `freq_cal.ini`, `iq.ini`, `power_cal.ini`

- `mscc-remote-audio/STEW-REMOTE-DIGITAL-ALC-GAIN.md` — Ron diagnosis (VB vs hot USB chain)
- `.mscc-coord/` — multi-host Build coordination
- `AGENTS.md` — Build watcher / sync policy

---

*End of plan capture. Update this file when decisions change; keep numeric tables out of git until factory trees are filled intentionally.*

## Locked 2026-09-20 — Ultimus majors + header ATU/PTT

- **Ultimus Legacy** major **6** — done (source + `release/Ultimus-Legacy/`)
- **Ultimus MKII ATU** major **7** — done
- **Ultimus MKII PTT** major **8** — done (distinct from ATU for client header)
- **Geminus Legacy** major **5**, minor **120** — done
- MSCC client title: add an **ATU** or **PTT** block next to `FW: major.minor`, derived from major (also map Proficio **3→PTT**, **4→ATU**). WPF + Avalonia — still open (UI list).