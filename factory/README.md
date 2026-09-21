# Factory calibration trees

Canonical ship tables for TX IQ, freq (PPM), and QRP `power_cal`.
Live runtime files stay `iq.ini` / `freq_cal.ini` / `power_cal.ini` under `%LocalAppData%\MSCC-NET9\`.
Factory is **ship + Reset only**. Freq: copy when live `freq_cal.ini` is missing. IQ/QRP: user cache is `cal\<line>\` (see below); factory is used when that cache is missing (new line / Settings Reset).

```
factory/
  iq/<line>/iq.ini
  freq/<line>/freq_cal.ini
  power/<line>/power_cal.ini
```

| Product line | FW majors | Notes |
|--------------|-----------|--------|
| proficio-legacy | 1 | Stock no-PIN only. PPM +22.00. LF IQ/QRP = 0 |
| geminus-mkii | 2 | PPM ~0. LF IQ 1 / 4 |
| proficio-mkii | 3, 4 | PTT/ATU share tables. PPM ~0. LF = 0 |
| geminus-legacy | 5 | IQ same as Geminus MKII (LF 1 / 4). PPM +22.50 provisional |
| ultimus-legacy | 6 | PPM +27.66. LF = 0 |
| ultimus-mkii | 7, 8 | ATU/PTT share tables. PPM ~0. LF = 0 |

QRP `POWER_LEVEL` is **measured − 3** (clamp ≥ 0). No PIN-mod tables.

Windows ms-sdr looks for this tree next to `ms-sdr-MKII.exe` (`C:\mscc-net9\factory\…`).

**Reset paths (cmd-019):** FREQ CAL Reset and TX IQ Reset All force-copy from this tree for the **connected** FW major, then push/reload. Settings Reset does **not** copy `iq.ini` / `freq_cal.ini` / `power_cal.ini` / `recv-iq.ini` from generic `init-files`; those stay missing until the next ms-sdr start seeds them.

**Per-line user cache (cmd-020):** `%LocalAppData%\MSCC-NET9\cal\<line>\iq.ini` and `power_cal.ini` hold last user IQ/QRP for that product line (`cal\LAST_LINE.txt` remembers the last FW line). Switching radios stashes active → previous line, then loads that line’s cache (or factory if none). First upgrade with existing live files bootstraps them into `cal\<line>\` (does not wipe a tuned HF radio). **No freq user cache.** TX IQ Reset All and QRP Reset overwrite live **and** `cal\<line>\` from factory. Settings Reset deletes `cal\` so the next start seeds factory. Ordinary same-radio restart keeps QRP/IQ tweaks via write-through.
