# Overseer intent

**Updated:** 2026-09-23  
**Overseer:** Build Commander

## Status

- **Active - cmd-040 (Avalonia, ubuntu-stew then rpi):** remote-audio parity 0.6.60 — independent RemoteDigitalAudio / PathPhones|PathDigital; Remote Digital mic slider (REMOTE_DIGI_MIC_VOL default 100); main Phones/Digital disabled while Remote (stricter than WPF); 0xBC ownership only (no PttOn/TuneMode from TxSetByServer). Orders on NEW-HP. See `COMMANDS.yaml` + `briefs/cmd-040.md`.
- **Peer - cmd-039 (rpi):** done — remote_mic diagnostic EVENT logging; fill unchanged; cmd-034 still on hold.
- **Peer - cmd-038 (NEW-HP WPF):** pending/orders — client Mic TX EVENT logging (diagnostic only).
- **On hold - cmd-034 (rpi):** remote_mic clear-on-TX + fixed 2:1 + silence on underrun — do not implement while logging / Avalonia parity ship.

## Short list

- **cmd-040 Avalonia next** (ubuntu-stew amd64 smoke, then rpi arm64 kit)
- Digi mush parked (A/B + EVENT logs; no fill change)
- EVENT logging cleanup later (client cmd-038 / host cmd-039 correlators)
- Unhold cmd-034 later

## Coord workflow

Write orders on the **canonical host** (NEW-HP for shared repo yaml) or **target host** first; push later. Call the user **Stew**.
Avalonia Build prefer **ubuntu-stew**, then **rpi** kit. Do not push unless Stew asks.
