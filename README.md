# MSCC / Proficio station monorepo

Private workspace for Multus SDR / MSCC / Proficio: **Linux servers**, **Windows & Linux UIs**, **PIC keyer**, **PSoC firmware**, and **STM32F411** migration.

| | |
|--|--|
| **GitHub** | https://github.com/w4mmp-ron/mscc-station |
| **Owner** | w4mmp-ron (Ron) |
| **Collaborator** | Stew (UI + Windows client / PCB) |

```bash
git clone https://github.com/w4mmp-ron/mscc-station.git
```

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

Active **client** work for both platforms lives here:

| Path | Role |
|------|------|
| **`mscc-ui/windows-work-tree/`** | **Windows WPF** client (`MSCC.Wpf`), shared **`MSCC.Core`**, Windows servers (`ms-sdr-MKII`, recv/trans), **`MSCC-Remote`** host helper |
| **`mscc-ui/Avalonia-Migration/`** | **Linux Avalonia** UI (`mscc-ui` deb for Pi) — parity with WPF; references **MSCC.Core** from the Windows tree |

| Concern | Edit |
|---------|------|
| WPF operate UI / Settings / CW | `mscc-ui/windows-work-tree/mscc-mscc/mscc-new/` |
| Opcodes / UDP / radio service | `…/src/MSCC.Core/` (shared by Avalonia) |
| Avalonia / Pi GUI | `mscc-ui/Avalonia-Migration/` |
| Windows deploy | typically `C:\mscc-net9` |

**Rule of thumb:** stabilize features on **WPF**, keep **Avalonia** in sync; protocol changes go in **MSCC.Core** once.

More detail (older layout notes may still appear under `mscc-ui/README.md`) — prefer this root map when paths conflict.

---

## Two Linux trees (do not mix)

Ron moved Pi work under **`rpi/`**. Ubuntu laptop work lives under **`linux/`**. Treat `rpi/` as a **guide** when changing Ubuntu; do **not** edit `rpi/` for x86_64 fixes.

| Tree | Who / host | How-to |
|------|------------|--------|
| **`rpi/`** | Ron / Raspberry Pi OS **arm64** | [`rpi/README.md`](rpi/README.md), [`rpi/pi-install/INSTALL.md`](rpi/pi-install/INSTALL.md) |
| **`linux/`** | Stew / Ubuntu Desktop **x86_64** | [`linux/README.md`](linux/README.md), [`INSTALL-UBUNTU.md`](INSTALL-UBUNTU.md) |
| **`linux-build/`** | Scripts | Ubuntu: `mscc-linux.sh`. Pi cross (optional): `cross-arm64.sh` uses **`rpi/`** |

**Current kits (GitHub web):** [`installers/`](installers/) — `linux/` (Ubuntu amd64), `rpi/` (Pi arm64), `windows/`. When a package is rebuilt, copy the newest file there (`./linux-build/drop-installers.sh`). Do not install `*_arm64.deb` on Ubuntu.

## Pi install kit (RPi)

| Path | Notes |
|------|--------|
| **`rpi/pi-install/`** | Current `.deb` packages + how-to |
| **`rpi/pi-install/INSTALL.md`** | End-to-end Pi install |
| **`rpi/mscc-deb/`**, **`rpi/mscc-binaries/`** | Server packaging (AArch64) |

## Linux backends

| Path | Notes |
|------|--------|
| `rpi/ms-sdr-linux/` / `linux/ms-sdr-linux/` | Command hub (Pi / Ubuntu) |
| `rpi/SDRcore-recv-linux/` / `linux/SDRcore-recv-linux/` | RX DSP |
| `rpi/SDRcore-trans-linux/` / `linux/SDRcore-trans-linux/` | TX DSP |
| `rpi/psoc-usb-bootload-linux/` / `linux/psoc-usb-bootload-linux/` | Firmware **CLI** (`make` → `bootloader`) + **GUI** (`bootloader-gui.py`) — not the same file |
| `rpi/mscc-init-gui/` | Init wizard (Architecture: all `.deb`) |

---

## Remote audio

| Path | Notes |
|------|--------|
| `mscc-remote-audio/` | **MsccRemotePhones** (Windows), test tools, Stew handoff |
| Opcode | `CMD_SET_AUDIO_DEVICE` (`0x9B`): **0** Digital, **1** Phones local, **2** Remote |
| Ports | RX phones **9100**, TX mic **9101** (MSA1) |

Client UI still needs the **Remote Audio** checkbox (Phones + checked → send **2**). See `mscc-remote-audio/STEW-REMOTE-AUDIO.md`.

---

## Firmware

### PSoC (shipping / reference)

| Path | Notes |
|------|--------|
| [`Proficio-firmware/`](Proficio-firmware/README.md) | PSoC Creator trees (moved from repo root) |
| `Proficio-firmware/Release-Proficio-MKII-PTT/` | MKII PTT |
| `Proficio-firmware/Release-Proficio-MKII-ATU/` | MKII ATU |
| `Proficio-firmware/Release-Proficio-Legacy/` | Legacy Proficio |
| `Proficio-firmware/bootloader/` | PSoC Creator bootloader project |
| `linux/psoc-usb-bootload-linux/` | Ubuntu firmware **upload** tools (CLI + GUI) |
| `rpi/psoc-usb-bootload-linux/` | Pi firmware upload tools (guide for Ubuntu) |

### PIC keyer

| Path | Notes |
|------|--------|
| `keyer/` | PIC16F18326 sources, memory docs, hex |

### STM32F411 Black Pill (PSoC replacement — in progress)

| Path | Notes |
|------|--------|
| `proficio-stm32f411-25MHz/` | HSE **25 MHz** tree (recent bring-up commits) |
| `proficio-stm32f411-8MHz/` | HSE **8 MHz** twin — keep docs in sync until one crystal is chosen |

**Pinout (locked on PCB — docs in each tree’s `docs/`):**

| Net | STM32 |
|-----|--------|
| **RESET** (PCM3060) | **PA2** |
| **BOOT** | **PA8** |
| **USBV+** (sense, ÷ → 3.3 V) | **PA9** |

Start: `docs/STEW-DAUGHTER-BOARD-PINOUT.md` in either STM32 folder. Firmware pin macros are Ron’s follow-up to match that lock.

---

## Other

| Path | Notes |
|------|--------|
| `swr-meter/` | External SWR helper |
| `linux/tty0tty-master/` | Virtual serial (Ubuntu) |
| `rpi/tty0tty-master/` | Virtual serial (Pi) |

---

## Owners (informal)

| Area | Primary |
|------|---------|
| Linux servers, packaging, remote AF path, STM32 FW | Ron |
| Windows WPF, Avalonia parity, MSCC.Core client, daughter PCB / pinout | Stew |

When one side changes opcodes or host behavior, note it for the other trees (`linux/` ↔ `rpi/` ↔ Windows servers, WPF ↔ Avalonia).
