# AGENTS.md â€” Grok instructions (this repo)

Grok loads this file at the repo root. Follow it on **every** host. Details live in the linked docs; do not invent a second protocol.

**Canonical GitHub:** https://github.com/w4mmp-ron/mscc-station  
**Do not use:** `MSCC-Grok-Build` (stale tree).

On start: `git pull`, then read [`handoff.md`](handoff.md). If `.mscc-coord/COMMANDS.yaml` has work for this host, follow **MSCC coord** below.

---


## Current work

**cmd-038** (windows-new-hp, **client**): WPF `RemoteMicSender` - **diagnostic-only** finer Mic TX EVENT logging (every underrun / drop / low-cushion; optional WaveIn gap); keep periodic summary; ms stamps in EVENT lines; Client **9.23.2**. No audio/cushion/Pi change. See `.mscc-coord/COMMANDS.yaml` + `.mscc-coord/briefs/cmd-038.md`.

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
| `rpi/` | Pi arm64 source of truth. Guide only on Ubuntu â€” do not patch Pi trees for laptop fixes. |
| `mscc-ui/` | WPF + Avalonia + `MSCC.Core` |
| `installers/{linux,rpi,windows}/` | Current kits for GitHub web |
| `linux-build/` | Ubuntu scripts (`mscc-linux.sh`, UI publish/deb, `drop-installers.sh`) |

- Ubuntu ELFs â†’ `$HOME/mscc`. **Never** copy x86 binaries into `rpi/mscc-binaries/`.
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
| **Overseer publishes orders for this host** | Orders are written **on this machineâ€™s checkout first** (no pull required to start). ACK in local `status/<host>.md`. |
| **After real code / kit changes** | Commit on this host. Push (or Norman pushes). **Then** other hosts `git pull` to catch up. |
| **Start of a work session on a non-target host** | `git pull` once if you need the latest bus/code from a push. |
| **During ACKs / status notes** | Update **local** `.mscc-coord/status/<your-host-id>.md` only. **Do not** commit, push, or pull just for status. |
| **Do not edit** | `COMMANDS.yaml`, `OVERSEER.md`, or another hostâ€™s status (Overseer owns those). |

### Per-command loop

1. Read this file (**Current work** + this section), then `.mscc-coord/OVERSEER.md` + `COMMANDS.yaml` if present.
2. If a command `target` includes this host (or `all`) and you have not finished that `id`:
   - Update **only** `.mscc-coord/status/<your-host-id>.md`: `accepted` â†’ `running` â†’ `done` or `blocked`.
   - Do **not** edit `COMMANDS.yaml`, `OVERSEER.md`, or another hostâ€™s status file.
3. Blocked = hardware, credentials, or a human decision â€” write a short reason. Do not change radio/UDP protocol without an overseer command.
4. Full protocol: `.mscc-coord/README.md`.

---

## Current work (2026-09-23)

### Active - cmd-038 (windows-new-hp, client)

WPF `RemoteMicSender`: diagnostic EVENT logging only - immediate/rate-limited `Mic TX EVENT underrun=` / `drop=` / `low_cushion=` (under half cushion / ~30 ms); optional WaveIn gap; keep first+every-500 summary with counters; embed ms in message body (file header is second-only). Bump **9.23.1 -> 9.23.2**. Brief: `.mscc-coord/briefs/cmd-038.md`. No cushion/audio change. cmd-034 Pi on hold. cmd-035 local gate done (`47e336c`). cmd-037 cushion done (`5f39a16`, 9.23.1).

### Done recently

cmd-037 WPF 60 ms mic cushion (`5f39a16`, 9.23.1) - residual rare JT65 bump + summary-only logging -> cmd-038.
cmd-036 WPF paced mic send (`d26c7a7`, 9.23.0).
cmd-035 local Win TX 35 ms gate (`47e336c`).
cmd-033 0xBC ownership + Remote vs local audio (WPF 9.22.0).
cmd-030 Remote Digital mic slider (9.21.7).
cmd-024..026 DIG-U / idle / band-last-used family.

## Hard donâ€™ts

- Donâ€™t `apt install` `*_arm64.deb` on Ubuntu (except `mscc-init-gui_*_all.deb`).
- Donâ€™t copy Ubuntu `$HOME/mscc` into `rpi/mscc-binaries/`.
- Donâ€™t persist opcode 2/3 as radio boot mode.
- Donâ€™t reuse opcode `0x0E` (Solidus).
- Ask before destructive git / force-push.



