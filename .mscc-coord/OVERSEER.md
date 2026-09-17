# Overseer intent

**Updated:** 2026-09-17  
**Overseer:** Build Commander

## Standing goal

Coordinate Multus SDR **MSCC** client/server debugging across Windows, Ubuntu, and Raspberry Pi. Standing rules: root `AGENTS.md`.

## Status notes

- **cmd-002 (rpi config):** Phone SSB power now reasonable after Pi `power.ini` / ALC / remote-phones align. RX ~10 dB floor = **Pi hardware noise** (Norman investigating). Do not chase AF source diffs for that.
- **Next code:** Avalonia audio mode UI must match WPF — three buttons (Phones | Digital, Remote underneath), not Phones/Digital toggle + Remote checkbox. See **cmd-003**.

## Notes for Builds

- One Build at a time. Pull at session start after Norman publishes. Push after real code.
- Ubuntu UI work → `mscc-ui/Avalonia-Migration/`. Do not edit `rpi/` for this.
