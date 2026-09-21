# Status — rpi

| | |
|--|--|
| **Host** | raspberrypi |
| **Checkout** | `/home/pi/src/mscc-station` |
| **Build** | **cmd-031** |
| **Last command id** | cmd-031 |
| **State** | pending |
| **Updated** | 2026-09-21 |

## ACK log

| command id | state | note |
|------------|-------|------|
| cmd-031 | pending | Source merged from NEW-HP `d0cb6b9`. Rebuild/install on Pi only — do NOT re-implement. |
| cmd-029 | done | Avalonia 0.6.59 arm64 + phones/gain |
| cmd-012 | done | Owner re-ack 0xB2/0xB3 |
| cmd-011 | done | 0.6.57 arm64 kit |

## Notes

`remote_mic_reset_stream()` is in rpi/ + linux/ sources (clears ring/g_frac/hist; UDP stays up).
Called from udp_thread REMOTE / REMOTE_DIGITAL case. Log: `remote_mic: stream reset (REMOTE path)`.

Finish:

```
cd /home/pi/src/mscc-station/rpi/SDRcore-trans-linux
make clean && make
# install to $HOME/mscc/ + rpi/mscc-binaries/; restart sdrcore-trans
```

Confirm log on Remote Digital enable.
