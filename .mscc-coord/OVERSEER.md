# Overseer intent

**Updated:** 2026-09-23 (EDT)  
**Overseer:** Build Commander

## Status

- **Active - cmd-036 (windows-new-hp, client):** WPF `RemoteMicSender` — dedicated capture thread; blocking 10 ms (480-frame) paced MSA1 UDP send (device clock). Orders + brief on NEW-HP; Grok Build implements. See `COMMANDS.yaml` + `briefs/cmd-036.md`.
- **Done - cmd-035 (windows-new-hp, server):** Local Win `sdrcore-trans` 35 ms TX gate (`47e336c`). Do not reopen.
- **On hold - cmd-034 (rpi):** remote_mic clear-on-TX + fixed 2:1 — do not touch.
- **Done recently - cmd-033 (windows-new-hp, client):** 0xBC ownership only; Remote vs local Phones/Digital; WPF 9.22.0.

## Short list (after 036)

- Smoke Win→Pi Remote Digital CQ skirts after paced mic send
- cmd-034 (Pi) when Stew un-holds
- Avalonia ports / factory items as before

## Coord workflow

Write orders on the **target host** first; push later. Call the user **Stew**.
WPF client only for cmd-036 (not Pi, not Avalonia unless shared helper forced, not sdrcore-trans).
