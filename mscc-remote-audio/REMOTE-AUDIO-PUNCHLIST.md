# Remote audio — punch list

**Date:** 2026-09-10  
**Status:** field-proven Windows client ↔ Linux/Pi servers. UI/Linux-to-Linux still rough.  
**No code in this pass** — catch up later from this plan.

Related: [STEW-REMOTE-AUDIO.md](STEW-REMOTE-AUDIO.md), [README.md](README.md).

---

## What already works

| Path | How |
|------|-----|
| Control | WPF (or Avalonia) Connect → radio host UDP **8888** |
| Operator **mic TX** | Phones + **Remote Audio** → `CMD_SET_AUDIO_DEVICE` **2** (`0x9B`). MsccRemotePhones TX host = radio IP, port **9101** |
| Operator **phones RX** | Radio `~/.local/mscc/remote-phones.ini`: `ENABLED=1`, `HOST=` **client IPv4**, `PORT=9100`. Recv streams MSA1. Restart servers after INI change. MsccRemotePhones listen **9100** |
| Digital | Stays on the radio host (VirtualA/B). Not on this path |

**Windows companion:** `mscc-remote-audio/MsccRemotePhones` → copy `MsccRemotePhones.exe` next to WPF (`C:\mscc-net9`).  
**Linux operator:** no player GUI. Avalonia skips launching the WinForms exe.

Opcode **2** does **not** start/stop the RX stream and does **not** mute the radio’s local phones.

---

## Direction (agreed)

Radio host stays the appliance. Operator AF lives **in the operate UI** (WPF **and** Avalonia), not a second process.

- One Connect, one **Remote Audio** control, devices in Settings / a **Remote** tab.
- Protocol (MSA1, jitter, EQ, ports) once in **MSCC.Core** (or a small shared AF library).
- Playback/capture stays native: WASAPI on Windows, Pulse/PipeWire (or PortAudio) on Linux.
- Client tells the radio “send phones to **me**” (its own IPv4 + 9100). Do not make the operator type HOST on the radio for the live path.
- `remote-phones.ini` remains a **fallback** only.

MsccRemotePhones stays as a prototype/reference until the in-UI panel ships; then it can retire.

---

## Punch list

### 1. Mute local phones when remote RX is on

**Problem:** Radio speaker keeps playing operator AF while UDP RX is active.

**Want:** Mute **operator phones only** (not I/Q, not VirtualA/B). Digital stays local.

| Item | Notes |
|------|--------|
| Recv: when remote RX stream is active, do not play operator AF locally | Same path as Phones today (`REMOTE_AUDIO` uses `manage_stream` like operator) |
| Default: mute local phones | Unattended radio / remote op |
| **Monitor at radio** checkbox | Default **off**. On = keep local phones for a second person in the shack |
| Remote RX off | Restore previous local phones behavior |
| Confirm trans I/Q and digi sinks unchanged | Regression check |

### 2. Client drives RX enable + destination (Linux GUI / any client)

**Problem:** RX is INI-only on the radio (`ENABLED` / `HOST`). No Init or MSCC screen. Linux-to-Linux has no way to turn it on from the operator seat.

**Want:** **Remote Audio On** from the client means:

1. Opcode **2** (mic, as today)
2. Enable phones UDP to **this client’s IPv4:9100**
3. **Remote Audio Off** stops the stream and unmutes local phones (unless monitor-at-radio)

| Item | Notes |
|------|--------|
| New opcode or extend **2** so recv starts/stops MSA1 TX to a host | Prefer explicit enable + host/port so INI is not required for live use |
| Client fills HOST from its own LAN IPv4 | No typing IP on the radio for the normal path |
| Init GUI may **show** current HOST/ENABLED | Fallback / debug, not the operator workflow |
| Recv still honors INI if the client never sent a destination | Backward compatible |
| Linux-to-Linux: Avalonia on the operator box is enough once (3) plays/captures AF | No WinForms helper |

### 3. Remote AF panel inside WPF and Avalonia

**Problem:** Extra Windows exe, launcher paths, two windows, no Linux player.

**Want:** Same **Remote** panel in **MSCC.Wpf** and **MSCC.Avalonia**.

| Item | Notes |
|------|--------|
| Shared wire: MSA1 RX 9100, TX 9101, jitter, EQ | MSCC.Core or shared AF lib — not copied twice |
| Windows playback/capture | WASAPI (port from MsccRemotePhones / NAudio) |
| Linux playback/capture | Pulse/PipeWire (or PortAudio) — new |
| UI: listen + mic devices, volume, mute, EQ, TX host (radio IP = Connect host) | Settings or **Remote** tab |
| Remote Audio on/off + monitor-at-radio on the main operate UI | Phones selected; grey on Digital |
| Stop companion launch of `MsccRemotePhones.exe` once in-UI AF works | Keep exe in tree until then |
| Linux-to-Linux smoke | Avalonia operator ↔ Pi or Ubuntu radio host |

---

## Out of scope (still later)

- Remote Digital / digi over MSA1
- Embedding into firmware / STM32
- Multi-operator mix (more than one remote AF client)

---

## Suggested order when we pick this up

1. Recv mute-local-phones + monitor flag (can ship with INI RX still).  
2. Client → recv “stream to me” (fixes Linux enable without a radio-side GUI).  
3. In-UI AF player/sender (WPF first or Avalonia-first — both must match).  
4. Drop MsccRemotePhones from the day-to-day path.

Field note (2026-09-10): Windows WPF + Ubuntu `192.168.1.234` worked with `HOST=` Windows client IP, MsccRemotePhones on 9100/9101, opcode 2 for mic.
