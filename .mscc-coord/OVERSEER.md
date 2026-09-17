# Overseer intent

**Updated:** 2026-09-17  
**Overseer:** Build Commander

## Standing goal

Coordinate Multus SDR **MSCC** client/server debugging across Windows, Ubuntu, and Raspberry Pi. Standing rules: root `AGENTS.md`. Optional bus: this folder.

## Current focus — Pi remote AF hot / noisy

**Working:** Win11 client → Win11 servers; Win11 client → Ubuntu servers (Phones + Digital).

**Broken:** Win11 client → **RPi** servers — RX noise floor ~10 dB high; Phone SSB TX much hotter. Likely RX AF overdriven + TX drive banks too high.

**Finding (Build Commander):** `linux/` vs `rpi/` remote-audio **sources match** (no meaningful code delta). Pi **runtime config** differs from working Ubuntu:

| Setting | Ubuntu (good) | Pi (bad) |
|---------|---------------|----------|
| `~/.local/mscc/power.ini` USB/LSB | 50 / 50 | 100 / 100 |
| `user_controls.ini` ALC_VALUE | 0 | 50 |
| `remote-phones.ini` | present ENABLED=1 | **missing** |

**This round:** `rpi` Build applies config steps only (see `COMMANDS.yaml` cmd-002 and `AGENTS.md` Current work). No source edits unless the experiment fails.

## Notes for Builds

- One Build at a time. Sync: pull at session start; push only with real code (config-only OK to commit if Norman wants it in-repo — this task is **host `~/.local/mscc`**, not the git tree).
- Do not copy Ubuntu ELFs into `rpi/`.
