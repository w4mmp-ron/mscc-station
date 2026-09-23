# Status — ubuntu-stew

| | |
|--|--|
| **Host** | stew-HP-Notebook |
| **Checkout** | `/home/stew/Documents/GitHub/mscc-station` |
| **Build** | cmd-040 |
| **Last command id** | cmd-040 |
| **State** | done |
| **Updated** | 2026-09-23 |

## ACK log

| command id | state | note |
|------------|-------|------|
| cmd-040 | done | Avalonia 0.6.60 remote-audio parity; amd64 + arm64 kits |
| cmd-032 | done | mscc_1.0.44_amd64.deb with remote_mic stream reset |
| cmd-028 | done | Avalonia 0.6.59 FW band gate |
| cmd-027 | done | Avalonia idle + DIG-U + FW title |

## Notes

Kits: `installers/linux/mscc-ui_0.6.60_amd64.deb` and `installers/rpi/mscc-ui_0.6.60_arm64.deb`.
Path Phones/Digital independent (`RemoteDigitalAudio`); REMOTE_DIGI_MIC_VOL default 100;
main Phones/Digital disabled while Remote; 0xBC ownership only (no PttOn/TuneMode).
Live Remote/WSJT 0xBC smoke still for Stew after install. Do not push.
