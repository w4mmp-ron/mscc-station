# AGENTS.md — Grok instructions (this repo)

Grok loads this file at the repo root. Follow it on **every** host. Details live in the linked docs; do not invent a second protocol.

**Canonical GitHub:** https://github.com/w4mmp-ron/mscc-station  
**Do not use:** `MSCC-Grok-Build` (stale tree).

On start: `git pull`, then read [`handoff.md`](handoff.md). If `.mscc-coord/COMMANDS.yaml` has work for this host, follow **MSCC coord** below.

**Note:** Ron pushes to `main`; always pull first (unless your orders say not to pull for that task).

---


## Current work

**Active - cmd-056 (windows-new-hp): Windows FREQ CAL second-STOP.** Port Linux `else if (cal_abort_pending)` (`519c682` / `linux/.../calibrate.c`) into `mscc-ui/windows-work-tree/ms-sdr-MKII/source/calibrate.c` so a second STOP while drain is pending logs `nothing new owed, drain still pending` and does **not** call `Cal_Abort_Drain_Done("nothing owed")`. Do **not** edit `rpi/` - Ron ports Pi after Stew pushes. Commit locally; do not push. See `.mscc-coord/briefs/cmd-056.md`.

**Active - cmd-057 (windows-new-hp): Remove CW-tab PHONES (WPF only, BL-001).** Ron: unused `0x70`. Remove checkbox + `CwPhones` wire-up; keep POTENTIA/QSK. Avalonia later on stew-HP. Manuals/screenshots later. Do not touch operator Phones audio path. Commit locally; do not push. See `.mscc-coord/briefs/cmd-057.md`.

**Active - cmd-055 (windows-new-hp): Windows factory_seed drop mirror + live wins (Ron Q4).** After / with cmd-048 Windows: change `load_iq_or_power` so when not switching lines and live exists, copy live -> `cal\<line>\` and return; use cal stash only on line switch or missing live; then factory. Drop `Factory_mirror_live_to_cal` write-through on power/IQ save. Covers `power_cal.ini` and `iq.ini`. `0xAA` leave ignored. Commit locally; do not push. See `.mscc-coord/briefs/cmd-055.md`.

**Coord (manuals, not a Build cmd):** Ron answered `.mscc-coord/QUESTIONS-FOR-RON.md`. Paste-ready Linux/Windows manual replacements in `.mscc-coord/MANUAL-UPDATES-FROM-RON.md` (Pi groups, upgrade keeps cal, CW POTENTIA/QSK, PHONES not used). PDFs not regenerated yet - Stew applies to draft source.

**Backlog BL-001:** WPF = **cmd-057**; Avalonia still open. See `.mscc-coord/BACKLOG.md`.

**Note for Stew:** After cmd-056 ships + Stew pushes Windows `calibrate.c`, tell Ron the hash so he ports Pi second-STOP.

**Active - cmd-052 (ubuntu-stew): Avalonia mscc-ui 0.6.69 StartRemoteAf stop-before-open (sticky Digital).** After Connect, clicking Remote (ToggleRemoteAudio → OnRemoteAudioChanged → StartRemoteAf) must fully `RemoteAf.Stop()` before ApplyRemoteAfDevices / StartRx / StartMic, with the same fail-safe Stop-on-fail as cmd-051. Fixes Pulse `Assertion 'm->n_waiting > 0' failed` on first open / Remote OFF→ON with Digital (cmd-051 only covered live Phones↔Digital flips). Do not change WPF or ApplyRemoteAfDevicesAndRestart. Bump to 0.6.69 (4 version places). REMOTE smoke: cold Digital Remote ON, cold Phones Remote ON, OFF→ON, Phones↔Digital still OK. Gate: HEAD = cmd-052 orders. Commit locally; do not push or pull. Do not touch `rpi/`, `linux/`, `docs/`, WPF or Core. See `.mscc-coord/briefs/cmd-052.md`.

**Active - cmd-051 (ubuntu-stew): Avalonia mscc-ui 0.6.68 Remote AF stop-before-restart on Phones↔Digital path switch.** While Remote is on, path/device restart via `ApplyRemoteAfDevicesAndRestart` must fully `RemoteAf.Stop()` (RX+mic/player) before re-resolving devices and StartRx/StartMic; guard failed opens; log before/after. Fixes Pulse `Assertion 'm->n_waiting > 0' failed` in `pa_threaded_mainloop_wait`. Do not change WPF. Bump to 0.6.68 (4 version places). REMOTE smoke: Phones↔Digital both directions + start on Digital then flip; Mic TX drops=0. Gate: HEAD = cmd-051 orders. Commit locally; do not push or pull. Do not touch `rpi/`, `linux/`, `docs/`, WPF or Core. See `.mscc-coord/briefs/cmd-051.md`.

**Active - cmd-049 (ubuntu-stew): Avalonia mscc-ui 0.6.67 S-meter + ALC meter HOLD and Peak.** Enable the placeholder HOLD / Peak checkboxes on both analog meters with the WPF behaviour (30 ms ballistics: HOLD = slow needle fall, off = tracks instantly; Peak = orange peak needle that holds ~2 s then falls), saved as SMETER_HOLD / SMETER_PEAK / ALC_HOLD / ALC_PEAK in `~/.config/MSCC/mscc-avalonia.ini` (defaults HOLD on, Peak off). HOLD/Peak only: no S-meter smoothing or other nits. Gate: HEAD = cmd-049 orders. Local smoke on the radio. Do not touch `rpi/`, `linux/`, `docs/`, WPF or Core. Commit locally; do not push or pull. See `.mscc-coord/briefs/cmd-049.md`.

**Active - cmd-047a (ubuntu-stew): Avalonia mscc-ui 0.6.64 UI nits + FREQ CAL fixes.** 10m band button as wide as the others (grid had one star column too few), remove the stray green "Band: 20m" label drawn over USER (WPF has none), FREQ CAL: Connect with the tab open re-applies CW, VFO B no longer changes VFO A, re-entering the tab cancels the deferred restore; minor Launch/log/tooltip/VFO B save fixes; restore the deleted status history. Gate: HEAD = cmd-047a orders on 8999f9a. Local smoke only. Leave the linux/tty0tty-master leftovers unstaged. Do not touch `rpi/`, `linux/`, WPF or Core. Commit locally; do not push or pull. See `.mscc-coord/briefs/cmd-047a.md`.

**Active - cmd-047 (ubuntu-stew): Avalonia mscc-ui 0.6.61 catches up with WPF + local Launch/Auto.** cmd-041 DIG-U Hi 1.4k/1.0k (idx 5/6), cmd-044/045/045a/045b FREQ CAL (tab CW/200/600, STOP + drain, progress, colours, CHECK LOOSE), VFO B restored (VFO A always starts active), QRP/Full Power/AMP from AmpOn, radio-model button removed, tooltips, cmd-046 power bank fix. Launch (Host 127.0.0.1 only) starts/stops the local servers; Auto presses Connect on start for any Host. Extra: tty0tty DKMS (Stew pastes one sudo line). Gate: HEAD = cmd-047 orders on 519c682; mscc 1.0.47 installed. Local smoke only (remote deferred). Leave the linux/tty0tty-master leftovers unstaged. Do not touch `rpi/`, `linux/`, WPF or Core. Commit locally; do not push or pull. See `.mscc-coord/briefs/cmd-047.md`.

**Active - cmd-043 (ubuntu-stew): Ubuntu mscc 1.0.47 amd64.** Port Ron's Pi fixes from `rpi/` into `linux/` (recv/trans ring + 40 ms digi latency, panadapter, IQ_BAND bounds, remote_mic, power-cal slot, local digital gain, ms-sdr FREQ CAL STOP), plus cmd-042 hi-cut 1.4k/1.0k and cmd-046 (USB/LSB/DIG-U use SSB power; TUNE power is TUNE only). recv/trans 3.141. Replaces the ubuntu-stew part of cmd-042 (no 1.0.45 amd64). Copy FROM `rpi/`, never write to it. Commit locally; do not push or pull. Note: Ron's Claude edits `rpi/` and pushes straight to main. See `.mscc-coord/briefs/cmd-043.md`.

**Active - cmd-039 (rpi):** `remote_mic` diagnostic-only finer EVENT logging (under/hold-last, overflow/drop, adaptive step, low occupancy, optional UDP gap); keep periodic summary; milliseconds in the body; tag `remote_mic EVENT`. **Do not** implement cmd-034 fill. See `.mscc-coord/COMMANDS.yaml` + `briefs/cmd-039.md`.

**On hold:** cmd-034 clear-on-TX + fixed 2:1 fill.

**Active - cmd-041 + cmd-042 (DIG-U Hi 1.4k/1.0k):** NEW-HP first: cmd-041 WPF + cmd-042 Windows `mscc-recv.exe` (0xD1 idx 5=1400, 6=1000; recv 3.141). Then, after Stew pushes: **ubuntu-stew** cmd-042 `linux/` -> `mscc_1.0.45_amd64.deb`; then **rpi** cmd-042 `rpi/` -> `mscc_1.0.45_arm64.deb`. Commit locally, do not push. Do not update the RPi Grok Build app. See `.mscc-coord/briefs/cmd-041.md` + `briefs/cmd-042.md`.

**Active - cmd-046 (NEW-HP, follow Ron):** TUNE power drives TUNE only. Windows SDRcore-trans `driver.c` LSB/USB use LSB_POWER/USB_POWER in every audio mode (digital no longer uses TUNE_POWER); WPF DIG-U main slider back on SSB POWER (reverts cmd-045 item 7.2 + A8 tooltip). WPF 9.26.7, SDRcore-trans 3.142. `linux/` part rides in cmd-043; `rpi/` unchanged. Commit locally, do not push. See `.mscc-coord/briefs/cmd-046.md` + `briefs/cmd-046-windows-notes.md`.

**Active - cmd-045b (NEW-HP):** FREQ CAL STOP gets the same look as the other FREQ CAL buttons (no amber override). After STOP, ms-sdr-MKII drains the aborted run (drops late recv replies, refuses a new AUTO/CHECK until recv's finish reply or 35 s) and tells the client; the client keeps AUTO/CHECK grey with `STOPPED — wait…` until then. The abort no longer sends recv a second finish. WPF 9.26.6, ms-sdr 3.176. Commit locally, do not push. See `.mscc-coord/briefs/cmd-045b.md`.

**Active - cmd-045a (NEW-HP):** FREQ CAL tab gets a STOP button (enabled only while AUTO/CHECK runs) that aborts the run cleanly via a new ms-sdr-MKII `CMD_SET_CAL_ABORT` (no PPM change), then resets the tab (STOPPED). WPF 9.26.5. Commit locally, do not push. See `.mscc-coord/briefs/cmd-045a.md`.

**Active - cmd-045 (NEW-HP):** WPF: VFO B frequency/mode restored after restart; FREQ CAL Stop/Start/close cleanup, running notice for AUTO/CHECK only, new CHECK/RESET messages, "MANUAL steps" label; QRP / Full Power / AMP always in sync (one state, AmpOn); remove the Proficio/Geminus button; DIG-U power slider on Tune power. Commit locally, do not push. See `.mscc-coord/briefs/cmd-045.md`.

**Active - cmd-044 (NEW-HP):** WPF tooltip/text fixes + dev-notes removal, Host/Port change applies on next Start, FREQ CAL fixes (colors, progress, CHECK LOOSE, CW on tab entry + restore on leave) and Windows `ms-sdr-MKII` calibrate.c fixes (progress counter reset, failed cal restores previous mode). Commit locally, do not push. Avalonia + Linux/Pi go with cmd-043 (reserved). See `.mscc-coord/briefs/cmd-044.md`.

**Peers on NEW-HP:** cmd-038 pending/orders for WPF Mic TX EVENT logging; cmd-037, cmd-036, and cmd-035 done.

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
| `rpi/` | Pi arm64 source of truth. **`rpi` host only** — read-only on every other host (see Hard don’ts). |
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

## Current work (2026-09-23)

### Active - cmd-039 (rpi)

Finer `remote_mic` EVENT logging only (under/hold-last, overflow/drop, adaptive
48→96 step, low occupancy, optional UDP gap). Greppable `remote_mic EVENT` with ms in
body. Keep first+every-500 summary. Edit `rpi/SDRcore-trans-linux/sources/remote_mic.c`.
Install `$HOME/mscc`. Optional package 1.0.44 → 1.0.45. **No fill algorithm change.**

### On hold - cmd-034 (rpi)

Clear-on-TX + fixed 2:1 + silence on underrun — do not implement while 039 ships.

### Peer work on NEW-HP

cmd-046: TUNE power drives TUNE only (Windows SDRcore-trans + WPF DIG-U slider back to SSB), orders (Ron's brief).
cmd-045b: WPF FREQ CAL STOP colour + ms-sdr-MKII abort drain / no double finish, orders.
cmd-045a: WPF FREQ CAL STOP button + ms-sdr-MKII CMD_SET_CAL_ABORT, orders.
cmd-045: WPF VFO B restore, FREQ CAL stop/close/messages, QRP/Full Power/AMP sync, remove radio-model button, DIG-U slider on Tune power, orders.
cmd-044: WPF tooltips + Host/Port on Start + FREQ CAL fixes; ms-sdr-MKII cal progress/fail mode, orders.
cmd-041: WPF DIG-U Hi 1.4k/1.0k options, orders (paired with cmd-042 Windows recv; build together).
cmd-038: WPF `RemoteMicSender` diagnostic EVENT logging, pending/orders.
cmd-037: WPF 60 ms mic cushion, done (`5f39a16`, 9.23.1).
cmd-036: WPF paced mic send, done (`d26c7a7`, 9.23.0).
cmd-035: local Win TX 35 ms gate, done (`47e336c`).

### Done recently

cmd-033 WPF 9.22.0; cmd-032 mscc 1.0.44; cmd-031 remote_mic stream reset.


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
- Ask before destructive git / force-push.


