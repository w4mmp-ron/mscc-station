# Overseer intent

**Updated:** 2026-09-18  
**Overseer:** Build Commander

## Status

- **Active — cmd-006 (ubuntu-stew):** Fix Avalonia Remote Digital VAC matching.
  - FindNamedAfDevice snaps `VirtualB.monitor` → bare `VirtualB` (StartsWith bug) — mic combo won’t stick.
  - Prefer `/usr/local/lib` mscc-portaudio (Pulse); mscc-ui launcher LD_LIBRARY_PATH.
  - Keep ALSA PCM name `VirtualB_monitor` (no dots). Ship 0.6.55.
- cmd-005 superseded (folded into 006). cmd-002–004 done.
- Spectrum pumping (→ Shack) cleared earlier by redeploying current Shack servers.

## Digi cable (Pi)

MSCC play → VirtualA · WSJT In → VirtualA.monitor · WSJT Out → VirtualB · MSCC mic → VirtualB.monitor

## Notes

Overseer writes instructions; Builds code/pack. One Build at a time. No ini workarounds (no MSCC_Digi_Mic as required UI name).
