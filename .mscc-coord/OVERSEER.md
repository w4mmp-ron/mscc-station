# Overseer intent

**Updated:** 2026-09-25  
**Overseer:** Build Commander

## Status

- **Active - cmd-041 + cmd-042 (DIG-U narrow Hi 1.4k/1.0k; Stew chose A):** cmd-041 WPF adds Hi idx 5=1.4k / 6=1.0k in DIG-U (+ 0xDD default-echo guard). cmd-042 SDRcore-recv 0xD1 cases 5/6 (recv 3.141). Order: NEW-HP (041 + 042 Windows mscc-recv.exe) -> Stew push -> ubuntu-stew mscc 1.0.45 amd64 -> Stew push -> rpi mscc 1.0.45 arm64. ms-sdr: no clamp, no edit. See `briefs/cmd-041.md` + `briefs/cmd-042.md`.
- **Active - cmd-040 (Avalonia, ubuntu-stew then rpi):** remote-audio parity 0.6.60 — independent RemoteDigitalAudio / PathPhones|PathDigital; Remote Digital mic slider (REMOTE_DIGI_MIC_VOL default 100); main Phones/Digital disabled while Remote (stricter than WPF); 0xBC ownership only (no PttOn/TuneMode from TxSetByServer). Orders on NEW-HP. See `COMMANDS.yaml` + `briefs/cmd-040.md`.
- **Peer - cmd-039 (rpi):** done — remote_mic diagnostic EVENT logging; fill unchanged; cmd-034 still on hold.
- **Peer - cmd-038 (NEW-HP WPF):** pending/orders — client Mic TX EVENT logging (diagnostic only).
- **On hold - cmd-034 (rpi):** remote_mic clear-on-TX + fixed 2:1 + silence on underrun — do not implement while logging / Avalonia parity ship.

## Short list

- **cmd-041 + cmd-042 NEW-HP first** (WPF + Windows recv), then Ubuntu 1.0.45, then Pi 1.0.45
- **cmd-040 Avalonia next** (ubuntu-stew amd64 smoke, then rpi arm64 kit)
- Digi mush parked (A/B + EVENT logs; no fill change)
- EVENT logging cleanup later (client cmd-038 / host cmd-039 correlators)
- Unhold cmd-034 later

## Coord workflow

Write orders on the **canonical host** (NEW-HP for shared repo yaml) or **target host** first; push later. Call the user **Stew**.
Avalonia Build prefer **ubuntu-stew**, then **rpi** kit. Do not push unless Stew asks.
