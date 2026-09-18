# Status — rpi

| | |
|--|--|
| **Host** | raspberrypi |
| **Checkout** | `/home/pi/src/mscc-station` |
| **Build** | Grok Build |
| **Last command id** | cmd-007 |
| **State** | done |
| **Updated** | 2026-09-18 |

## ACK log

| command id | state | note |
|------------|-------|------|
| cmd-007 | accepted | Prefer Pulse VirtualB.monitor over ALSA VirtualB_monitor |
| cmd-007 | running | FindNamed +50 Pulse / −10 ALSA; 64ch penalty; always re-resolve digi; 0.6.56 |
| cmd-007 | done | **mscc-ui 0.6.56** installed. Combo shows `[PulseAudio]` / `[ALSA]`. Log: `api=PulseAudio`. |
| cmd-006 | done | 0.6.55 stick (`e15697a`) |

## Notes

ALSA `VirtualB_monitor` capture peak ~70; Pulse `VirtualB.monitor` ~20000. Saved REMOTE_MIC_DEV=9 was ALSA.

Restart **mscc-ui**. Remote Digital mic should be **VirtualB.monitor [PulseAudio]**. Log line must include `api=PulseAudio`, not ALSA.

WSJT Out remains Pulse **VirtualB**. digital-microphone.ini still `VirtualB.monitor`.
