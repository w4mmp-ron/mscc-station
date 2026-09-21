# Status — rpi

| | |
|--|--|
| **Host** | raspberrypi |
| **Checkout** | `/home/pi/src/mscc-station` |
| **Build** | Grok Build |
| **Last command id** | cmd-029 |
| **State** | done |
| **Updated** | 2026-09-21 |

## ACK log

| command id | state | note |
|------------|-------|------|
| cmd-029 | accepted | 0.6.59 kit + phones/gain |
| cmd-029 | running | source commit; rebuild recv/trans; publish 0.6.59 |
| cmd-029 | done | `installers/rpi/mscc-ui_0.6.59_arm64.deb`; cores in `$HOME/mscc` + `rpi/mscc-binaries/` |
| cmd-012 | done | ms-sdr owner re-ack 0xB2/0xB3 |
| cmd-011 | done | 0.6.57 arm64 kit |

## Notes

- Source: recv `*12000`; trans REMOTE_AUDIO 2.5× (linux/ mirrored). Commit `7ffa2d3`.
- Rebuild: `sdrcore-recv` / `sdrcore-trans` 16:51 local; PortAudio `/usr/local/lib`. Stack running.
- Kit: Package mscc-ui Version **0.6.59** Architecture **arm64**. Installed on this Pi.
- Smoke: radio USB `16c0:05dc`, FW **3.232** (MKII-PTT family — HF, not Geminus LF). CAT `/dev/tnt0`. `remote_mic` listen `:9101`.
- Win11→Pi remote digi TUNE / phones play ~30%: for Stew/Ron at the Shack client (servers ready here).
- Mic slider out of scope. Not pushed.
