# Overseer intent

**Updated:** 2026-09-17  
**Overseer:** Build Commander

## Standing goal

Coordinate Multus SDR **MSCC** client/server debugging across Windows, Ubuntu, and Raspberry Pi via this folder — not live chat between Grok Build instances.

## Current focus

Remote audio / **Remote WSJT-X** (Ubuntu client → Pi radio). Local WSJT-X on Pi and Ubuntu already working (2026-09-16 notes in `linux/` and `rpi/`).

## Notes for Builds

- Read `handoff.md` before editing or building.
- Canonical repo: `https://github.com/w4mmp-ron/mscc-station`
- Do **not** use the stale `MSCC-Grok-Build` tree.
- When blocked (need hardware, credentials, or a human decision), set status `blocked` and write a short reason — do not invent a workaround that changes protocol without an overseer command.
