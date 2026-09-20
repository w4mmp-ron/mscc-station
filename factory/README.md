# Factory calibration trees

Canonical ship tables for TX IQ, freq (PPM), and QRP `power_cal`.
Live runtime files stay `iq.ini` / `freq_cal.ini` / `power_cal.ini` under `%LocalAppData%\MSCC-NET9\`.
ms-sdr **copies** from here **only when the live file is missing** (first boot / Reset wipe). Ordinary restart does not overwrite user cal.

```
factory/
  iq/<line>/iq.ini
  freq/<line>/freq_cal.ini
  power/<line>/power_cal.ini
```

| Product line | FW majors | Notes |
|--------------|-----------|--------|
| proficio-legacy | 1 | Stock no-PIN only. PPM +22.00. LF IQ/QRP = 0 |
| geminus-mkii | 2 | PPM ~0. LF IQ −31/−34 |
| proficio-mkii | 3, 4 | PTT/ATU share tables. PPM ~0. LF = 0 |
| geminus-legacy | 5 | IQ same as Geminus MKII. PPM +22.50 provisional |
| ultimus-legacy | 6 | PPM +27.66. LF = 0 |
| ultimus-mkii | 7, 8 | ATU/PTT share tables. PPM ~0. LF = 0 |

QRP `POWER_LEVEL` is **measured − 3** (clamp ≥ 0). No PIN-mod tables.

Windows ms-sdr looks for this tree next to `ms-sdr-MKII.exe` (`C:\mscc-net9\factory\…`).
