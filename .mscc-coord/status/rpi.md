# Status — rpi

| | |
|--|--|
| **Host** | raspberrypi |
| **Checkout** | `/home/pi/src/mscc-station` |
| **Build** | cmd-031 |
| **Last command id** | cmd-031 |
| **State** | pending |
| **Updated** | 2026-09-21 |

## ACK log

| command id | state | note |
|------------|-------|------|
| cmd-031 | pending | Code already done on NEW-HP (`d0cb6b9`). Pi: pull after Stew pushes, then rebuild/install only — do NOT re-implement. |
| cmd-029 | done | Avalonia 0.6.59 arm64 + phones/gain |
| cmd-012 | done | ms-sdr owner re-ack 0xB2/0xB3 |
| cmd-011 | done | 0.6.57 arm64 kit |

## Notes

Source commit lives on NEW-HP (not origin yet): `remote_mic_reset_stream` + udp_thread call in linux/ + rpi/.
After `git pull` (merge with local cmd-029 commits if needed): rebuild sdrcore-trans only, install, verify log line.
