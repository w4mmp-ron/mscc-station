# Status — windows-new-hp

| | |
|--|--|
| **Host** | NEW-HP-LAPTOP (Windows) |
| **Checkout** | `C:\Users\n8vet\OneDrive\Documents\GitHub\mscc-station` |
| **Build** | **cmd-014** |
| **Last command id** | cmd-014 |
| **State** | done |
| **Updated** | 2026-09-20 |

## ACK log

| command id | state | note |
|------------|-------|------|
| cmd-014 | done | owner re-ack sends packed 0xB2 + 0xB3 + status; ms-sdr VERSION_MINOR 170 → C:\mscc-net9 |
| cmd-009 | done | packed 0xB2 first-send + Multus log + WPF ATU/PTT |

## Notes

Windows `main-controller.c` Session_Is_Owner re-ack now matches Pi/Ubuntu: rebuild packed if 0, log `owner re-ack FW packed`, `Gui_send_param` 0xB2 then 0xB3 then status. No `Session_Claim` on re-ack. Claim path unchanged from cmd-009.

Built Release `ms-sdr-MKII.exe` (VERSION_MINOR 170), copied to `C:\mscc-net9` and `mscc-ui/Release/windows-wpf/`.

Live reconnect: this laptop is usually the remote client (no radio USB). Stew: Launch Servers + Connect → Disconnect client only → Connect again; title should keep `FW: x.y   ATU|PTT`; ms-sdr log `Session owner re-ack` + `owner re-ack FW packed`.
