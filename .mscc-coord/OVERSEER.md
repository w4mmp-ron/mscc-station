# Overseer intent

**Updated:** 2026-09-23  
**Overseer:** Build Commander

## Status

- **Active - cmd-034 (rpi):** Remote mic digi-safe bridge — clear bucket + short mute on TX ON; fixed 0.5 (2:1) fill; silence on underrun (no hold-last / no adaptive nudge). Mirror under linux/. Stew SA smoke NEW-HP→Pi Remote Digital CQ.
- **Reviewed - cmd-033 (windows-new-hp):** OK — 0xBC ownership only; Remote vs local audio independence (WPF 9.22.0).

## Short list (after 034)

- If SA still mushy: MSA1 wire / client WaveIn proof (optional WAV tap)
- Avalonia ports / backlog UI items
- Mode USB-on-load / digi LAST defaults (pending)

## Coord workflow

Write orders on the **target host** first; push later. Call the user **Stew**.
