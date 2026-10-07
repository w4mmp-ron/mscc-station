# MSCC / Proficio station monorepo

Workspace for Multus SDR / MSCC (Multus SDR Control Console) / Proficio: **Linux servers**, **Windows & Linux UIs**, **PIC keyer**, **PSoC firmware**, and **STM32F411** migration.

| | |
|--|--|
| **GitHub** | https://github.com/w4mmp-ron/mscc-station |
| **Owner** | w4mmp-ron (Ron) |
| **Collaborator** | n8vet (Stew) (UI + Windows client / PCB) |
| **Problem reports** | https://multussdr.groups.io/g/main/topics |

```bash
git clone https://github.com/w4mmp-ron/mscc-station.git
```

---

## Operators: start here

| What | Where |
|------|-------|
| **Installers** (pick your computer) | [`installers/`](installers/) — [`windows/`](installers/windows/), [`linux/`](installers/linux/) (Ubuntu amd64), [`rpi/`](installers/rpi/) (Pi OS 64-bit) |
| **What changed** | Windows: [`installers/windows/CHANGELOG.md`](installers/windows/CHANGELOG.md) · Ubuntu: [`installers/linux/CHANGELOG.md`](installers/linux/CHANGELOG.md) · Raspberry Pi: see git history for `installers/rpi/` |
| **Operator's guides (PDF)** | [`docs/manuals/`](docs/manuals/README.md) — Windows Operator's Guide Release 0; Linux/Raspberry Pi and Remote (preliminary) |
| **Problems / questions** | https://multussdr.groups.io/g/main/topics |

Do not install `*_arm64.deb` on Ubuntu or `*_amd64.deb` on a Pi.

---

## Product stack (quick)

```text
  Operator UI                          Radio host
  ┌─────────────────────┐              ┌──────────────────────────┐
  │ Windows WPF         │◄── UDP ────►│ ms-sdr + sdrcore-recv/trans│
  │ Linux Avalonia      │   opcodes   │ (Linux Pi or Windows)      │
  └──────────┬──────────┘              └────────────┬─────────────┘
             │                                      │ USB / I²C
   optional: MsccRemotePhones                 ┌─────┴──────┐
   (operator AF over UDP)                     ▼            ▼
                                         PSoC / STM32    PIC keyer
```

**Digital apps** can stay on the radio host. **Operator phones/mic** can be remote (see `mscc-remote-audio/`).

---

## UI — `mscc-ui/`

| Path | Role |
|------|------|
| **`mscc-ui/windows-work-tree/`** | **Windows WPF** client (`MSCC.Wpf`), shared **`MSCC.Core`**, Windows servers (`ms-sdr-MKII`, recv/trans), **`MSCC-Remote`** host helper |
| **`mscc-ui/Avalonia-Migration/`** | **Linux Avalonia** UI (`mscc-ui` deb, Pi and Ubuntu) — parity with WPF; references **MSCC.Core** from the Windows tree |

| Concern | Edit |
|---------|------|
| WPF operate UI / Settings / CW | `mscc-ui/windows-work-tree/mscc-mscc/mscc-new/` |
| Opcodes / UDP / radio service | `…/src/MSCC.Core/` (shared by Avalonia) |
| Avalonia / Pi GUI | `mscc-ui/Avalonia-Migration/` |
| Windows deploy | typically `C:\mscc-net9` |

**Rule of thumb:** stabilize features on **WPF**, keep **Avalonia** in sync; protocol changes go in **MSCC.Core** once.

---

## Two Linux trees (do not mix)

Pi work lives under **`rpi/`**. Ubuntu laptop work lives under **`linux/`**. Treat `rpi/` as a **guide** when changing Ubuntu; do **not** edit `rpi/` for x86_64 fixes.

| Tree | Who / host | How-to |
|------|------------|--------|
| **`rpi/`** | Ron / Raspberry Pi OS **arm64** | [`rpi/README.md`](rpi/README.md), [`installers/rpi/INSTALL.md`](installers/rpi/INSTALL.md) |
| **`linux/`** | n8vet (Stew) / Ubuntu Desktop **x86_64** | [`linux/README.md`](linux/README.md), [`installers/linux/INSTALL.md`](installers/linux/INSTALL.md), [`INSTALL-UBUNTU.md`](INSTALL-UBUNTU.md) (from source) |
| **`linux-build/`** | Scripts | Ubuntu: `mscc-linux.sh`. Pi cross (optional): `cross-arm64.sh` uses **`rpi/`** |

**Current kits:** [`installers/`](installers/). When a package is rebuilt, copy the newest file there (`./linux-build/drop-installers.sh`) and add an entry to that folder's `CHANGELOG.md` (Windows and Ubuntu).

## Pi install kit (RPi)

| Path | Notes |
|------|--------|
| **`installers/rpi/`** | Current `.deb` packages (mscc, mscc-ui, mscc-init, mscc-portaudio, optional mscc-firmware and proficio-flash-tools) |
| **`installers/rpi/INSTALL.md`** | Pi install steps; full guide [`rpi/mscc-deb/INSTALL-FOR-PI.md`](rpi/mscc-deb/INSTALL-FOR-PI.md) |
| **`rpi/mscc-deb/`**, **`rpi/mscc-binaries/`** | Server packaging (AArch64) |

## Linux backends

| Path | Notes |
|------|--------|
| `rpi/ms-sdr-linux/` / `linux/ms-sdr-linux/` | Command hub (Pi / Ubuntu) |
| `rpi/SDRcore-recv-linux/` / `linux/SDRcore-recv-linux/` | RX DSP |
| `rpi/SDRcore-trans-linux/` / `linux/SDRcore-trans-linux/` | TX DSP |
| `rpi/psoc-usb-bootload-linux/` / `linux/psoc-usb-bootload-linux/` | Firmware **CLI** (`make` → `bootloader`) + **GUI** (`bootloader-gui.py`) — not the same file |
| `rpi/mscc-init-gui/` | Init wizard (`mscc-init`, Architecture: all `.deb`) |
| `factory/` | Factory calibration tables per radio line (shipped in the `mscc` debs at `/usr/share/mscc/factory/` and next to the Windows servers) |

---

## Remote audio

| Path | Notes |
|------|--------|
| `mscc-remote-audio/` | **MsccRemotePhones** (Windows), test tools, notes |
| Opcode | `CMD_SET_AUDIO_DEVICE` (`0x9B`): **0** Digital, **1** Phones local, **2** Remote |
| Ports | RX phones **9100**, TX mic **9101** (MSA1) |

See `docs/manuals/` (Remote Operation guide) and `mscc-remote-audio/STEW-REMOTE-AUDIO.md`.

---

## Firmware

### PSoC (shipping / reference)

| Path | Notes |
|------|--------|
| [`radio-psoc-firmware/`](radio-psoc-firmware/README.md) | PSoC Creator trees for all radios (Proficio, Geminus, Ultimus) |
| `radio-psoc-firmware/Proficio-MKII-PTT/` | MKII PTT |
| `radio-psoc-firmware/Proficio-MKII-ATU/` | MKII ATU |
| `radio-psoc-firmware/Proficio-Legacy/` | Legacy Proficio |
| `radio-psoc-firmware/Proficio-bootloader/` | PSoC Creator bootloader project |
| `radio-psoc-firmware/release/<RadioName>/` | Shipping `.cyacd` / `.hex` (packaged as the optional `mscc-firmware` deb → `/usr/share/mscc/firmware/`) |
| `linux/psoc-usb-bootload-linux/` | Ubuntu firmware **upload** tools (CLI + GUI) |
| `rpi/psoc-usb-bootload-linux/` | Pi firmware upload tools (guide for Ubuntu) |

### PIC keyer

| Path | Notes |
|------|--------|
| `keyer/` | PIC16F18326 sources, memory docs, hex |

### STM32F411 Black Pill (PSoC replacement)

Moved to a dedicated repo (not in this tree):

**https://github.com/w4mmp-ron/psoc-replacement-stm32**

Contains `proficio-stm32f411-25MHz/` (production) and `proficio-stm32f411-8MHz/` (lab). Pi flash tools: `proficio-flash-tools` deb in `installers/rpi/`.

---

## Other

| Path | Notes |
|------|--------|
| `Solidus/` | Archived Solidus snapshot + current Solidus display work (Ron) |
| `swr-meter/` | External SWR helper |
| `linux/tty0tty-master/` | Virtual serial (Ubuntu) |
| `rpi/tty0tty-master/` | Virtual serial (Pi) |

---

## Owners (informal)

| Area | Primary |
|------|---------|
| Linux servers, packaging, remote AF path, STM32 FW | Ron |
| Windows WPF, Avalonia parity, MSCC.Core client, daughter PCB / pinout | n8vet (Stew) |

When one side changes opcodes or host behavior, note it for the other trees (`linux/` ↔ `rpi/` ↔ Windows servers, WPF ↔ Avalonia).
