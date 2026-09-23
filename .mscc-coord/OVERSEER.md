# Overseer intent

**Updated:** 2026-09-23  
**Overseer:** Build Commander

## Status

- **Active - cmd-039 (rpi):** `remote_mic` diagnostic-only finer EVENT logging (under/hold-last, overflow/drop, adaptive step, low occupancy, optional UDP gap); keep the ~5 s summary; milliseconds in the message body; greppable `remote_mic EVENT`. Pi/`rpi/` only. No fill algorithm change. See `COMMANDS.yaml` + `briefs/cmd-039.md`.
- **On hold - cmd-034 (rpi):** remote_mic clear-on-TX + fixed 2:1 + silence on underrun — do not implement while logging ships.
- **Peer (NEW-HP):** cmd-038 pending/orders (client EVENT logging); cmd-037, cmd-036, and cmd-035 done.

## Short list (after 039)

- Stew Build/smoke cmd-039 EVENT logs vs Spike
- Optional cmd-038 client EVENT correlation
- Unhold cmd-034 later
- Avalonia / backlog as before

## Coord workflow

Write orders on the **target host** first; push later. Call the user **Stew**.
Pi remote_mic logging only for cmd-039 (not linux/ parity, not WPF, not cmd-034 fill).
