# Status — rpi

| | |
|--|--|
| **Host** | raspberrypi |
| **Checkout** | `/home/pi/src/mscc-station` |
| **Build** | cmd-029 |
| **Last command id** | cmd-029 |
| **State** | pending |
| **Updated** | 2026-09-21 |

## ACK log

| command id | state | note |
|------------|-------|------|
| cmd-029 | pending | Avalonia 0.6.59 arm64 kit + remote phones 12000 + REMOTE_AUDIO 2.5× gain; rebuild sdrcore |
| cmd-012 | done | Owner re-ack resends 0xB2 FW + 0xB3 Core + status; no re-claim |
| cmd-011 | done | 0.6.57 arm64 kit |

## Notes

Pulled to `bffc94e` (cmd-027/028 Avalonia 0.6.59 source). Working tree still has intentional phones/gain diffs — commit then rebuild (do not discard).
Ron path: Win11 → RPi servers. Mic slider parked.
