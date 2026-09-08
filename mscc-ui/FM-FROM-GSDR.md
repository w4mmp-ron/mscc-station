# FM numbers from GSDR (for MSCC)

Reference only. **Do not port DttSP or GSDR WinForms.** Implement FM in MSCC `SDRcore-recv` / `SDRcore-trans` the same way as AM/SSB (`MODE_AM`, `ssb_modulate`, `am_modulate`).

Source: Genesis GSDR 2.0.18 (`console.resx` CTCSS list, `InitFilterPresets`, `chkCTCSS_CheckedChanged`).

Suggested new opmode (recv + trans, Windows and Linux copies):

```c
#define MODE_LSB  1
#define MODE_USB  2
#define MODE_AM   3
#define MODE_TUNE 4
#define MODE_CW   5
#define MODE_FM   6   /* add — not in MSCC today */
```

---

## CTCSS tones (Hz)

GSDR combo `comboFMCTCSSFreq` — 39 EIA/TIA-603 tones. Default **67.0**. Encode amplitude in GSDR was **0.05** of full scale (`DttSP.SetCTCSSAmplitude(0, 0.05)`).

```
67.0  69.3  71.9  74.4  77.0  79.7  82.5  85.4  88.5
91.5  94.8  97.4  100.0 103.5 107.2 110.9 114.8 118.8
123.0 127.3 131.8 136.5 141.3 146.2 151.4 156.7 162.2
167.9 173.8 179.9 186.2 192.8 203.5 210.7 218.1 225.7
233.6 241.8 250.3
```

EIA tones **not** in GSDR (optional extras): `199.5  206.5  229.1  254.1`

C array:

```c
static const float ctcss_hz[] = {
    67.0f,  69.3f,  71.9f,  74.4f,  77.0f,  79.7f,  82.5f,  85.4f,  88.5f,
    91.5f,  94.8f,  97.4f, 100.0f, 103.5f, 107.2f, 110.9f, 114.8f, 118.8f,
   123.0f, 127.3f, 131.8f, 136.5f, 141.3f, 146.2f, 151.4f, 156.7f, 162.2f,
   167.9f, 173.8f, 179.9f, 186.2f, 192.8f, 203.5f, 210.7f, 218.1f, 225.7f,
   233.6f, 241.8f, 250.3f
};
```

---

## RX filter presets (Hz, low/high around 0)

GSDR shares **FMN** presets with AM/SAM/DSB. Last-used default **F5**.

| Button | Label | Low | High | Width |
|--------|-------|-----|------|-------|
| F1 | 16k | −8000 | 8000 | 16 kHz |
| F2 | 12k | −6000 | 6000 | 12 kHz |
| F3 | 10k | −5000 | 5000 | 10 kHz |
| F4 | 8.0k | −4000 | 4000 | 8 kHz |
| F5 | 6.6k | −3300 | 3300 | 6.6 kHz |
| F6 | 5.2k | −2600 | 2600 | 5.2 kHz |
| F7 | 4.0k | −2000 | 2000 | 4 kHz |
| F8 | 3.1k | −1550 | 1550 | 3.1 kHz |
| F9 | 2.9k | −1450 | 1450 | 2.9 kHz |
| F10 | 2.4k | −1200 | 1200 | 2.4 kHz |

**WFM** (broadcast-wide; Proficio sample rate may not support the widest):

| Button | Label | Low | High |
|--------|-------|-----|------|
| F1 | 180k | −90000 | 90000 |
| F2 | 150k | −75000 | 75000 |
| F3 | 120k | −60000 | 60000 |
| F4 | 100k | −50000 | 50000 |
| F5 | 80k | −40000 | 40000 |
| F6 | 60k | −30000 | 30000 |
| F7 | 48k | −24000 | 24000 |
| F8 | 32k | −16000 | 16000 |
| F9 | 24k | −12000 | 12000 |

For ham NFM on Proficio (48/96 kHz IQ), use FMN F4–F6 (~5–8 kHz). Ignore WFM unless you add a wide IF later.

---

## Repeater offset (GSDR)

`RPTRmode`: `low = 0`, `simplex`, `high`.

On TX with FMN:

- simplex — no shift
- low — `tx_freq -= RPTR_offset`
- high — `tx_freq += RPTR_offset`

`RPTR_offset` is `udFMOffset.Value / 1000` in **MHz** (UI value 600 → 0.600 MHz). Common US 2 m: **0.600 MHz**; 70 cm often **5.000 MHz**. GSDR default offset is 0 until the operator sets it.

---

## Deviation (not a GSDR user table)

GSDR does not expose peak deviation in the UI; DttSP FMN is roughly ham NFM. For MSCC, pick explicit values:

| Use | Peak deviation | Notes |
|-----|----------------|--------|
| NFM / ham | 2.5 kHz or 5 kHz | 5 kHz is common US amateur FM |
| CTCSS | ~10% of peak, or ~0.05 FS like GSDR | Keep tone in the 0–300 Hz region after demod |

TX: NCO phase increment `2*pi * (dev_hz * audio) / fs`.  
RX: quadrature discriminator, then de-emphasis (~750 µs US amateur) and CTCSS high-pass if needed.

---

## Where to implement in this repo

| Layer | Paths |
|-------|--------|
| RX demod | `SDRcore-recv-linux/` and `mscc-ui/windows-work-tree/SDRcore-recv/` |
| TX modulator | `SDRcore-trans-linux/` and `mscc-ui/windows-work-tree/SDRcore-trans/` |
| Opcode / mode enum | `MSCC.Core` (keep Win/Linux servers in lockstep) |
| UI | WPF under `windows-work-tree/mscc-mscc/`, then Avalonia |

Do not copy GSDR `audio.cs` FMN branches or DttSP `EnableCTCSS` / `SetCTCSSOscFreq`.
