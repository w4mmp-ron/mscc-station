# MSCC — Factory Calibration Numbers (on-hand batch)

**Date captured:** 2026-09-19 (Shack, live `%LocalAppData%\MSCC-NET9\`)  
**Companion plan:** `MSCC-POLISH-AND-FACTORY-PLAN.md` (load trees / decisions)  
**Source:** Stew FREQ CAL + TX IQ + QRP on each radio; Build Commander read inis after save.

Band index order matches current `iq.ini` / `power_cal.ini` / server last-used enum:

| Index | Band |
|------:|------|
| 0 | 10 m |
| 1 | 12 m |
| 2 | 15 m |
| 3 | 17 m |
| 4 | 20 m |
| 5 | 30 m |
| 6 | 40 m |
| 7 | 60 m |
| 8 | 80 m |
| 9 | 160 m |
| 10 | 630 m |
| 11 | 2200 m |

LF slots (10–11) on HF-only radios may be **carry-over** from a prior Geminus session — do not treat as that HF radio’s LF factory unless noted.

---

## 1. `freq_cal.ini` (PPM)

Format: `PPM_INT`, `PPM_DEC` → PPM = INT + DEC/100; plus `DELTA`, `CALIBRATION_TEMPERATURE`.

| Radio | PPM_INT | PPM_DEC | DELTA | Temp | PPM | Stew residual | Factory use |
|-------|--------:|--------:|------:|-----:|----:|---------------|-------------|
| Geminus MKII | 2 | 0 | 0 | 37 | **+2.00** | ~0.13 Hz | Geminus MKII / MKII ballpark ~0 |
| Proficio MKII | *(file not rewritten; still +2.00 from prior)* | | | | **+2.00** | ~0.7 Hz | Proficio MKII ~0 |
| Ultimus Legacy | 27 | 66 | 0 | 35 | **+27.66** | ~8 Hz then ~4 Hz warm | Ultimus Legacy |
| Ultimus MKII | −1 | 0 | 0 | 41 | **−1.00** | — | Ultimus MKII ~0 |
| Proficio Legacy (PIN mod) | −21 | 0 | 0 | 44 | **−21.00** | — | Freq OK for Legacy Proficio ref; note sign vs stock |
| Proficio Legacy (stock, no PIN) | 22 | 0 | 0 | 0 | **+22.00** | — | **Preferred** Legacy Proficio PPM ballpark |
| Geminus Legacy | — | — | — | — | **+22 … +23** (provisional) | No unit on hand | Use ~+22.50 until measured |

**Ship intent:** MKII lines ≈ 0; Legacy lines ballpark so OOB ≲ ~50 Hz for auto-tune.

---

## 2. `iq.ini` — TX IQ amplitude (`IQ_OFFSET` by band)

### 2.1 Geminus MKII → factory `geminus-mkii` (+ pad → provisional `geminus-legacy`)

| Band | 10 | 12 | 15 | 17 | 20 | 30 | 40 | 60 | 80 | 160 | 630 | 2200 |
|------|---:|---:|---:|---:|---:|---:|---:|---:|---:|----:|----:|-----:|
| IQ_OFFSET | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | −31 | −34 |

### 2.2 Proficio MKII → factory `proficio-mkii` (majors 3 & 4 shared)

| Band | 10 | 12 | 15 | 17 | 20 | 30 | 40 | 60 | 80 | 160 | 630† | 2200† |
|------|---:|---:|---:|---:|---:|---:|---:|---:|---:|----:|-----:|------:|
| IQ_OFFSET | −80 | −67 | −48 | −40 | −24 | −7 | +6 | +13 | +19 | +25 | −31 | −34 |

† LF likely leftover from Geminus — ignore for Proficio MKII factory HF table (use 0 or omit LF).

### 2.3 Ultimus Legacy → factory `ultimus-legacy`

| Band | 10 | 12 | 15 | 17 | 20 | 30 | 40 | 60 | 80 | 160 | 630† | 2200† |
|------|---:|---:|---:|---:|---:|---:|---:|---:|---:|----:|-----:|------:|
| IQ_OFFSET | −111 | −83 | −78 | −55 | −45 | −22 | −10 | −8 | −2 | +4 | −31 | −34 |

### 2.4 Ultimus MKII → factory `ultimus-mkii`

| Band | 10 | 12 | 15 | 17 | 20 | 30 | 40 | 60 | 80 | 160 | 630† | 2200† |
|------|---:|---:|---:|---:|---:|---:|---:|---:|---:|----:|-----:|------:|
| IQ_OFFSET | +85 | +70 | +65 | +58 | +36 | +24 | +14 | +7 | −2 | −7 | −31 | −34 |

Stew note: worst-case image ~**−50 dBc** (legal); wants phase later to bury.

### 2.5 Proficio Legacy PIN-mod → IQ reference only (not QRP)

| Band | 10 | 12 | 15 | 17 | 20 | 30 | 40 | 60 | 80 | 160 |
|------|---:|---:|---:|---:|---:|---:|---:|---:|---:|----:|
| IQ_OFFSET | −148 | −138 | −105 | −89 | −71 | −53 | −38 | −30 | −23 | −14 |

### 2.6 Proficio Legacy stock (no PIN) → **preferred** factory `proficio-legacy`

Upper bands tweaked by Stew this pass:

| Band | 10 | 12 | 15 | 17 | 20 | 30 | 40 | 60 | 80 | 160 |
|------|---:|---:|---:|---:|---:|---:|---:|---:|---:|----:|
| IQ_OFFSET | −124 | −122 | −95 | −80 | −71 | −53 | −38 | −30 | −23 | −14 |

**IQ is the primary factory deliverable** (uncalibrated ~−30 dBc → legal with these curves).

---

## 3. `power_cal.ini` — QRP `POWER_LEVEL` by band

**Rule:** ship a few points **below** measured per band per radio line. No flat global default.

### 3.1 Geminus MKII

| Band | 10 | 12 | 15 | 17 | 20 | 30 | 40 | 60 | 80 | 160 | 630 | 2200 |
|------|---:|---:|---:|---:|---:|---:|---:|---:|---:|----:|----:|-----:|
| POWER_LEVEL | 32 | 21 | 21 | 21 | 25 | 19 | 18 | 19 | 19 | 33 | 40 | 47 |

### 3.2 Proficio MKII

| Band | 10 | 12 | 15 | 17 | 20 | 30 | 40 | 60 | 80 | 160 | 630† | 2200† |
|------|---:|---:|---:|---:|---:|---:|---:|---:|---:|----:|-----:|------:|
| POWER_LEVEL | 58 | 37 | 32 | 38 | 32 | 51 | 31 | 34 | 35 | 49 | 40 | 47 |

### 3.3 Ultimus Legacy

| Band | 10 | 12 | 15 | 17 | 20 | 30 | 40 | 60 | 80 | 160 |
|------|---:|---:|---:|---:|---:|---:|---:|---:|---:|----:|
| POWER_LEVEL | 35 | 27 | 24 | 28 | 23 | 24 | 27 | 25 | 52 | 54 |

### 3.4 Ultimus MKII

| Band | 10 | 12 | 15 | 17 | 20 | 30 | 40 | 60 | 80 | 160 |
|------|---:|---:|---:|---:|---:|---:|---:|---:|---:|----:|
| POWER_LEVEL | 30 | 21 | 21 | 20 | 24 | 18 | 17 | 20 | 19 | 42 |

### 3.5 Proficio Legacy PIN-mod — **exclude from stock ship**

(Diode loss → elevated drive, esp. 12/10)

| Band | 10 | 12 | 15 | 17 | 20 | 30 | 40 | 60 | 80 | 160 |
|------|---:|---:|---:|---:|---:|---:|---:|---:|---:|----:|
| POWER_LEVEL | 52 | 33 | 25 | 30 | 32 | 30 | 34 | 36 | **90** | **74** |

### 3.6 Proficio Legacy stock (no PIN) — **preferred** factory QRP

| Band | 10 | 12 | 15 | 17 | 20 | 30 | 40 | 60 | 80 | 160 |
|------|---:|---:|---:|---:|---:|---:|---:|---:|---:|----:|
| POWER_LEVEL | 39 | 32 | 28 | 37 | 23 | 36 | 36 | 34 | 34 | 48 |

### 3.7 Geminus Legacy (provisional)

Copy Geminus MKII `power_cal` with a small pad down per band (TX path ≈ MKII). No unit measured.

---

## 4. Which ini → which factory branch

```
Live (runtime)              Factory seed (by FW major → product line)
─────────────────           ─────────────────────────────────────────
iq.ini                   ←  factory/iq/<line>/iq.ini
freq_cal.ini             ←  factory/freq/<line>/freq_cal.ini
power_cal.ini            ←  factory/power/<line>/power_cal.ini
```

| Product line | FW majors (planned) | IQ source row | Freq source | Power source |
|--------------|---------------------|---------------|-------------|--------------|
| proficio-legacy | 1 | §2.6 stock (PIN §2.5 IQ-only backup) | §1 stock +22 (PIN −21 ref) | §3.6 stock only |
| proficio-mkii | 3, 4 | §2.2 | ~0 | §3.2 |
| geminus-mkii | 2 | §2.1 | ~0 / +2 | §3.1 |
| geminus-legacy | 5 | §2.1 + pad | +22…+23 provisional | §3.1 + pad |
| ultimus-legacy | 6 | §2.3 | +27.66 | §3.3 |
| ultimus-mkii | 7 | §2.4 | −1 / ~0 | §3.4 |

PTT/ATU share the same factory tables per line; majors differ for UI header only.

---

## 5. Notes

- `amplifier_cal.ini` was not used meaningfully this batch (zeros) — omit from factory for now.
- `recv-iq.ini` not part of this TX factory pass.
- When implementing ship QRP, subtract a few counts per band from the tables above (Stew: conservative, user fine-tunes up).
- Exact factory folder names should match `MSCC-POLISH-AND-FACTORY-PLAN.md` §4–6.

---

*End of numbers capture. Update when more radios are measured.*
