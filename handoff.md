# MSCC station — session handoff

**Read this first** before editing or building anything in a new Grok / Cursor / Claude session.

Grok auto-loads repo-root [`AGENTS.md`](AGENTS.md) (trees, installer drop, coord bus, hard don'ts). Keep **AGENTS.md** short and standing; put current state here.

| | |
|--|--|
| **Canonical working folder** | `C:\Users\n8vet\OneDrive\Documents\GitHub\mscc-station` |
| **GitHub** | https://github.com/w4mmp-ron/mscc-station |
| **Do NOT use** | `C:\Users\n8vet\OneDrive\Documents\MSCC-Grok-Build` (stale / parallel copy — wrong tree) |

```powershell
cd C:\Users\n8vet\OneDrive\Documents\GitHub\mscc-station
grok
```

On Ubuntu: `cd ~/mscc-station` (use `pwd -P` for Avalonia publish), then `grok`.

---

## Who does what

| Person | Focus |
|--------|--------|
| **n8vet (Stew)** | UI (WPF + Avalonia), Windows client / servers, Ubuntu amd64 kit, shared `MSCC.Core`, PCB |
| **Ron** | Pi (arm64) servers and packaging under `rpi/`, Solidus, STM32 |

---

## Current state (2026-10-07)

### Shipped kits (`installers/`)

| Platform | Current packages | Change log |
|----------|------------------|------------|
| Windows | `mscc-net9-R10-4-0-install.exe` (WPF 10.4.0) | [`installers/windows/CHANGELOG.md`](installers/windows/CHANGELOG.md) |
| Ubuntu amd64 | `mscc_1.0.49`, `mscc-ui_0.6.71`, `mscc-init-gui_1.0.13`, `mscc-portaudio_19.8.2`, optional `mscc-firmware_1.0.0` | [`installers/linux/CHANGELOG.md`](installers/linux/CHANGELOG.md) |
| Raspberry Pi arm64 | `mscc_1.0.56`, `mscc-ui_0.6.71`, `mscc-init_1.0.18`, `mscc-portaudio_19.8.3`, optional `proficio-flash-tools_1.0.3`, `mscc-firmware_1.0.0` | git history of `installers/rpi/` (Ron) |

All three platforms have factory calibration tables plus per-radio cal park/restore with **Save settings** (opcode `0x29`). Manuals: [`docs/manuals/`](docs/manuals/README.md) (Windows Release 0; Linux/Remote preliminary).

### Open / pending

- **Ubuntu kit rebuild pending:** recv DC blocker + full FFT (recv 3.143), 800-bin spectrum fix, and CW PHONES removal in Avalonia are in source but **not** in the amd64 installer (`mscc_1.0.49` / `mscc-ui_0.6.71` amd64 predate them).
- **Spectrum largest-bin per display point:** done on the Pi (`mscc` 1.0.56); port to Windows / Ubuntu recv, and client still draws one data point per pixel (WPF + Avalonia).
- **USB-on-load LAST_MODE fix.**
- **CAT MD mode map bug** on the Linux server; **CAT mode-set parity** across Windows / Linux.
- **FM** still unfinished.
- **DIG-CW** design.
- **FREQ CAL progress bar reset.**
- **Client: ask / warn if radio firmware is missing.**
- **LAST_HF / LF digi defaults.**
- **TX IQ phase adjust** — back-burner.
- **EVENT logging cleanup** (remote_mic / mic TX diagnostic EVENT logs).
- **cmd-034** (clear-on-TX + fixed 2:1 fill) — on hold.
- Linux / Remote manuals need a regen pass (SPECTRUM RESOLUTION, Save settings, PHONES removed).
- Ron (Pi side, FYI only): Si5351 drive-level plan (`rpi/si5351-drive.md`), Solidus display plan (`Solidus/solidus-display.md`).

---

## Repo map

```text
mscc-station/
  handoff.md  README.md  AGENTS.md
  INSTALL-UBUNTU.md          ← Ubuntu x86_64 build from source
  installers/{linux,rpi,windows}/  ← current kits for GitHub web (+ CHANGELOG.md for windows/linux)
  docs/manuals/              ← operator guides (PDF)
  factory/                   ← factory cal tables (shipped in all kits)
  linux-build/               ← Ubuntu scripts (mscc-linux.sh, cross-arm64.sh, drop-installers.sh)
  linux/                     ← Ubuntu x86_64 sources (edit here, not rpi/)
  rpi/                       ← Ron's Pi trees (read-only from non-Pi hosts)
  radio-psoc-firmware/       ← PSoC Creator trees + release/ (Shack builds only)
  mscc-ui/
    Avalonia-Migration/      ← Linux Avalonia UI
    windows-work-tree/       ← WPF + MSCC.Core + Windows ms-sdr/recv/trans
    Release/                 ← builder drops / history
  mscc-remote-audio/  keyer/  Solidus/ (Ron)  swr-meter/
```

### Parallel trees (keep protocol in sync)

| Ubuntu (`linux/`) | Pi (`rpi/`) | Windows |
|-------------------|-------------|---------|
| `linux/ms-sdr-linux/` | `rpi/ms-sdr-linux/` | `mscc-ui/windows-work-tree/ms-sdr-MKII/` |
| `linux/SDRcore-recv-linux/` | `rpi/SDRcore-recv-linux/` | `mscc-ui/windows-work-tree/SDRcore-recv/` |
| `linux/SDRcore-trans-linux/` | `rpi/SDRcore-trans-linux/` | `mscc-ui/windows-work-tree/SDRcore-trans/` |

Shared client opcodes / UDP: `mscc-ui/windows-work-tree/mscc-mscc/mscc-new/src/MSCC.Core/` (Avalonia references this).

---

## Build notes

- **Ubuntu (amd64):** sources in `linux/`; `./linux-build/mscc-linux.sh all`, debs via `linux-build/build-mscc-deb-amd64.sh` and `build-mscc-ui-deb-amd64.sh` (both drop into `installers/linux/`). See [`INSTALL-UBUNTU.md`](INSTALL-UBUNTU.md).
- **Pi (arm64):** Ron builds on the Pi from `rpi/`; see [`rpi/mscc-deb/BUILD-SERVERS-ON-PI.md`](rpi/mscc-deb/BUILD-SERVERS-ON-PI.md). Shipping `mscc_*_arm64.deb` must contain AArch64 binaries; never copy Ubuntu `$HOME/mscc` ELFs into `rpi/mscc-binaries/`.
- **Windows:** WPF / servers in `mscc-ui/windows-work-tree/`; installer → `installers/windows/`, history in `mscc-ui/Release/windows-wpf/`.
- **PSoC firmware:** Shack only (Keil). Shipping files in `radio-psoc-firmware/release/`.
- After any kit build: copy the newest file into `installers/<platform>/` and add a `CHANGELOG.md` entry (windows / linux).

---

## For the next AI session

1. Working directory must be `mscc-station`, not `MSCC-Grok-Build`.
2. Read this `handoff.md`, then `AGENTS.md` / `README.md`.
3. Do not edit `rpi/` or `Solidus/` from non-Pi hosts.
4. Ask before destructive git / force-push / mass deletes.

## Quick links

| Doc | Path |
|-----|------|
| Repo map | `README.md` |
| Standing rules | `AGENTS.md` |
| Ubuntu build from source | `INSTALL-UBUNTU.md` |
| Current kits | `installers/` |
| Pi operator install | `installers/rpi/INSTALL.md` |
| Rebuild servers on Pi | `rpi/mscc-deb/BUILD-SERVERS-ON-PI.md` |
| Manuals | `docs/manuals/` |
