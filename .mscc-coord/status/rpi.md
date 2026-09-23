# Status — rpi

| | |
|--|--|
| **Host** | raspberrypi |
| **Checkout** | `/home/pi/src/mscc-station` |
| **Build** | Grok Build |
| **Last command id** | cmd-032 |
| **State** | idle (await cmd-039 ACK) |
| **Updated** | 2026-09-23 |

## ACK log

| command id | state | note |
|------------|-------|------|
| cmd-032 | done | Debs in `installers/rpi/` and `rpi/Rpi-installers/` |
| cmd-031 | done | trans 17:56 stream reset |

## Notes

- Next: **cmd-039** in COMMANDS.yaml + briefs/cmd-039.md (finer remote_mic EVENT logging, diagnostic-only). ACK when Build starts.
- **cmd-034** remains ordered but **ON HOLD** — do not implement fill/clear-on-TX in 039.
- Live `$HOME/mscc/sdrcore-trans` still has cmd-031 reset string; periodic `remote_mic: pkt ok=` only (no EVENT yet). Log often shows `occ=16383` with silent overflow.
