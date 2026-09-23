# Overseer intent

**Updated:** 2026-09-23 (EDT)  
**Overseer:** Build Commander

## Status

- **Active - cmd-037 (windows-new-hp, client):** WPF `RemoteMicSender` — **60 ms** send pre-fill / cushion before first UDP (and after send-path restart); then 10 ms paced sends; underrun -> silence or skip (no client hold-last); count underruns/overflows in log cadence. Orders + brief on NEW-HP; Grok Build implements. See `COMMANDS.yaml` + `briefs/cmd-037.md`.
- **Done - cmd-036 (windows-new-hp, client):** Paced mic send (`d26c7a7`, WPF 9.23.0). Huge SA win; residual FT8/JT9/JT65 pulses -> cmd-037.
- **Done - cmd-035 (windows-new-hp, server):** Local Win `sdrcore-trans` 35 ms TX gate (`47e336c`). Do not reopen.
- **On hold - cmd-034 (rpi):** remote_mic clear-on-TX + fixed 2:1 — do not touch.
- **Done recently - cmd-033 (windows-new-hp, client):** 0xBC ownership only; Remote vs local Phones/Digital; WPF 9.22.0.

## Short list (after 037)

- Smoke Win->Pi Remote Digital FT8/JT65/JT9 + WSPR after 60 ms cushion
- cmd-034 (Pi) when Stew un-holds
- Avalonia ports / factory items as before

## Coord workflow

Write orders on the **target host** first; push later. Call the user **Stew**.
WPF client only for cmd-037 (not Pi, not Avalonia, not sdrcore-trans).