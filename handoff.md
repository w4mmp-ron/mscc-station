# MSCC station — session handoff

**Read this first** before editing or building anything in a new Grok / Cursor / Claude session.

| | |
|--|--|
| **Canonical working folder** | `C:\Users\n8vet\OneDrive\Documents\GitHub\mscc-station` |
| **GitHub** | https://github.com/w4mmp-ron/mscc-station |
| **Do NOT use** | `C:\Users\n8vet\OneDrive\Documents\MSCC-Grok-Build` (stale / parallel copy — wrong tree) |

When opening Grok Build (Windows or Ubuntu):

```bash
cd /path/to/mscc-station
grok
```

On Windows PowerShell:

```powershell
cd C:\Users\n8vet\OneDrive\Documents\GitHub\mscc-station
grok
```

---

## Who does what

| Person | Focus |
|--------|--------|
| **Stew** | UI (WPF + Avalonia), Windows client / PCB, shared `MSCC.Core` |
| **Ron** | Linux servers, Pi packaging, `.deb` builds, Linux/Ubuntu build host |

---

## Current status (as of 2026-09-09)

### Done and working

- **FM (NFM)** end-to-end on Windows (UI + Windows servers) verified earlier.
- **Linux server sources** aligned with Windows for FM:
  - Wire mode **5** → DSP **MODE_FM = 6**
  - **`CMD_SET_FM_POWER` `0x9E`** (dedicated FM power; not AM)
  - TX `fm_modulate`, RX FM / AGC-bypass path
- **Ron rebuilt Linux servers** (night of 2026-09-08/09) — **works on the Pi**.
- Package built: **`rpi/mscc-deb/mscc_1.0.42_arm64.deb`** (present locally; packaging `Version:` is **1.0.42**).
- Avalonia UI with FM + FM Power + TX IQ table: **`mscc-ui_0.6.44_arm64.deb`** in `rpi/pi-install/packages/` and under `mscc-ui/Release/avalonia/arm64/`.
- **Ubuntu amd64 laptop** (`stew-HP-Notebook`, Ubuntu 26.04): **`INSTALL-UBUNTU.md`**. Servers from `linux-build/mscc-linux.sh` using **`linux/`** sources; UI deb `mscc-ui/Release/avalonia/x86_64/`. Pi drop is `Release/avalonia/arm64/`.

### Follow-ups / keep in mind

- Confirm **`mscc_1.0.42_arm64.deb`** is copied into **`rpi/pi-install/packages/`** (kit still listed **1.0.41** there last check — refresh kit if needed).
- Update **`rpi/pi-install/INSTALL.md`** package version lines when the kit is refreshed.
- Ubuntu laptop: edit **`linux/`** only. Treat **`rpi/`** as a guide; do not change it for x86_64. Pi packages still from Ron’s `rpi/` + Pi rebuild.
- **Current installers** for GitHub web: **`installers/{linux,rpi,windows}/`**. After a kit build, run **`./linux-build/drop-installers.sh`** (or the build script that already calls it). History stays in builder folders (`rpi/mscc-deb/`, `linux/mscc-deb/`, `rpi/mscc-init-gui/`, …).

---

## Repo map

```text
mscc-station/
  handoff.md
  README.md
  INSTALL-UBUNTU.md          ← Ubuntu x86_64 install
  linux-build/               ← Ubuntu scripts (mscc-linux.sh, cross-arm64.sh, drop-installers.sh)
  linux/                     ← Ubuntu x86_64 sources (edit here, not rpi/)
  installers/{linux,rpi,windows}/  ← current kits for GitHub web
  rpi/                       ← Ron’s Pi trees (guide only for Ubuntu work)
    ms-sdr-linux/ SDRcore-*-linux/ mscc-deb/ mscc-binaries/ pi-install/
  Proficio-firmware/         ← PSoC Creator trees (was repo-root Release-Proficio-*)
  mscc-ui/Release/avalonia/arm64/   ← Pi debs
  mscc-ui/Release/avalonia/x86_64/  ← Ubuntu UI + init-gui

  # UI (Stew)
  mscc-ui/
    Avalonia-Migration/      ← Linux Avalonia UI
    windows-work-tree/       ← WPF + MSCC.Core + Windows ms-sdr/recv/trans
    Release/avalonia/        ← published UI debs / drop folder

  # Firmware / other
  keyer/
  Release-Geminus-*/
  mscc-remote-audio/
  …
```

### Parallel trees (keep FM / protocol in sync)

| Ubuntu (`linux/`) | Pi guide (`rpi/`) | Windows |
|-------------------|-------------------|---------|
| `linux/ms-sdr-linux/` | `rpi/ms-sdr-linux/` | `mscc-ui/windows-work-tree/ms-sdr-MKII/` |
| `linux/SDRcore-recv-linux/` | `rpi/SDRcore-recv-linux/` | `mscc-ui/windows-work-tree/SDRcore-recv/` |
| `linux/SDRcore-trans-linux/` | `rpi/SDRcore-trans-linux/` | `mscc-ui/windows-work-tree/SDRcore-trans/` |

Shared client opcodes / UDP: `mscc-ui/windows-work-tree/.../MSCC.Core/` (Avalonia references this).

---

## Ubuntu laptop / Linux build host (Ron)

**Purpose:** edit + compile Linux servers without using Windows; package `.deb` for the Pi.

### One-time setup

```bash
# Repo
cd ~
git clone https://github.com/w4mmp-ron/mscc-station.git
cd ~/mscc-station

# Grok Build (already installed on Stew's Ubuntu laptop)
# curl -fsSL https://x.ai/cli/install.sh | bash
# then: grok   (sign in)

# Build tools (servers)
sudo apt update
sudo apt install -y build-essential g++ libusb-1.0-0-dev libhidapi-libusb0 \
  dpkg-dev

# PortAudio for digi (same as Pi) — from this repo's package if targeting Pi-compatible layout:
# sudo apt install -y ./rpi/pi-install/packages/mscc-portaudio_19.8.2_arm64.deb
# Note: that .deb is arm64. On an x86_64 Ubuntu laptop you can still *edit* and
# cross-check sources; produce shipping binaries on Pi (arm64) or a Pi-like arm64 host.
```

**Important:** Shipping `mscc_*_arm64.deb` binaries must be **AArch64**.  
- **Raspberry Pi** = correct architecture for final server binaries.  
- **x86_64 Ubuntu laptop** = great for source edit, git, Grok, docs; for a shippable arm64 deb, build on the **Pi** (or an arm64 VM/board).

### Day-to-day on Linux (after `git pull`)

```bash
cd ~/mscc-station
git pull
grok    # optional — work in this folder only

# On the Pi (arm64), rebuild servers from rpi/:
mscc stop
cd ~/mscc-station/rpi/SDRcore-recv-linux  && make clean && make
cd ~/mscc-station/rpi/SDRcore-trans-linux && make clean && make
cd ~/mscc-station/rpi/ms-sdr-linux        && make clean && make
# ldd $HOME/mscc/sdrcore-recv | grep portaudio   # expect /usr/local/lib

# Package:
cp -a $HOME/mscc/{sdrcore-recv,sdrcore-trans,ms-sdr} ~/mscc-station/rpi/mscc-binaries/
# bump Version: in rpi/mscc-deb/packaging/DEBIAN/control if needed
cd ~/mscc-station/rpi/mscc-deb && ./build-deb.sh
./install-mscc.sh ./mscc_<Version>_arm64.deb
cp -a ./mscc_<Version>_arm64.deb ~/mscc-station/rpi/pi-install/packages/
```

Longer checklist: **`rpi/mscc-deb/BUILD-SERVERS-ON-PI.md`**.

Ubuntu laptop (x86_64) is **not** this path: use **`linux/`** + `./linux-build/mscc-linux.sh all`. Never copy `$HOME/mscc` x86 ELFs into `rpi/mscc-binaries/`.

---

## Windows (Stew) — UI only reminder

```powershell
cd C:\Users\n8vet\OneDrive\Documents\GitHub\mscc-station
```

| Task | Where |
|------|--------|
| Avalonia UI | `mscc-ui/Avalonia-Migration/` |
| WPF / Core | `mscc-ui/windows-work-tree/` |
| Publish UI deb | Avalonia scripts → `mscc-ui/Release/avalonia/arm64/` → copy to `rpi/pi-install/packages/` |
| Windows servers | `mscc-ui/windows-work-tree/ms-sdr-MKII`, `SDRcore-recv`, `SDRcore-trans` |

Do **not** expect Windows to produce Pi arm64 server binaries.

---

## Install kit (operators / Pi)

| Order | Package | Example |
|-------|---------|---------|
| 1 | PortAudio | `mscc-portaudio_19.8.2_arm64.deb` |
| 2 | Servers | `mscc_1.0.42_arm64.deb` (Ron’s FM build — prefer over 1.0.41) |
| 3 | Init GUI | `mscc-init-gui_1.0.13_all.deb` |
| 4 | Avalonia UI | `mscc-ui_0.6.44_arm64.deb` |

How-to: **`rpi/pi-install/INSTALL.md`**. Ubuntu: **`INSTALL-UBUNTU.md`**.

---

## For the next AI session

1. **Working directory must be `mscc-station`**, not `MSCC-Grok-Build`.
2. Read this **`handoff.md`**, then **`README.md`** if needed.
3. FM Linux **source** ↔ Windows **source** are in line; Ron’s **1.0.42** build works on Pi.
4. Ask before destructive git / force-push / mass deletes.
5. Prefer editing only the trees listed above for the requested feature.

### Suggested first message when resuming

> Open `C:\Users\n8vet\OneDrive\Documents\GitHub\mscc-station` (or `~/mscc-station` on Ubuntu). Read `handoff.md`. Continue from there — do not use MSCC-Grok-Build.

---

## Quick links

| Doc | Path |
|-----|------|
| This handoff | `handoff.md` |
| Repo map | `README.md` |
| Ubuntu laptop install | `INSTALL-UBUNTU.md` |
| Current kits (web) | `installers/` |
| Ubuntu sources | `linux/` |
| Pi operator install | `rpi/pi-install/INSTALL.md` |
| Rebuild servers on Pi | `rpi/mscc-deb/BUILD-SERVERS-ON-PI.md` |
| FM notes (UI) | `mscc-ui/FM-FROM-GSDR.md` (if present) |
