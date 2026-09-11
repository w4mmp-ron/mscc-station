# Remote audio — punch list

**Date:** 2026-09-10  
**Status:** field-proven Windows client ↔ Linux/Pi servers (INI RX + opcode 2 mic). Next: mute + client-driven RX on **Linux servers**, then in-UI AF.

Related: [STEW-REMOTE-AUDIO.md](STEW-REMOTE-AUDIO.md), [README.md](README.md).

---

## What already works

| Path | How |
|------|-----|
| Control | WPF (or Avalonia) Connect → radio host UDP **8888** |
| Operator **mic TX** | Phones + **Remote Audio** → `CMD_SET_AUDIO_DEVICE` **2** (`0x9B`). MsccRemotePhones TX host = radio IP, port **9101** |
| Operator **phones RX** | Radio `~/.local/mscc/remote-phones.ini`: `ENABLED=1`, `HOST=` **client IPv4**, `PORT=9100`. Recv streams MSA1. Restart servers after INI change. MsccRemotePhones listen **9100** |
| Digital | Stays on the radio host (VirtualA/B). Not on this path yet |

**Windows companion:** `MsccRemotePhones.exe` beside WPF (`C:\mscc-net9`). Operator-side player, **not** a server GUI.  
**Linux operator:** no player yet. Avalonia skips the WinForms exe.

Opcode **2** does **not** start/stop the RX stream and does **not** mute the radio’s local phones.

---

## Direction (agreed)

Radio host stays the appliance. Operator AF + CAT live on the **operate PC** when Remote is on.

**UI (operate rail)**

- **Audio** button: **Phones ↔ Digital** only (never cycles through remote).
- **Remote** checkbox: master “this PC is the operator seat.” Do **not** grey it on Digital.
- Remote **on** → button text/function become **R-Phones** / **R-Digital**; open the AF popup.
- Remote **off** → back to the same local Phones/Digital; close AF; no popup.
- Last Phones/Digital and last Remote on/off are sticky independently.

| Remote | Audio button | Opcode `0x9B` | AF | CAT |
|--------|--------------|---------------|----|-----|
| off | Phones | 1 | Radio local phones | CAT on the radio host (ms-sdr) |
| off | Digital | 0 | Radio VirtualA/B (or local VAC if servers are here) | CAT on the radio host |
| on | R-Phones | 2 | MSA1 voice to this PC | This PC proxies CAT → Connect host |
| on | R-Digital | **3** (new) | Same VAC as local Digital, UDP to Connect host | Same CAT proxy |

WSJT-X devices and CAT port **do not change**. The checkbox is the map. Connect host is *which radio*.

**Push live settings over 8888** — do not write `remote-phones.ini` for the normal path.

- Keep **`0x9B`** as mode 0 / 1 / 2 / 3 (small integer; cannot carry an IP).
- Add **two 4-byte opcodes** on the existing `SDRcore_recv_send_param` hop (client → ms-sdr → recv). Assign unused bytes at implement time (must be free on ms-sdr, recv, and trans).

| New opcode (name) | Payload | Recv does |
|-------------------|---------|-----------|
| **CMD_SET_REMOTE_RX_HOST** | IPv4 as `uint32` | MSA1 destination |
| **CMD_SET_REMOTE_RX_CTRL** | port + flags | **enable**, **monitor-at-radio**, phones vs digital |

Remote **on:** host = client’s own IPv4 (local address of the 8888 socket — correct NIC if multi-homed), port **9100**, enable=1, monitor=0 unless checked, then `0x9B` = 2 or 3.  
Remote **off:** enable=0, then `0x9B` = 0 or 1. Recv stops UDP and restores local phones (unless monitor).

Trans does **not** need HOST. Mic TX is already “whoever sends to **9101**.” Opcode 2/3 only selects that ring vs local/VAC.

`remote-phones.ini` remains a **fallback** if the client never sent a host.

**AF UI** is an owned popup (same pattern as LOG and S/W): session position, reopen activates. Bind `Ui*` chrome (LOG/S/W still hardcode dark — Remote should follow the user scheme). Closing the popup does **not** turn Remote off; the checkbox does. TX host = Connect host (no second IP box).

**Trees:** `rpi/` is the Pi source of truth (do not edit for Ubuntu). Ubuntu work is **`linux/`**. Same opcodes later in Windows `ms-sdr-MKII` / `SDRcore-recv` / `SDRcore-trans` if a Windows box is the radio host.

---

## Punch list

### 1. Mute local phones when remote RX is on

**Problem:** Radio speaker keeps playing operator AF while UDP RX is active.

**Want:** Mute **operator phones only** (not I/Q, not VirtualA/B).

| Item | Notes |
|------|--------|
| Recv: when remote RX stream is active, do not play operator AF locally | Today `REMOTE_AUDIO` uses `manage_stream` like Phones |
| Default: mute local phones | Unattended radio / remote op |
| **Monitor at radio** (in the Remote popup) | Default **off**. On = keep local phones for a second person in the shack |
| Remote RX off | Restore previous local phones behavior |
| Confirm trans I/Q and digi sinks unchanged | Regression check |
| Can ship with INI RX still | Then hang the same mute on enable=1 from (2) |

### 2. Client drives RX enable + destination

**Problem:** RX is INI-only (`ENABLED` / `HOST`). Restart servers after edit. Linux-to-Linux cannot turn it on from the operator seat.

**Want:** Remote checkbox **on** means:

1. Send **CMD_SET_REMOTE_RX_HOST** + **CMD_SET_REMOTE_RX_CTRL** (enable, port, monitor)
2. Send `0x9B` = 2 (R-Phones) or 3 (R-Digital)
3. Checkbox **off** → enable=0, unmute local phones (unless monitor), `0x9B` = 0 or 1

| Item | Notes |
|------|--------|
| Recv starts/stops MSA1 live — no process restart | |
| Client fills HOST from the 8888 socket’s local IPv4 | No typing IP on the radio |
| Init GUI may **show** current HOST/ENABLED | Fallback / debug, not the operator workflow |
| Recv still honors INI if the client never sent a destination | Backward compatible |
| Trans unchanged for HOST | Still listens **9101** |

### 3. Remote AF popup inside WPF and Avalonia

**Problem:** Extra Windows exe, launcher paths, two windows, no Linux player.

**Want:** Same popup in **MSCC.Wpf** and **MSCC.Avalonia**.

| Item | Notes |
|------|--------|
| Shared wire: MSA1 RX 9100, TX 9101, jitter, EQ | **MSCC.Core** (or small shared AF lib) — not copied twice |
| Windows playback/capture | WASAPI (port from MsccRemotePhones / NAudio) |
| Linux playback/capture | Pulse/PipeWire (or PortAudio) — this **is** the Linux remote-audio GUI |
| Popup: devices, volume, mute, EQ (phones only), monitor-at-radio, status | LOG/S/W owned-window pattern; `Ui*` brushes |
| TX host = Connect host | No second IP box |
| Open when Remote checks on; close AF when Remote checks off | X on the window leaves Remote on |
| Stop launching `MsccRemotePhones.exe` once in-UI AF works | See companion note below |
| Linux-to-Linux smoke | Avalonia operator ↔ Ubuntu or Pi radio host |

### 4. Remote Digital + CAT proxy

**Problem:** Digi apps (WSJT-X) on the operator PC cannot use phones AF. CAT today lives **inside ms-sdr** on the radio host (Windows COM / Linux `$HOME/ms-sdr-cat`). It does not follow Connect.

**Want:** Remote + Digital = **R-Digital** (`0x9B` = 3).

| Item | Notes |
|------|--------|
| Recv taps **digital** AF (VirtualA path), not phones AF | Different filters/AGC/levels |
| Trans digital mic from MSA1, not VirtualB | Mute radio VirtualA/B while R-Digital is on (same idea as item 1) |
| Client plays/captures **the same** `digital-speaker.ini` / `digital-microphone.ini` VAC | WSJT-X devices never change |
| Fixed jitter buffer (~80–150 ms) | Do not let the buffer hunt (FT8) |
| **CAT proxy** on the operator PC | Local Kenwood COM/TCP → Connected ms-sdr. Same WSJT-X CAT settings local or remote |
| Local Digital + Remote **off** + Connect to Ubuntu | Digi app still on the radio (today). R-Digital is the “app on this PC” choice |

---

## Companion exe (Windows fallback only)

`MsccRemotePhones.exe` is operator-side, not required because the radio host is Windows.

- Keep until the **WPF** popup ships; restyle to client chrome if we touch it.
- After in-UI AF works, drop it from the day-to-day path.
- Linux never uses it (Avalonia popup).

---

## Out of scope (still later)

- Remote Digital over WAN / multi-operator mix
- Embedding into firmware / STM32
- Init GUI as the operator workflow (debug display only)

---

## Suggested order

1. **Linux servers (`linux/`)** — item 1 mute + monitor (INI RX still OK). Ubuntu laptop build. Do not edit `rpi/` for this.  
2. **Linux servers (`linux/`)** — item 2 host/enable opcodes + ms-sdr forward. Smoke with existing MsccRemotePhones or a tiny UDP listener.  
3. **WPF** — Remote checkbox as master (stop 3-way Audio cycle); popup; stop requiring the exe.  
4. **Avalonia** on the Ubuntu laptop — same checkbox + popup (Linux GUI).  
5. **Item 4** — opcode 3, digital tap, VAC, CAT proxy.  
6. Port proven `linux/` recv/ms-sdr bits to **`rpi/`** when shipping a Pi `.deb`. Same opcodes in Windows servers if needed.  
7. Retire MsccRemotePhones from daily use.

Field note (2026-09-10): Windows WPF + Ubuntu `192.168.1.234` worked with `HOST=` Windows client IP, MsccRemotePhones on 9100/9101, opcode 2 for mic.
