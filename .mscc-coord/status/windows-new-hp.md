# Status — windows-new-hp

| | |
|--|--|
| **Host** | NEW-HP-LAPTOP (Windows) |
| **Checkout** | `C:\Users\n8vet\OneDrive\Documents\GitHub\mscc-station` |
| **Build** | **cmd-015** |
| **Last command id** | cmd-015 |
| **State** | done |
| **Updated** | 2026-09-20 |

## ACK log

| command id | state | note |
|------------|-------|------|
| cmd-015 | done | WPF 9.20.1 band gate from FW major; INI until FW; manual override kept |
| cmd-014 | done | owner re-ack 0xB2+0xB3 |

## Notes

`FirmwareVersionReported` → map major: 2/5 Geminus (LF on, HF gray); 1/3/4/6/7/8 Proficio-family (HF on, LF gray); unknown/"--" keep RADIO_MODEL INI. Sets RadioModelButton, SwitchRadioModelWaterfall, ApplyRadioModelBandGating, SyncGenButtonForRadioModel. Manual RadioModelButton still toggles. Reconnect uses cmd-014 FW re-send.

WPF **9.20.1** in `C:\mscc-net9`. Stew: Connect Proficio/Ultimus (3/7) → HF live, 2200/630 gray; Geminus 2/5 → opposite; Disconnect/Connect keeps gating; button still flips.
