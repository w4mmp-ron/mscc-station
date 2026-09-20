# Overseer intent

**Updated:** 2026-09-20  
**Overseer:** Build Commander

## Status

- **Verified — cmd-017 / cmd-017b:** category swap + per-category S/W banks OK.
- **Next up:** Remote Digital mic slider (Ron cleared) — WPF first.

## Short list (next spins)

1. **LAST_HF / LAST_LF ship defaults** → **14.074 MHz** / **474.2 kHz** (popular digital; Stew 2026-09-20). Currently 14.000 / 475.000 in cmd-017 constants.
2. Remote Digital mic slider (default 100%, allow attenuate).
3. Avalonia port block (band-gate / S/W banks / title) when WPF theme settles.
4. Client ask-if-FW-missing after Connect.
5. USB-on-load mode stick (digi not collapsing to USB).
6. Factory IQ/freq/power seed — still held unless Stew expands.

## Coord workflow

Write orders on the **target host** first; push later. Call the user **Stew**.