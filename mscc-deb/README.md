# MSCC for Raspberry Pi OS — Debian package

**Package:** `mscc_*_arm64.deb` (current: **1.0.27**)  
**Platform:** Raspberry Pi **4 / 5**, **64-bit Raspberry Pi OS** only  
**Radio:** Multus / Proficio (USB I/Q + control)

This repository builds and documents the **arm64 `.deb`** that installs the MSCC server stack on a Pi: `ms-sdr`, `sdrcore-recv`, `sdrcore-trans`, configuration seeds, digi virtual audio, desktop Start/Stop, and the tty0tty null-modem module.

| Related packages (sibling trees) | Current example | Purpose |
|----------------------------------|-----------------|----------|
| **`mscc-portaudio_*_arm64.deb`** | **19.8.2** | Pulse+ALSA PortAudio → `/usr/local` (**install first**) |
| **`mscc-init-gui_*_all.deb`** | **1.0.10** | Graphical setup wizard (menu: **MSCC Init**) |

End-user install: **[INSTALL-FOR-PI.md](INSTALL-FOR-PI.md)** (also `.docx`; PDF may lag).  
Developer handoff / status: **[resume.md](resume.md)**.

---

## Install order (operators)

```text
1. mscc-portaudio  (PortAudio @ /usr/local)
2. mscc            (this package)
3. mscc-init-gui   (optional; recommended)
```

```bash
sudo apt install -y ./mscc-portaudio_19.8.2_arm64.deb
sudo apt install -y ./mscc_1.0.27_arm64.deb
sudo apt install -y ./mscc-init-gui_1.0.10_all.deb
ldd $HOME/mscc/sdrcore-recv | grep portaudio
# expect: /usr/local/lib/libportaudio.so.2
```

Use **Terminal from the Pi desktop** for `apt install`. Then mostly **menu** under **Sound & Video**:

| Menu | Action |
|------|--------|
| **MSCC Init** | Configure (from mscc-init-gui package) |
| **MSCC Start** | Start servers (also re-applies operator ALSA levels to 100%) |
| **MSCC Stop** | Stop servers |

See **INSTALL-FOR-PI.md** for the full operator card.

---

## What this package installs

| Component | Location | Purpose |
|-----------|----------|---------|
| Servers | `$HOME/mscc/` | `ms-sdr`, `sdrcore-recv`, `sdrcore-trans` |
| Scripts | `$HOME/mscc/` | `mscc.sh`, `mscc-init` |
| Config | `$HOME/.local/mscc/` | Seeded **only if missing/empty** (never overwrites) |
| Links | `/usr/local/bin/` | `mscc`, `mscc-init`, `mscc-virtual-audio`, `mscc-desktop-ctl` |
| Desktop menu | `/usr/share/applications/` | **MSCC Start**, **MSCC Stop** |
| Virtual digi audio | `/usr/share/mscc/bin/mscc-virtual-audio.sh` | Pulse/PipeWire **VirtualA/B** null sinks |
| User systemd | `/usr/lib/systemd/user/mscc-virtual-audio.service` | Creates Virtual* at user login |
| Proficio udev | `/etc/udev/rules.d/99-proficio.rules` | USB 16c0:05dc access |
| Boot overlay | `config.txt` | `dtoverlay=audioinjector-wm8731-audio` (operator HAT) |
| tty0tty | built on target | `/dev/tnt0`… for CAT/PTT null-modem |

**Install user:** package postinst resolves `SUDO_USER` and installs into **that** user’s home — not root’s. No hard-coded `/home/ron` paths.

---

## Architecture (servers)

```text
MSCC client (PC, WPF)  ←UDP→  ms-sdr (:8888)  ←→  Proficio (USB control)
                                │
                                ├→ sdrcore-recv  (:9000)  I/Q RX + speakers / digi out
                                └→ sdrcore-trans (:9200)  I/Q TX + mics / digi in
```

**Headless:** servers run on the Pi without a GUI. The **MSCC client on the PC** is the operator console.

- **Operator (P) path:** Proficio I/Q (ALSA, always 96 kHz) + operator speaker/mic (any rate; resampler if needed).  
- **Digital (D) path:** Proficio I/Q (ALSA) + Pulse **VirtualA** / **VirtualB**.  
- PortAudio **cannot** mix ALSA + Pulse in one stream → **dual stream** (ring buffer).  
- Operator rate ≠ 96 kHz → dual stream + **Oboe** freestanding resampler (recv down, trans up).

---

## Digital audio (Pulse / PipeWire)

### Devices created by `mscc-virtual-audio`

| Sink | Rate | Typical use |
|------|------|-------------|
| **VirtualA** | 96 kHz | **sdrcore-recv** digi **output** |
| **VirtualB** | default (often 48 kHz) | Digi app **TX** (e.g. WSJT Output) |
| VirtualA_TX / VirtualB_TX | 96k / default | Optional second pair |

| Source | Typical use |
|--------|-------------|
| **VirtualA.monitor** | Digi app **RX** (e.g. WSJT Input) |
| **VirtualB.monitor** | **sdrcore-trans** digi **microphone** |

### Recommended wiring

```text
Proficio I/Q (ALSA) ──► sdrcore-recv ──play──► VirtualA
                                                │
                                                └─monitor──► WSJT Input

WSJT Output ──play──► VirtualB ──monitor──► sdrcore-trans digi mic
                                              │
                                              └──► Proficio I/Q TX (ALSA)
```

### Seed / init defaults (new empty config only)

- `digital-speaker.ini` → `VirtualA`  
- `digital-microphone.ini` → `VirtualB.monitor`  

**CLI `mscc-init` and GUI `mscc-init-gui`:** operator devices only; digi fixed as above.  
Existing config files are **never** overwritten on package upgrade.

ALSA `MSCC_Cable_*` / package `audio-setup` are **not** part of this package. Digi is Pulse/PipeWire Virtual* only.

**Digi levels:** Pulse (`pactl`), not `alsamixer`. Create script may leave sinks at 60%; raise VirtualA if WSJT RX is low:

```bash
pactl set-sink-volume VirtualA 100%
pactl set-sink-mute VirtualA 0
```

---

## PortAudio (required companion package)

Debian/Trixie `libportaudio2` is often **ALSA-only** (no usable Pulse host API for Virtual*).

| Package | Installs | Role |
|---------|----------|------|
| **`mscc-portaudio`** | `/usr/local/lib/libportaudio.so*` + ldconfig | Pulse + ALSA PortAudio for MSCC |
| **`mscc` 1.0.23+** | binaries with **rpath → /usr/local** | Loads the MSCC PortAudio at runtime |

Build PortAudio deb from sibling **`portaudio/build`** (AArch64), not from a home-only install for shipping.

`mscc.sh` prefers `/usr/local`, then falls back to `$HOME/portaudio-install` for old layouts. **`.bashrc` alone is not enough** for `nohup` / desktop Start.

---

## Operator levels (ALSA phones)

- **MSCC Init** sets operator card mixer controls to **100%** and writes `~/.local/mscc/operator-alsa-cards.txt`.  
- **`mscc start` / MSCC Start** **re-applies** 100% via `amixer` every time (USB card index can change).  
- Do **not** rely on `~/.asoundrc` or `alsactl` alone for sticky phone levels.  
- AF gain after that is the **MSCC client Volume** (and digi is Pulse).

---

## Spectrum / pan (recv)

| Feature | Server behavior | Client lockstep |
|---------|-----------------|-----------------|
| Log scale | bias **40**, clamp MAX_Y **16000** | `RawYToDb` bias **40** |
| Pan Resolution | index 0/1/2 → **800 / 1600 / 3200** bins (~72 kHz span) | UI control |
| Legacy refresh 3–10 | `G_Panadapter_Blocks` (update *rate*) | not bin count |

Bins = frequency slots across the pan (not sample rate). High/Max may look similar if the UI downsamples to fixed screen width.  
**GRID MIN/MAX** are client display only (not logged by `sdrcore-recv`).

---

## Commands

```bash
mscc start|stop|status|restart
mscc-desktop-ctl start|stop|status   # used by menu
mscc-init                            # CLI wizard
mscc-init-gui                        # GUI wizard (separate package)
mscc-virtual-audio                   # recreate Virtual* sinks now
systemctl --user status mscc-virtual-audio
```

Config/logs:

- Config: `$HOME/.local/mscc/`  
- App logs: often `$HOME/sdrcore-recv.log` or under `$HOME/.local/mscc/` (see open path in log)  
- Start stdout: `$HOME/mscc/logs/*.stdout`  

Useful grep:

```bash
grep -E "PAN RESOLUTION|DUAL STREAM|ALSA card|resample" ~/sdrcore-recv.log | tail -20
```

---

## Build this package (developer)

**Sibling trees:**

```text
worktrees/
  mscc-deb/                 ← this folder
  mscc-binaries/            ← prebuilt arm64 binaries + mscc.sh
  mscc-init-files-linux/    ← seed .ini files
  tty0tty-master/module/    ← tty0tty sources
  mscc-portaudio/           ← separate PortAudio .deb
  mscc-init-gui/            ← separate GUI init .deb
  portaudio/                ← PortAudio source + build/ for PA deb
  SDRcore-recv-linux/       ← rebuild sdrcore-recv on Pi
  SDRcore-trans-linux/      ← rebuild sdrcore-trans on Pi
  ms-sdr-linux/
  mscc-init-linux/
```

```bash
cd /path/to/worktrees/mscc-deb
./build-deb.sh
# → mscc_<Version>_arm64.deb
```

**Update binaries:** copy Pi-built arm64 files into `../mscc-binaries/` (linked against `/usr/local` PortAudio), bump `Version:` in `packaging/DEBIAN/control`, rebuild.

| Binary | Source tree |
|--------|-------------|
| `ms-sdr` | `ms-sdr-linux` |
| `sdrcore-recv` | `SDRcore-recv-linux` |
| `sdrcore-trans` | `SDRcore-trans-linux` |
| `mscc-init` | `mscc-init-linux` |
| `mscc.sh` | `mscc-binaries` (ALSA level re-apply on start) |

Makefiles for recv/trans use `PORTAUDIO_PREFIX=/usr/local` + rpath; accept `libportaudio.so*`.

---

## Groups and hardware

| Group | Why |
|-------|-----|
| `dialout` | `/dev/tnt*` CAT/PTT |
| `plugdev` | Proficio USB (with udev rule) |
| `audio` | Sound devices |

Log out/in after first install if groups were just added.

**AudioInjector HAT:** package may append `dtoverlay=audioinjector-wm8731-audio` — **reboot** once if newly added.

---

## WSJT-X (typical)

| Setting | Value |
|---------|--------|
| Input | `VirtualA.monitor` |
| Output | `VirtualB` |
| CAT | as in init (often `/dev/tnt0`) |

---

## Uninstall

```bash
sudo apt remove mscc
sudo apt remove mscc-init-gui mscc-portaudio   # if installed
```

User config under `$HOME/.local/mscc` is left in place.

---

## Important constraints

1. **Pi OS 64-bit only** (arm64 package).  
2. **Do not run servers as root.**  
3. **Install `mscc-portaudio` before relying on digi Virtual*.**  
4. **Config seed never overwrites** existing `.ini` files.  
5. **Dual stream** for ALSA I/Q + Pulse digi (and for rate mismatch).  
6. **Headless operate:** servers on Pi; MSCC client on PC.  
7. Operator levels: **re-applied on start** (ALSA `amixer`), not `.asoundrc`.

---

## Related documents

| File | Content |
|------|---------|
| [INSTALL-FOR-PI.md](INSTALL-FOR-PI.md) | Plain-English install for operators |
| [resume.md](resume.md) | Status, history, open items for developers |
| [PROJECT-MEMORY.md](PROJECT-MEMORY.md) | Longer engineering notes (if present) |

---

## License / ownership

MSCC / Multus SDR server binaries and DSP are Multus SDR / Multus-related licensing. Packaging scripts in this tree are for Multus MSCC Linux distribution on Raspberry Pi OS.
