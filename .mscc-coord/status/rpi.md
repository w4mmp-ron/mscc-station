# Status — rpi

| | |
|--|--|
| **Host** | raspberrypi |
| **Checkout** | `/home/pi/src/mscc-station` |
| **Build** | **cmd-031** |
| **Last command id** | cmd-031 |
| **State** | blocked |
| **Updated** | 2026-09-21 |

## ACK log

| command id | state | note |
|------------|-------|------|
| cmd-031 | accepted | reset remote_mic ring/g_frac on REMOTE path open |
| cmd-031 | running | source in rpi/ + linux/ SDRcore-trans-linux |
| cmd-031 | blocked | make/install/restart needs raspberrypi (SSH refused from this session) |
| cmd-012 | done | Owner re-ack resends 0xB2 FW + 0xB3 Core + status; no re-claim |
| cmd-011 | done | 0.6.57 arm64 kit |

## Notes

Source is in git (cmd-031). `remote_mic_reset_stream()` clears g_w/g_r, g_hist0/1, g_frac, g_under; socket/thread stay up.
Called from udp_thread CMD_SET_AUDIO_DEVICE shared REMOTE_AUDIO / REMOTE_DIGITAL_AUDIO case.
Log: `remote_mic: stream reset (REMOTE path)`.

On the Pi after `git pull`:

```
cd /home/pi/src/mscc-station/rpi/SDRcore-trans-linux
make clean && make
cp <bin> $HOME/mscc/ and rpi/mscc-binaries/
restart sdrcore-trans
```

Then confirm log on Remote Digital enable and TUNE after client Stop/Start without host recycle.
