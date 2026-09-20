# Status — windows-new-hp

| | |
|--|--|
| **Host** | NEW-HP-LAPTOP (Windows) |
| **Checkout** | `C:\Users\n8vet\OneDrive\Documents\GitHub\mscc-station` |
| **Build** | **cmd-009** |
| **Last command id** | cmd-009 |
| **State** | done |
| **Updated** | 2026-09-20 |

## ACK log

| command id | state | note |
|------------|-------|------|
| cmd-009 | done | packed 0xB2; Multus radio found; WPF ATU/PTT; ms-sdr 169 + WPF 9.20.0 in C:\mscc-net9 |

## Notes

Windows ms-sdr matches Linux: `G_firmware_version_packed`, srGetVersion pe0fko vs Proficio byte-order detect, one `Gui_send_param(CMD_GET_SET_FIRMWARE_VERSION, packed)` at both main-controller sites. Log: `srOpen . Multus radio found`. WPF WindowTitle appends ATU (majors 4/7) or PTT (3/8). Client decode unchanged.

Built: `ms-sdr-MKII.exe` (VERSION_MINOR 169), WPF **9.20.0** copied to `C:\mscc-net9`.

Live radio USB not present on this laptop (used as remote client). Title suffix unit: `7.151`→ATU, `8.232`→PTT, `2.151`→none. Stew: Start with Launch Servers + radio to confirm title `FW: x.y   ATU|PTT` and log packed hex / not `151.0`.
