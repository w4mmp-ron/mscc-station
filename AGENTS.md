# AGENTS.md — Grok instructions (this repo)

Grok loads this file at the repo root. Follow it on **every** host. Details live in the linked docs; do not invent a second protocol.

**Canonical GitHub:** https://github.com/w4mmp-ron/mscc-station  
**Do not use:** `MSCC-Grok-Build` (stale tree).

On start: `git pull`, then read [`handoff.md`](handoff.md). If `.mscc-coord/COMMANDS.yaml` has work for this host, follow **MSCC coord** below.

---


## Current work

**cmd-030** (windows-new-hp): Enable Remote Digital mic drive slider (default 100%, persist REMOTE_DIGI_MIC_VOL); Client 9.21.7. See `.mscc-coord/COMMANDS.yaml`.

## Who / where

| Person | Focus |
|--------|--------|
| **Stew** | UI (WPF + Avalonia), Windows client, `MSCC.Core` |
| **Ron** | Linux servers, Pi packaging, `.deb` |

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
| `rpi/` | Pi arm64 source of truth. Guide only on Ubuntu — do not patch Pi trees for laptop fixes. |
| `mscc-ui/` | WPF + Avalonia + `MSCC.Core` |
| `installers/{linux,rpi,windows}/` | Current kits for GitHub web |
| `linux-build/` | Ubuntu scripts (`mscc-linux.sh`, UI publish/deb, `drop-installers.sh`) |

- Ubuntu ELFs → `$HOME/mscc`. **Never** copy x86 binaries into `rpi/mscc-binaries/`.
- Cross-arm64 UI/servers on this laptop: `linux-build/mscc-ui-arm64.sh`, `linux-build/cross-arm64.sh`. Use `pwd -P` (symlink `~/mscc-station` vs `Documents/GitHub` breaks Avalonia XAML publish).
- Persist radio `AUDIO_DEVICE` **0/1** only (never boot 2/3).

---

## Installer drop (required)

When a kit is built, copy the newest file into `installers/<platform>/`. History stays in the builder folder.

```bash
./linux-build/drop-installers.sh          # all three
./linux-build/drop-installers.sh linux
```

| Platform | Current kit | History |
|----------|-------------|---------|
| Ubuntu | `installers/linux/` | `linux/mscc-deb/`, Avalonia-Migration, `rpi/mscc-init-gui/` |
| Pi | `installers/rpi/` | `rpi/mscc-deb/`, `rpi/mscc-init-gui/`, `rpi/mscc-portaudio/` |
| Windows | `installers/windows/` | `mscc-ui/Release/windows-wpf/` |

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

## Current work (2026-09-21)

### Active - cmd-024 (windows-new-hp)

Idle UI before Start: frequency 0, no band or mode button pressed; do not
SaveLastUsed / LAST_HF until session live (stops 7.100/40m/USB poisoning DIG-U).
See `.mscc-coord/COMMANDS.yaml` cmd-024.

### Done recently

cmd-023 digi LAST defaults + USB/DIG-U stick + FW ask + FreqCal reset (9.21.3).
cmd-022 WindowTitle radio name.

## Hard don’ts

- Don’t `apt install` `*_arm64.deb` on Ubuntu (except `mscc-init-gui_*_all.deb`).
- Don’t copy Ubuntu `$HOME/mscc` into `rpi/mscc-binaries/`.
- Don’t persist opcode 2/3 as radio boot mode.
- Don’t reuse opcode `0x0E` (Solidus).
- Ask before destructive git / force-push.

