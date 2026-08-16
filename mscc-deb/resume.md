# MSCC Linux packaging — resume / handoff

**Date:** 2026-08-08 (keyer note) / packaging versions may lag  
**Tree:** `worktrees/mscc-deb` (+ siblings below)

| Package | Current file |
|---------|----------------|
| Main stack | **`mscc_1.0.34_arm64.deb`** (includes updated `mscc-binaries` as of 2026-08-14) |
| PortAudio | **`mscc-portaudio_19.8.2_arm64.deb`** |
| Init GUI | **`mscc-init-gui_1.0.11_all.deb`** |

Operator install: **INSTALL-FOR-PI.md**. Package overview: **README.md**.

### Keyer CQ memory (2026-08)

Linux radio path for CQ memory (`0x9C`) is in **`ms-sdr-linux`** + PIC + Proficio — see **`keyer/RESUME.md`**, **`ms-sdr-linux/RESUME.md`**.  
Ship a new **ms-sdr** binary in the next `mscc` deb when ready. Seed `cw.ini` may include `CW_Mem_Text_WPM=0` (host-only Farnsworth param; no USB yet).  
**Windows ms-sdr** is a separate update (Stew).

---

## Current status (working)

| Item | State |
|------|--------|
| Main package | **1.0.27** — arm64 servers + CLI + Virtual* + desktop Start/Stop + updated `sdrcore-recv` |
| PortAudio | **`mscc-portaudio` 19.8.2** from `portaudio/build` → `/usr/local` |
| Digi RX/TX | VirtualA / VirtualB.monitor; dual stream when host APIs differ; WSJT verified |
| Operator phones | **Any sample rate**; Oboe resampler in recv/trans when not 96 kHz |
| ALSA levels | Init sets 100%; **`mscc start` re-applies** via `amixer` (not `.asoundrc`) |
| Init GUI | Operator devices any rate; digi fixed; remembers ALSA cards; menu **MSCC Init** |
| Spectrum (recv) | Bias **40**, MAX_Y **16000**; **Pan Resolution** 800/1600/3200 (client lockstep) |
| Desktop | **MSCC Start** / **MSCC Stop**; `mscc-desktop-ctl` |
| postinst | Does **not** run mscc-init (user runs after install) |
| End-to-end | Servers on Pi; **WPF** MSCC client on PC (headless operate) |

---

## Install order

```text
1. mscc-portaudio_19.8.2_arm64.deb
2. mscc_1.0.27_arm64.deb
3. mscc-init-gui_1.0.10_all.deb
```

```bash
ldd $HOME/mscc/sdrcore-recv | grep portaudio
# expect: /usr/local/lib/libportaudio.so.2
```

**Do not** put PortAudio only in `$HOME/portaudio-install` + `.bashrc` for shipping. Use the deb + rpath.

---

## Sibling trees (handoff)

```text
worktrees/
  mscc-deb/                 # this package + INSTALL-FOR-PI + resume
  mscc-binaries/            # arm64 drop: ms-sdr, sdrcore-*, mscc-init, mscc.sh
  mscc-init-files-linux/    # seed inis
  mscc-portaudio/           # PortAudio .deb (libs from portaudio/build)
  portaudio/                # PortAudio sources + AArch64 build/ (packaging source)
  mscc-init-gui/            # GUI init wizard .deb
  mscc-init-linux/          # CLI init sources
  ms-sdr-linux/             # ms-sdr sources
  SDRcore-recv-linux/       # recv: dual stream, Oboe, pan resolution, bias 40
  SDRcore-trans-linux/      # trans: dual stream, Oboe upsample
  tty0tty-master/module/
  swr-meter/                # optional WiFi SWR notes
  mscc-mscc/                # Windows WPF client area (not Pi package)
```

**Removed / not required for Pi package:** Windows-only `SDRcore-recv` / `SDRcore-trans` trees, old `alsa/` digi loopback, `mscc-init` (non-linux), `ms-sdr-MKII`.  
**Optional cruft:** `portaudio-install/` (legacy staging; packaging uses `portaudio/`), `oboe-main/` (resampler vendored under recv/trans `sources/resampler/`).

---

## Architecture (one sentence)

**Three headless processes on the Pi** (ms-sdr, sdrcore-recv, sdrcore-trans) do RF and audio; **WPF client on the PC** is the remote console (UDP).

```text
MSCC client (PC)  ←UDP→  ms-sdr (:8888)  ←→  Proficio
                           ├→ sdrcore-recv (:9000)
                           └→ sdrcore-trans (:9200)
```

---

## Digi path (canonical)

```text
RX:  Proficio(ALSA) → sdrcore-recv → VirtualA → VirtualA.monitor → WSJT Input
TX:  WSJT Output → VirtualB → VirtualB.monitor → sdrcore-trans → Proficio(ALSA)
```

**Digi volume:** Pulse sinks (not ALSA). Create script currently sets Virtual* sinks to **60%**; operator AF is separate. Raising VirtualA to 100% via `pactl` improves digi headroom; client Volume then rides lower. Optional future: bake digi sinks to 100% in `mscc-virtual-audio`.

---

## Operator audio (phones)

| Topic | Behavior |
|-------|----------|
| Proficio I/Q | Always **96 kHz** |
| Operator device | May be 48k etc. → **dual stream + Oboe** resample |
| Rates match 96k | Full duplex or dual ring, **no** resampler |
| Levels | ALSA **100% unmute** on init; **re-apply on every `mscc start`** (`operator-alsa-cards.txt` + ini names) |
| Not used | `~/.asoundrc` for levels (unreliable on Pi OS) |

---

## Spectrum / pan (recv) — client lockstep required

| Server | Client |
|--------|--------|
| Log bias **40**, MAX_Y **16000** | `RawYToDb` bias **40** |
| Pan Resolution index **0/1/2** → **800/1600/3200** bins | UI “Pan Resolution” |
| Legacy refresh **3–10** → `G_Panadapter_Blocks` (update *rate*) | Not the same as bin count |

Log example: `PAN RESOLUTION CHANGED → 1600 bins (index 1, High) …`  
**GRID MIN/MAX** are **client-only** (do not appear in recv log).  
Visual difference at High/Max may be small if the UI downsamples to fixed screen width.

**Product note (handoff):** Multi-resolution + bias 40 is a shared mapping for more than one rig; Proficio operators may prefer simpler defaults (KISS). Revert path exists (bias 22 / MAX_Y 4999 / fixed 800) if product reverses.

---

## Critical lessons (do not regress)

1. Digi = **Pulse Virtual***, not ALSA cables in package.  
2. No ALSA+Pulse full duplex → **dual stream** when host APIs differ.  
3. Distro PortAudio often ALSA-only → **mscc-portaudio** + **rpath `/usr/local`**.  
4. **`.bashrc` LD_LIBRARY_PATH** does not apply to nohup / desktop Start.  
5. **`alsactl` / `.asoundrc` alone do not stick** operator levels → re-`amixer` on start.  
6. Config seed **never overwrites** existing `~/.local/mscc/*.ini`.  
7. postinst must **not** run interactive mscc-init.  
8. Backup before non-trivial source edits.  
9. Install order: **portaudio → mscc → init-gui**.  
10. Handoff changes need a **punch list** (files + client lockstep).

---

## Build / release checklist

### mscc-portaudio
1. AArch64 libs in `portaudio/build/libportaudio.so*` + `portaudio/include/`.  
2. `cd mscc-portaudio && ./build-deb.sh` → `mscc-portaudio_19.8.2_arm64.deb` (or bump Version).

### mscc
1. Pi-build arm64 binaries → `mscc-binaries/` (`sdrcore-recv`, `sdrcore-trans`, `ms-sdr`, `mscc-init`, `mscc.sh`).  
2. `ldd` each PortAudio binary → `/usr/local/lib`.  
3. Bump `packaging/DEBIAN/control` Version.  
4. `./build-deb.sh`.  
5. Smoke: Virtual*, digi D mode, operator phones 96k and non-96k, pan log lines, Start/Stop.

### mscc-init-gui
1. `cd mscc-init-gui && ./build-deb.sh`.  
2. Menu **MSCC Init**; pick any-rate phones; confirm levels after start.

---

## Open / deferred

| Item | Notes |
|------|--------|
| Digi Virtual* sinks default **100%** | Still 60% in create script; optional |
| Always dual in D even same API | Dual when host APIs **differ** (or rate mismatch) |
| Remote operator AF to PC | Not in scope; future product |
| WiFi SWR via ms-sdr | `swr-meter/`; opcodes 0x0B/0C/0D |
| `log_msg` helper | `SDRcore-recv-linux/sources/log_msg.{c,h}` — **not wired** yet |
| INSTALL-FOR-PI.pdf | Regenerate from .md/.docx as needed |
| WPF client bugs | e.g. GRID range slider snap — client only |

---

## Quick Pi checks

```bash
ldconfig -p | grep portaudio
ldd $HOME/mscc/sdrcore-recv | grep portaudio
ldd $HOME/mscc/sdrcore-trans | grep portaudio

pactl list short sinks | grep Virtual
systemctl --user status mscc-virtual-audio
mscc status

# Pan resolution
grep -E "PAN RESOLUTION|PAN SMOOTHING|DUAL STREAM|Oboe|resample|ALSA card" \
  ~/sdrcore-recv.log ~/.local/mscc/sdrcore-recv.log 2>/dev/null | tail -30
```

---

## Version milestones (recent)

| Ver | Notes |
|-----|--------|
| **1.0.23** | PortAudio rpath `/usr/local`; mscc.sh prefers /usr/local |
| **1.0.24–1.0.26** | ALSA levels on start; packaging polish |
| **1.0.27** | Updated **sdrcore-recv** (pan resolution + bias 40 + Oboe path as built) |
| **mscc-portaudio 19.8.2** | Libs from `portaudio/build` (not portaudio-install) |
| **mscc-init-gui 1.0.10** | Any-rate devices; ALSA card remember + 100% set |

---

## Division of labor

| Owner | Scope |
|-------|--------|
| **Pi / packaging (this handoff)** | debs, servers, digi Virtual*, PortAudio, init GUI, docs |
| **Client developer** | WPF MSCC client (spectrum UI, GRID, pan display); Avalonia if pursued later |

---

## Contact context

- Hardware: Pi 5, Multus Proficio, operator USB/HAT audio as configured.  
- Digi: WSJT-X (often on Pi via VNC) + VirtualA/B.  
- Headless = servers without GUI on Pi; operate from PC client.

---

*Update this file when shipping a new `.deb` or changing digi / PortAudio / pan contracts.*
