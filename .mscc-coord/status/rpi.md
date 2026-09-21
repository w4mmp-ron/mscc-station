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
| cmd-031 | pending | remote_mic_reset_stream on REMOTE/REMOTE_DIGITAL open; mirror linux/ |
| cmd-029 | done | Avalonia 0.6.59 arm64 + phones/gain |
| cmd-012 | done | ms-sdr owner re-ack 0xB2/0xB3 |
| cmd-011 | done | 0.6.57 arm64 kit |

## Notes

Stew: soft-reset ring/g_frac/hist without tearing UDP — insurance for sticky mush after client swaps (Ron path). Mic slider N/A here.
