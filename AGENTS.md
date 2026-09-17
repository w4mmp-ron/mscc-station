# AGENTS.md — Grok instructions (this repo)

Grok loads this file at the repo root. Follow it on **every** host. Details live in the linked docs; do not invent a second protocol.

**Canonical GitHub:** https://github.com/w4mmp-ron/mscc-station  
**Do not use:** `MSCC-Grok-Build` (stale tree).

On start: `git pull`, then read [`handoff.md`](handoff.md). If `.mscc-coord/COMMANDS.yaml` has work for this host, follow **MSCC coord** below.

---

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
| **Start of a work session** | `git pull` once so you have current `AGENTS.md` / code. |
| **During ACKs / status notes** | Update **local** `.mscc-coord/status/<your-host-id>.md` only. **Do not** commit, push, or pull just for status. |
| **After real code / kit changes** | Commit the code (and any status worth keeping). Push (or ask Norman to push via GitHub GUI on stew-HP). Other hosts `git pull` before their next code session. |
| **Overseer publishes new orders** | Orders land in `AGENTS.md` (**Current work**) and/or `.mscc-coord/COMMANDS.yaml` + `OVERSEER.md`. Pull when Norman says the bus was updated — not on a timer. |

### Per-command loop

1. Read this file (**Current work** + this section), then `.mscc-coord/OVERSEER.md` + `COMMANDS.yaml` if present.
2. If a command `target` includes this host (or `all`) and you have not finished that `id`:
   - Update **only** `.mscc-coord/status/<your-host-id>.md`: `accepted` → `running` → `done` or `blocked`.
   - Do **not** edit `COMMANDS.yaml`, `OVERSEER.md`, or another host’s status file.
3. Blocked = hardware, credentials, or a human decision — write a short reason. Do not change radio/UDP protocol without an overseer command.
4. Full protocol: `.mscc-coord/README.md`.

---

## Current work (2026-09-17)

### Remote audio — Pi config experiment (cmd-002)

**Working:** Win11 client → Win11 servers; Win11 client → Ubuntu servers (Phones + Digital).

**Broken:** Win11 client → **RPi** servers — RX noise floor ~+10 dB; Phone SSB TX much hotter (RX AF likely overdriven; TX drive banks too hot).

**Do not chase `linux/` vs `rpi/` source first** — remote AF sources match. Align Pi **`~/.local/mscc`** to Ubuntu, then retest.

**`rpi` Build — cmd-002** (config only, no repo source edits):

1. ~~Backup~~ (skipped — Norman OK)
2. `~/.local/mscc/power.ini`: set `USB_POWER=50`, `LSB_POWER=50` (Ubuntu match; Pi was 100/100)
3. `~/.local/mscc/user_controls.ini`: set `ALC_VALUE=0` (Pi was 50)
4. Create `~/.local/mscc/remote-phones.ini` (was missing on Pi):
   `ENABLED=1`, `HOST=<Win11 LAN IP>`, `PORT=9100`, `MONITOR=0`  
   Confirm IP with Norman if needed (Ubuntu used `192.168.1.228`). Add `remote-mic.ini` with `PORT=9101` if missing.
5. Restart MSCC on the Pi; Norman retests Win11 → Pi Phones+Remote.

ACK only in `.mscc-coord/status/rpi.md`. Full command: `.mscc-coord/COMMANDS.yaml` id `cmd-002`.

### Earlier context

**Local WSJT-X** (Pi + Ubuntu) is good. Remote WSJT-X checklist: [`mscc-remote-audio/REMOTE-WSJTX-CHECKLIST.md`](mscc-remote-audio/REMOTE-WSJTX-CHECKLIST.md).

Also: [`linux/UBUNTU-LOCAL-WSJTX-2026-09-16.md`](linux/UBUNTU-LOCAL-WSJTX-2026-09-16.md), [`rpi/PI-LOCAL-WSJTX-2026-09-16.md`](rpi/PI-LOCAL-WSJTX-2026-09-16.md), [`mscc-remote-audio/REMOTE-AUDIO-PUNCHLIST.md`](mscc-remote-audio/REMOTE-AUDIO-PUNCHLIST.md).

## Hard don’ts

- Don’t `apt install` `*_arm64.deb` on Ubuntu (except `mscc-init-gui_*_all.deb`).
- Don’t copy Ubuntu `$HOME/mscc` into `rpi/mscc-binaries/`.
- Don’t persist opcode 2/3 as radio boot mode.
- Don’t reuse opcode `0x0E` (Solidus).
- Ask before destructive git / force-push.
