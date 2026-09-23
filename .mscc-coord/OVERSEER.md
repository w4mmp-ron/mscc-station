# Overseer intent

**Updated:** 2026-09-23  
**Overseer:** Build Commander

## Status

- **Active - cmd-039 (rpi):** `remote_mic` **diagnostic-only** finer EVENT logging (under/hold-last, overflow/drop, adaptive step, low occ, optional UDP gap); keep ~5 s summary; **ms in message body**; greppable `remote_mic EVENT`. Pi/`rpi/` only. No fill algorithm change. See `COMMANDS.yaml` + `briefs/cmd-039.md`.
- **On hold - cmd-034 (rpi):** remote_mic clear-on-TX + fixed 2:1 + silence on underrun — **do not implement** while logging ships.
- **Peer (NEW-HP, not this host):** cmd-038 WPF Mic TX EVENT logging (client) — do not redo here.

## Short list (after 039)

- Stew JT65 smoke with greppable Pi EVENT + client EVENT (after 038) vs Spike bump time
- cmd-034 when Stew un-holds (after logging correlation)
- Avalonia / backlog as before

## Coord workflow

Write orders on the **target host** first; push later. Call the user **Stew**.
Pi remote_mic logging only for cmd-039 (not linux/ parity, not WPF, not cmd-034 fill).
