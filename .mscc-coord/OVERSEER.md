# Overseer intent

**Updated:** 2026-09-23 (EDT)  
**Overseer:** Build Commander

## Status

- **Active - cmd-038 (windows-new-hp, client):** WPF `RemoteMicSender` - **diagnostic-only** finer Mic TX EVENT logging (underrun / drop / low-cushion / optional WaveIn gap); keep ~5 s summary; ms stamps in message body; Client **9.23.2**. No audio/cushion change. Orders + brief on NEW-HP; Grok Build implements. See `COMMANDS.yaml` + `briefs/cmd-038.md`.
- **Done - cmd-037 (windows-new-hp, client):** 60 ms prefill cushion (`5f39a16`, WPF 9.23.1). Stew: looks great; rare JT65 bump ~35 s / ~8 s before end; drops=0 underruns=0 often -> cmd-038 logging.
- **Done - cmd-036 (windows-new-hp, client):** Paced mic send (`d26c7a7`, WPF 9.23.0).
- **Done - cmd-035 (windows-new-hp, server):** Local Win `sdrcore-trans` 35 ms TX gate (`47e336c`). Do not reopen.
- **On hold - cmd-034 (rpi):** remote_mic clear-on-TX + fixed 2:1 - do not touch.
- **Done recently - cmd-033 (windows-new-hp, client):** 0xBC ownership only; Remote vs local Phones/Digital; WPF 9.22.0.

## Short list (after 038)

- Stew A/B Win client -> Ubuntu host while Build ships EVENT logging
- Capture next JT65 bump with greppable `Mic TX EVENT` lines + Spike time
- cmd-034 (Pi) when Stew un-holds
- Avalonia ports / factory items as before

## Coord workflow

Write orders on the **target host** first; push later. Call the user **Stew**.
WPF client only for cmd-038 (not Pi, not Avalonia, not sdrcore-trans). Diagnostic logging only.
