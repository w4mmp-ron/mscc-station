# AGENTS.md — Grok instructions (this repo)

Grok loads this file at the repo root. Follow it on **every** host. Details live in the linked docs; do not invent a second protocol.

**Canonical GitHub:** https://github.com/w4mmp-ron/mscc-station  
**Do not use:** `MSCC-Grok-Build` (stale tree).

On start: `git pull`, then read [`handoff.md`](handoff.md). If `.mscc-coord/COMMANDS.yaml` has work for this host, follow **MSCC coord** below.

**Note:** Ron pushes to `main`; always pull first (unless your orders say not to pull for that task).

---

## Current work

No Build command is active as of 2026-10-07. Check `.mscc-coord/COMMANDS.yaml` for new orders. Current state and the open / pending list live in [`handoff.md`](handoff.md).

**Pending (no orders yet):** Ubuntu amd64 kit rebuild (recv DC blocker + full FFT 3.143, 800-bin fix, Avalonia CW PHONES removal are in source but not in `installers/linux/`); port the Pi largest-bin spectrum change to Windows / Ubuntu recv; USB-on-load LAST_MODE fix; CAT MD mode map bug on the Linux server and CAT mode-set parity; FM unfinished; DIG-CW design; FREQ CAL progress bar reset; client ask if FW missing; LAST_HF/LF digi defaults; EVENT logging cleanup; TX IQ phase (back-burner).

**On hold:** cmd-034 clear-on-TX + fixed 2:1 fill.

**Current kits:** Windows R10-4-0 (WPF 10.4.0); Ubuntu `mscc` 1.0.49 / `mscc-ui` 0.6.71; Pi `mscc` 1.0.56 / `mscc-ui` 0.6.71 / `mscc-init` 1.0.18.

## Who / where

| Person | Focus |
|--------|--------|
| **n8vet (Stew)** | UI (WPF + Avalonia), Windows client, Ubuntu amd64 kit, `MSCC.Core` |
| **Ron** | Pi servers + packaging (`rpi/`), Solidus |

| Host id | Machine | Checkout |
|---------|---------|----------|
| `ubuntu-stew` | stew-HP-Notebook | `/home/stew/Documents/GitHub/mscc-station` |
| `windows-new-hp` | NEW-HP-LAPTOP | `C:\Users\n8vet\OneDrive\Documents\GitHub\mscc-station` |
| `windows-shack` | Shack | TBD |
| `rpi` | raspberrypi | `/home/pi/src/mscc-station` |

Prefer **one Grok Build at a time**.

---

## Trees (do not mix)

| Tree | Role |
|------|------|
| `linux/` | Ubuntu x86_64 servers / helpers. **Edit here** for this laptop. |
| `rpi/` | Pi arm64 source of truth. **`rpi` host only** — read-only on every other host (see Hard don’ts). |
| `mscc-ui/` | WPF + Avalonia + `MSCC.Core` |
| `installers/{linux,rpi,windows}/` | Current kits for GitHub web |
| `linux-build/` | Ubuntu scripts (`mscc-linux.sh`, UI publish/deb, `drop-installers.sh`) |

- Ubuntu ELFs → `$HOME/mscc`. **Never** copy x86 binaries into `rpi/mscc-binaries/`.
- Cross-arm64 UI/servers on this laptop: `linux-build/mscc-ui-arm64.sh`, `linux-build/cross-arm64.sh`. Use `pwd -P` (symlink `~/mscc-station` vs `Documents/GitHub` breaks Avalonia XAML publish).
- Persist radio `AUDIO_DEVICE` **0/1** only (never boot 2/3).

---

## Installer drop (required)

When a kit is built, copy the newest file into `installers/<platform>/` and add an entry to that folder's `CHANGELOG.md` (windows / linux). History stays in the builder folder.

```bash
./linux-build/drop-installers.sh          # all three
./linux-build/drop-installers.sh linux
```

| Platform | Current kit | History |
|----------|-------------|---------|
| Ubuntu | `installers/linux/` (+ `CHANGELOG.md`) | `linux/mscc-deb/`, `mscc-ui/Release/avalonia/x86_64/` |
| Pi | `installers/rpi/` | `rpi/mscc-deb/`, `rpi/mscc-init-gui/`, `rpi/mscc-portaudio/` |
| Windows | `installers/windows/` (+ `CHANGELOG.md`) | `mscc-ui/Release/windows-wpf/` |

---

## MSCC coord (Build Commander)

**Standing instructions for every host live in this `AGENTS.md`.** Overseer (Build Commander) updates this file and `.mscc-coord/` when goals change. You do **not** need a push/pull on every ACK.

### Sync policy (important)

| When | What to do |
|------|------------|
| **Overseer publishes orders for this host** | Orders are written **on this machine’s checkout first** (no pull required to start). ACK in local `status/<host>.md`. |
| **After real code / kit changes** | Commit on this host. Push (or Norman pushes). **Then** other hosts `git pull` to catch up. |
| **Start of a work session on a non-target host** | `git pull` once if you need the latest bus/code from a push. |
| **During ACKs / status notes** | Update **local** `.mscc-coord/status/<your-host-id>.md` only. **Do not** commit, push, or pull just for status. |
| **Do not edit** | `COMMANDS.yaml`, `OVERSEER.md`, or another host’s status (Overseer owns those). |

### Per-command loop

1. Read this file (**Current work** + this section), then `.mscc-coord/OVERSEER.md` + `COMMANDS.yaml` if present.
2. If a command `target` includes this host (or `all`) and you have not finished that `id`:
   - Update **only** `.mscc-coord/status/<your-host-id>.md`: `accepted` → `running` → `done` or `blocked`.
   - Do **not** edit `COMMANDS.yaml`, `OVERSEER.md`, or another host’s status file.
3. Blocked = hardware, credentials, or a human decision — write a short reason. Do not change radio/UDP protocol without an overseer command.
4. Full protocol: `.mscc-coord/README.md`.

---

## Hard don’ts

- **Don’t touch `rpi/`.** No edits, builds into, commits, merges, or conflict
  resolution in `rpi/` from `windows-new-hp`, `ubuntu-stew`, `windows-shack` or any
  other non-Pi session. Only the `rpi` host changes `rpi/`. Pi work needed for a
  Windows/Ubuntu change → write it up as a note for the `rpi` host; don't patch it.
  A merge that touches `rpi/` → stop and ask.

- Don’t `apt install` `*_arm64.deb` on Ubuntu (except `mscc-init-gui_*_all.deb`).
- Don’t copy Ubuntu `$HOME/mscc` into `rpi/mscc-binaries/`.
- Don’t persist opcode 2/3 as radio boot mode.
- Don’t reuse opcode `0x0E` (Solidus).
- Don’t touch `Solidus/` from non-Pi hosts (Ron's work).
- Ask before destructive git / force-push.

