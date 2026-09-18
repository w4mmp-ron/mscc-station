# Overseer intent

**Updated:** 2026-09-18  
**Overseer:** Build Commander

## Status

- **Active — cmd-007 (rpi):** Prefer **Pulse** VAC devices over ALSA `VirtualB_monitor` / `VirtualA`.
  Proven: ALSA monitor capture peak ~70; Pulse `VirtualB.monitor` peak 20000 with tone into VirtualB.
  MSCC was on REMOTE_MIC_DEV=9 (ALSA) → silent digi TX.
- 0.6.55 (`e15697a`): FindNamed stick + PortAudio `/usr/local` — sticky OK; wrong host API remained.
  Build ACKed that work as cmd-005 (yaml cmd-006 naming miss).

## Digi cable

WSJT Out → Pulse **VirtualB** · MSCC mic → Pulse **VirtualB.monitor** (not ALSA VirtualB_monitor)

## Coord workflow

Write orders on the **target host** first; push later to sync others.
