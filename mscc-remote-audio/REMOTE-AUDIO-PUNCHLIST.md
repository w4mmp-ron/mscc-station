# Remote audio — punch list

**Date:** 2026-09-10  
**Status:** field-proven Windows client ↔ Ubuntu servers (INI RX + opcode 2 mic).  
**Working rule:** implement and prove everything on **`linux/`** (this Ubuntu box). Do **not** edit **`rpi/`** until that is working; then reconcile Pi from the proven `linux/` bits.

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

Radio host stays the appliance. Operator AF + CAT live on the **operate PC** **only when Remote is on**. With Remote **off** (or no UI at all), AF + CAT stay **on the radio host** — that is Ron’s Pi + WSJT-X path and it must keep working.

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

**Two seats for WSJT-X / CAT — keep both:**

| Seat | How | Typical |
|------|-----|---------|
| **Radio host** (Pi/Ubuntu) | Remote **off**, Audio **Digital** (`0x9B` = 0). VirtualA/B + CAT (`$HOME/ms-sdr-cat` or tty0tty). **No UI required.** | Ron: small monitor, only WSJT-X; servers already running |
| **Operate PC** | Remote **on**, Audio **R-Digital** (`0x9B` = 3). VAC + CAT **proxy** on this PC. | Stew: WSJT on the laptop, radio in the other room |

The checkbox is which seat is live — not “WSJT settings never change across machines.” Each seat keeps **its own** WSJT device/CAT config. Connect host is *which radio*.

**Headless (keep):** ms-sdr already does `User_Controls_Apply_To_Cores()` — pushes `AUDIO_DEVICE` from `user_controls.ini` to recv/trans with **no client**. CAT (PTY / `/dev/tnt0`) is independent of the GUI. Workflow: open UI (or Init), set **Digital**, close UI; next `mscc start` still comes up Digital. Do **not** require the operate UI to stay open for local digi.

**Must not break that:**

- Closing the UI, or Connect with Remote **off**, must **not** switch AF off Digital, mute VirtualA/B, or move CAT onto a proxy.
- Do **not** persist `0x9B` = 2 or 3 as the radio’s boot mode. `user_controls.ini` `AUDIO_DEVICE` stays **0 or 1** (local Digital/Phones). Remote is a **client** sticky.
- If a Remote client **disconnects** (or Remote goes off), recv enable=0 and restore last **local** 0/1 so the Pi can keep WSJT without a GUI.
- A spectrum-only Connect (Remote off) is a spectator; it must not steal the shack’s Digital/CAT.

**Push live settings over 8888** — do not write `remote-phones.ini` for the normal path.

- Keep **`0x9B`** as mode 0 / 1 / 2 / 3 (small integer; cannot carry an IP).
- Add **two 4-byte opcodes** on the existing `SDRcore_recv_send_param` hop (client → ms-sdr → recv). Assign unused bytes at implement time (must be free on ms-sdr, recv, and trans).

| New opcode (name) | Byte | Payload | Recv does |
|-------------------|------|---------|-----------|
| **CMD_SET_REMOTE_RX_HOST** | **`0x25`** | IPv4, 4 bytes **network order** | MSA1 destination |
| **CMD_SET_REMOTE_RX_CTRL** | **`0x28`** | uint32 LE: port[15:0] \| enable<<16 \| monitor<<17 | start/stop stream + mute |

Do **not** put phones vs digital in CTRL (`0x9B` 2 vs 3 is the AF class). Send **HOST then CTRL**. Smoke: `mscc-remote-audio/set-remote-rx.py`. Not `0x0E` (Solidus).

Remote **on:** host = client’s own IPv4 (route to Connect host, not `0.0.0.0` from an unconnected socket), port **9100**, enable=1, monitor=0 unless checked, then `0x9B` = 2 or 3.  
Remote **off:** enable=0, then `0x9B` = 0 or 1. Recv stops UDP and restores local phones (unless monitor).

**`linux/` live opcodes (this pass):** recv + ms-sdr forward `0x25`/`0x28` without restart. INI still used if the client never sent a host. Trans does not handle these.

Trans does **not** need HOST. Mic TX is already “whoever sends to **9101**.” Opcode 2/3 only selects that ring vs local/VAC.

`remote-phones.ini` remains a **fallback** if the client never sent a host.

**AF UI** is an owned popup (same pattern as LOG and S/W): session position, reopen activates. Bind `Ui*` chrome (LOG/S/W still hardcode dark — Remote should follow the user scheme). Closing the popup does **not** turn Remote off; the checkbox does. TX host = Connect host (no second IP box).

**Trees:** all server work for this plan is **`linux/`** (`SDRcore-recv-linux`, `SDRcore-trans-linux`, `ms-sdr-linux`). Treat **`rpi/`** as read-only until Ubuntu remote AF is proven, then port/reconcile. Windows server trees later only if a Windows box is the radio host. Clients (WPF / Avalonia) talk to this Ubuntu radio; they are not `rpi/` work.

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
| **`linux/` INI mute (this pass)** | Recv zeros operator phones when `ENABLED=1` and `MONITOR=0`. VirtualA not muted. `MONITOR=1` keeps shack speaker. Restart recv after INI change. |

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
| **CAT proxy** on the operator PC | Only while Remote is **on**. Local Kenwood COM/TCP → Connected ms-sdr |
| Local Digital + Remote **off** (UI closed or never opened) | **Keep:** WSJT-X + CAT on the radio host. This is Ron’s Pi desktop. Not deprecated by R-Digital |
| Disconnect / Remote off | Drop proxy; CAT and VirtualA/B stay on the radio as last local Digital/Phones |

---

## Solidus (later — do not paint into a corner)

Resurrected standalone rig: **RPi in the radio**, front **~5″ display**, early MSCC. Local appliance **and** optional remote computer. Tree: [`Solidus/`](../Solidus/) (`client/Avalonia`, `servers/`, `Proficio/`). Avalonia is heading there. **Not this sprint.**

That is the same two-seat model as Ron’s Pi + WSJT:

| On the box | Remote computer |
|------------|-----------------|
| Servers + CAT + Digital/Phones | Connect 8888; Remote on = operator AF/CAT here |
| Small screen: WSJT-X only, or Avalonia as the front panel | WPF or Avalonia as a client |

ms-sdr already has **appliance** hooks: `Apply_Appliance_Startup()`, `G_Transceiver_type == SOLIDUS`, temp/GPIO/fan, **`CMD_SET_SOLIDUS_STATUS` (`0x0E`)**. Headless cores without a GUI is the Solidus boot path.

**Do not break / extra work later:**

- Keep **headless Digital + CAT** with UI closed (5″ can show only WSJT-X).
- **Do not reuse opcode `0x0E`** (or other Solidus/extended I2C bytes) for remote RX. New RX opcodes must be unused on ms-sdr, recv, **and** trans.
- Client Connect must **not** steal AF/CAT unless Remote is on (front panel or shack WSJT still owns the seat).
- Boot/`user_controls.ini` stays local **0/1**, never 2/3 — the radio comes up as an appliance.
- Disconnect Remote → restore local 0/1 (someone can walk up to the box).
- Avalonia on the 5″ is the **front panel**, not MsccRemotePhones. In-UI AF belongs in Avalonia; keep the Remote popup **optional** (owned window) so a dense 800×480 face is still possible. Today’s shell is `MinWidth="1024"` — Solidus will need a compact layout later; do not assume a large AF window.
- Solidus hardware (MCP23017, TX relay, fan, temp) stays independent of MSA1 remote AF.

---

## Companion exe (Windows fallback only)

`MsccRemotePhones.exe` is operator-side, not required because the radio host is Windows.

- Keep until the **WPF** popup ships; restyle to client chrome if we touch it.
- After in-UI AF works, drop it from the day-to-day path.
- Linux never uses it (Avalonia popup).

---

## Out of scope (still later)

- Remote Digital over WAN / multi-operator mix
- Requiring the operate UI to stay open for shack WSJT-X (headless Digital must remain)
- Embedding into firmware / STM32
- Init GUI as the operator workflow (debug display only)
- Solidus 5″ compact Avalonia layout / kiosk (use the same opcodes; different chrome later)

---

## Suggested order

Prove on **this Ubuntu radio (`linux/`)** first. **`rpi/` is last**, after it works here.

1. **`linux/` recv** — item 1 mute + monitor (INI RX still OK).  
2. **`linux/` recv + ms-sdr** — item 2 host/enable opcodes + 32-bit forward. Smoke with current WPF + MsccRemotePhones.  
3. **WPF and/or Avalonia** against this Ubuntu host — Remote checkbox as master; AF popup; Linux GUI = Avalonia.  
4. **Item 4 on `linux/`** — opcode 3, digital tap, VAC, CAT proxy. Keep Ron’s headless Pi-style Digital (UI closed) on this box too.  
5. **Reconcile `rpi/`** — copy proven recv/ms-sdr/trans bits, same opcodes, Pi `.deb`. Not before step 2 (at least) is solid.  
6. Retire MsccRemotePhones from daily use.

Field note (2026-09-10): Windows WPF + Ubuntu `192.168.1.234` worked with `HOST=` Windows client IP, MsccRemotePhones on 9100/9101, opcode 2 for mic.
