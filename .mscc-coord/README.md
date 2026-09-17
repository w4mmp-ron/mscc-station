# MSCC coordination bus

Overseer: **Build Commander** (Grok Bot). Workers: **Grok Build** on each host (one at a time).

## Files

| Path | Who writes | Purpose |
|------|------------|---------|
| `OVERSEER.md` | Build Commander | Standing goal / context |
| `COMMANDS.yaml` | Build Commander | Ordered commands for Builds |
| `status/<host>.md` | That host’s Build only | ACK + progress + results |
| `RESULTS/` (optional) | Builds | Logs, notes, artifact pointers |

## Host ids

| id | Machine | Checkout |
|----|---------|----------|
| `windows-new-hp` | NEW-HP-LAPTOP | `C:\Users\n8vet\OneDrive\Documents\GitHub\mscc-station` |
| `windows-shack` | Shack | TBD |
| `ubuntu-stew` | stew-HP-Notebook | `/home/stew/Documents/GitHub/mscc-station` |
| `rpi` | raspberrypi | `/home/pi/src/mscc-station` |

## Rules

1. Build Commander only edits `OVERSEER.md` and `COMMANDS.yaml`.
2. Each Build only edits its own `status/<host>.md` (and optional files under `RESULTS/<host>/`).
3. Never edit another host’s status file.
4. Prefer **one Build running at a time** (Norman’s rule).
5. After finishing a command: set status, `git pull` / commit status only if asked, then stop and wait for the next command id.
6. Trees: Ubuntu work → `linux/`; Pi work → `rpi/`; UI → `mscc-ui/`. Do not mix `linux/` and `rpi/` for the wrong arch.

## Build loop (each host)

```text
cd <checkout>
git pull
# read .mscc-coord/OVERSEER.md and COMMANDS.yaml
# if a command targets this host (or target: all) and status is pending:
#   ACK → running → do work → done|blocked
# update only status/<this-host>.md
```

See root `AGENTS.md` (§ MSCC coord) and `handoff.md`.
