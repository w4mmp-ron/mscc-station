# AGENTS.md — Grok instructions (this repo)

Grok loads this file at the repo root. Follow it on **every** host. Details live in the linked docs; do not invent a second protocol.

**Canonical GitHub:** https://github.com/w4mmp-ron/mscc-station  
**Do not use:** `MSCC-Grok-Build` (stale tree).

On start: `git pull`, then read [`handoff.md`](handoff.md). If `.mscc-coord/COMMANDS.yaml` has work for this host, follow **MSCC coord** below.

**Note:** Ron pushes to `main`; always pull first (unless your orders say not to pull for that task).

---

## Shared rules with Ron and Stew ("cut release", releases)

These are the same rules Ron's and Stew's Claude follow. The full text is in the repo-root
[`CLAUDE.md`](CLAUDE.md); read it before any build or release. If this file and `CLAUDE.md`
disagree on these points, `CLAUDE.md` wins.

**Keywords**

| The owner says | Do |
|---|---|
| "build", "build R...", "make the installer" | Build only. Installers go into `installers/`. Nothing is committed, pushed or published. |
| "update the notes" | Edit the notes only. No commit. |
| **"cut release"** | Run the "Cut release" checklist in `CLAUDE.md`: installers, docs, one commit, push, GitHub Release. |
| "stop" | Stop at once. |

- **Pull first:** `git pull --rebase --autostash` before new work; `git fetch` again before the
  push. If someone pushed meanwhile, rebase and rebuild whatever their change touches.
- **Nothing goes into git before "cut release"**, notes included. Then **one commit**, with
  `git add` of specific paths only (other people and tools work in this repo).
- **Versions** live only in the Software Versions Addendum
  (`docs/manuals/MSCC-Software-Versions-Addendum.md` + `.pdf`, keep it to 2 pages). Guides never
  carry version numbers.
- **Release number** `R<month>-<day>-<n>`. Every Windows installer gets a new R number, and the
  WPF client version is set to match it (R11-19-1 -> client 11.19.1).
- **Server trees in step:** a server change goes into every server tree. On a non-Pi host the Pi
  part is a note for Ron, not an edit (see Hard don'ts).

**GitHub Release layout (Ron and Stew agreed 2026-11-19)**

1. **Every release carries ALL the installers**, changed or not: the Windows installer, the Pi
   (arm64) `mscc` and `mscc-avalonia` .debs. No Ubuntu files (Stew, 2026-11-25). The newest release is always the complete current set.
2. **The notes start with a short version table** (each package, the computer it's for, its
   version, changed or unchanged), then what changed, then **"Downloads: which file do I need?"**
   (one line per file), then links to the Windows and Pi install guides and the Addendum PDF.
   Use links, not repo paths.
3. **Check the uploaded file sizes** against the local files.
4. **Keep only the newest release and one earlier one.** After publishing, delete every older
   release, whoever made it: `gh release delete R.. -R w4mmp-ron/mscc-station --yes`. Keep the
   git tags (no `--cleanup-tag`), so an old version can be rebuilt from the repo.

Command: `gh release create R.. -R w4mmp-ron/mscc-station --title R.. --target main --notes-file notes.md <all installers>`.

**Ubuntu .debs:** not in releases (Stew, 2026-11-25): no Ubuntu files or table lines. R11-25-0
carries them once and stays as it is.

---

## Current work

**Active — cmd-099 (ubuntu-stew / stew-HP):** Avalonia VFO polish: no green border (dimming marks the idle VFO), red active dot by the VFO label, larger labels/mode/digits, no hover step text; top row fits 1024×600 (VFOs 154 px each, ALC fully visible); new light-bar row under the VFOs: A>B, B>A, A<>B, SPLIT (existing CMD_SET_SPLIT path, RX A / TX B), STEP. Brief: `.mscc-coord/cmd-099-avalonia-vfo-polish-row.md`. Follows cmd-098. No WPF change, no new protocol commands, no version bump, no installer, no GitHub Release. Build commits; Stew pushes. Leave rpi/, Solidus/, installers/rpi, and the WPF tree alone.

Older notes below are stale as of 2026-10-07. Check `.mscc-coord/COMMANDS.yaml` and [`handoff.md`](handoff.md) for historical open / pending items.

**Pending (no orders yet):** Ubuntu amd64 kit rebuild (recv DC blocker + full FFT 3.143, 800-bin fix, Avalonia CW PHONES removal are in source but not in `installers/linux/`); port the Pi largest-bin spectrum change to Windows / Ubuntu recv; USB-on-load LAST_MODE fix; CAT MD mode map bug on the Linux server and CAT mode-set parity; FM unfinished; DIG-CW design; FREQ CAL progress bar reset; client ask if FW missing; LAST_HF/LF digi defaults; EVENT logging cleanup; TX IQ phase (back-burner).

**On hold:** cmd-034 clear-on-TX + fixed 2:1 fill.

**Current kits:** see the Software Versions Addendum (`docs/manuals/MSCC-Software-Versions-Addendum.md`) and the newest release at <https://github.com/w4mmp-ron/mscc-station/releases>.

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

