# Overseer intent

**Updated:** 2026-09-23 (EDT)  
**Overseer:** Build Commander

## Status

- **Active - cmd-035 (windows-new-hp, server):** Local Win `sdrcore-trans` clear/mute gate on `CMD_SET_TX_ON` 0->1 for phones/digital only. Survey + orders on NEW-HP; Grok Build implements. See `COMMANDS.yaml` + `briefs/cmd-035.md`.
- **On hold - cmd-034 (rpi):** remote_mic clear-on-TX + fixed 2:1 - do not touch.
- **Done recently - cmd-033 (windows-new-hp, client):** 0xBC ownership only; Remote vs local Phones/Digital; WPF 9.22.0.

## Short list (after 035)

- Observe whether Win->Pi remote digi skirts improve after local clear/mute
- cmd-034 (Pi) when Stew un-holds
- Avalonia ports / factory items as before

## Coord workflow

Write orders on the **target host** first; push later. Call the user **Stew**.
Windows servers only for cmd-035 (not WPF, not Avalonia).
