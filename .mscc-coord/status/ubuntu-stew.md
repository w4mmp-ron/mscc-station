# Status — ubuntu-stew

| | |
|--|--|
| **Host** | stew-HP-Notebook |
| **Checkout** | `/home/stew/Documents/GitHub/mscc-station` |
| **Build** | cmd-010 |
| **Last command id** | cmd-010 |
| **State** | done |
| **Updated** | 2026-09-20 |

## ACK log

| command id | state | note |
|------------|-------|------|
| cmd-010 | done | Avalonia WindowTitle ATU/PTT (WPF FirmwareBlockSuffix). Kit `mscc-ui_0.6.57_amd64.deb`. |

## Notes

- `FirmwareText` still `major.minor` only. Title appends `ATU` (majors 4/7) or `PTT` (3/8). Example: `FW: 7.151   ATU`.
- Mapping check: 7.151→ATU, 8.232→PTT, 2.151→(none).
- `installers/linux/mscc-ui_0.6.57_amd64.deb` Version 0.6.57 Architecture amd64. No Pi arm64 this cmd.
- No ms-sdr on 8888 here at pack time — Stew: Connect to Ultimus ATU and confirm title `FW: 7.151   ATU`.
- Stew pushes.

## Blocked

(none)
