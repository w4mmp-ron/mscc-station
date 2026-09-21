# Status — rpi

| | |
|--|--|
| **Host** | raspberrypi |
| **Checkout** | `/home/pi/src/mscc-station` |
| **Build** | Grok Build |
| **Last command id** | cmd-031 |
| **State** | done |
| **Updated** | 2026-09-21 |

## ACK log

| command id | state | note |
|------------|-------|------|
| cmd-031 | accepted | Source already merged — rebuild only |
| cmd-031 | running | `make clean && make` sdrcore-trans |
| cmd-031 | done | Binary in `$HOME/mscc` + `rpi/mscc-binaries/`. String `remote_mic: stream reset (REMOTE path)` present. |
| cmd-029 | done | 0.6.59 kit + phones/gain |

## Notes

Confirmed in tree (no re-edit):
- `rpi/.../remote_mic.c:240` `remote_mic_reset_stream`
- `rpi/.../udp_thread.c:593` call on REMOTE_AUDIO / REMOTE_DIGITAL shared case
- linux/ mirrored

Rebuilt 17:56 local, PortAudio `/usr/local/lib`. Listen `:9101`.
Reset log line fires when client **enables** Remote Digital/Phones (not on trans boot). Enable Remote on Win11 to confirm; TUNE after Stop/Start without recycling trans.
