# Status — rpi

| | |
|--|--|
| **Host** | raspberrypi |
| **Checkout** | `/home/pi/src/mscc-station` |
| **Build** | Grok Build |
| **Last command id** | cmd-007 pending (was cmd-005 ACK) (user said cmd-006; yaml has cmd-005) |
| **State** | done |
| **Updated** | 2026-09-18 |

## ACK log

| command id | state | note |
|------------|-------|------|
| cmd-007 | pending | Overseer: prefer Pulse VirtualB.monitor over ALSA VirtualB_monitor |
| cmd-005 | accepted | Yaml **cmd-005** (prefer Pulse PortAudio). User said cmd-006. This host is **rpi**, not ubuntu-stew. Did **not** write ubuntu-stew.md. |
| cmd-005 | running | PortAudioNative `/usr/local/lib`; mscc-ui LD_LIBRARY_PATH; FindNamedAfDevice → VirtualB.monitor; drop MSCC_Digi_Mic |
| cmd-005 | done | **mscc-ui 0.6.55 arm64** installed on this Pi. Remote Digital should list and keep **VirtualB.monitor**. |

## Notes

Pi client → Win11 host, Remote Digital: dropdown had no sticky VirtualB.monitor because Avalonia loaded Debian ALSA PortAudio and matcher fell through to item 0 / Digi_Mic remap.

- Prefer `/usr/local/lib/libportaudio.so.2` (Pulse+ALSA).
- Match `VirtualB.monitor` (normalize `_`/`.`); skip `MSCC_Digi_Mic`.
- Remote AF combo: do not default to first device if unmatched.
- Unloaded live `MSCC_Digi_Mic` remap.

Restart **mscc-ui**. Remote Digital mic combo should show and stay on **VirtualB.monitor**. Log line: `PortAudio: /usr/local/lib/libportaudio.so.2`.
