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

## Pi install kit (start here for RPi)

| Path | Notes |
|------|--------|
| **`pi-install/`** | **Current `.deb` packages + one how-to** — servers, Avalonia UI, remote audio, firmware upload, upgrades |
| **`pi-install/INSTALL.md`** | End-to-end install flow |
| **`pi-install/packages/`** | Latest: portaudio, `mscc`, init-gui, `mscc-ui` |

## Linux backends & packaging

| Path | Notes |
|------|--------|
| `ms-sdr-linux/` | Linux command hub |
| `SDRcore-recv-linux/` | RX DSP (+ remote phones stream) |
| `SDRcore-trans-linux/` | TX DSP (+ remote mic UDP) |
| `mscc-deb/` | Pi `.deb` packaging / longer install prose |
| `mscc-binaries/` | Built server binaries |
| `mscc-init-linux/` / `mscc-init-gui/` / `mscc-init-files-linux/` | Init tools & seed INIs |
| `mscc-portaudio/` / `portaudio*` / `oboe-main/` | Audio stack / packaging |

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
| `Release-Proficio-MKII-PTT/` | MKII PTT |
| `Release-Proficio-MKII-ATU/` | MKII ATU |
| `Release-Proficio-Legacy/` | Legacy Proficio |
| `bootloader/` / `psoc-usb-bootload-linux/` | Bootload tools |

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
| `tty0tty-master/` | Virtual serial (Linux) |

---

## Owners (informal)

| Area | Primary |
|------|---------|
| Linux servers, packaging, remote AF path, STM32 FW | Ron |
| Windows WPF, Avalonia parity, MSCC.Core client, daughter PCB / pinout | Stew |

When one side changes opcodes or host behavior, note it for the other tree (Linux ↔ Windows servers, WPF ↔ Avalonia).
