# MSCC coordination bus

Overseer: **Build Commander** (Grok Bot). Workers: **Grok Build** on each host (one at a time).

**Primary instructions for all Builds:** root [`AGENTS.md`](../AGENTS.md) (who/where, trees, current work, coord rules). This folder is the optional command/status bus — not a second protocol.

## Sync policy

Do **not** push/pull on every status ACK.

- **Pull** at session start, and after Norman says overseer/docs were published.
- **Push** when there is **real code** (or a kit) to share — bundle useful status into that commit if needed.
- Status files may stay **local-only** between code pushes; report blockers in chat to Norman/Build Commander if others need them sooner.

## Files

| Path | Who writes | Purpose |
|------|------------|---------|
| `../AGENTS.md` | Overseer + humans | Standing rules + **Current work** for every Build |
| `OVERSEER.md` | Build Commander | Short standing goal / context |
| `COMMANDS.yaml` | Build Commander | Optional ordered commands |
| `status/<host>.md` | That host’s Build only | ACK + progress (often local until a code push) |
| `RESULTS/` (optional) | Builds | Logs, notes, artifact pointers |

## Host ids

| id | Machine | Checkout |
|----|---------|----------|
| `windows-new-hp` | NEW-HP-LAPTOP | `C:\Users\n8vet\OneDrive\Documents\GitHub\mscc-station` |
| `windows-shack` | Shack | TBD |
| `ubuntu-stew` | stew-HP-Notebook | `/home/stew/Documents/GitHub/mscc-station` |
| `rpi` | raspberrypi | `/home/pi/src/mscc-station` |

## Rules

1. Prefer editing **`AGENTS.md` Current work** for shared marching orders so every Build sees one source of truth after a pull.
2. Build Commander may also edit `OVERSEER.md` / `COMMANDS.yaml` for structured tasks.
3. Each Build only edits its own `status/<host>.md` (and optional `RESULTS/<host>/`).
4. Never edit another host’s status file.
5. Prefer **one Build running at a time** (Norman’s rule).
6. Trees: Ubuntu → `linux/`; Pi → `rpi/`; UI → `mscc-ui/`. Do not mix arch.

## Build loop (each host)

```text
cd <checkout>
git pull                    # session start / after published orders
# read AGENTS.md (Current work + MSCC coord)
# optional: OVERSEER.md + COMMANDS.yaml
# do work → update only status/<this-host>.md (local OK)
# when code is ready: commit (+ status if useful) → push → others pull later
```

See root `AGENTS.md` and `handoff.md`.
