# Remote audio — punch list

**Date:** 2026-09-11  
**Status:** Ubuntu `linux/` servers (items 1–2) + WPF in-UI Remote Phones (item 3, Windows) field-proven tonight. Stopped here.  
**Working rule:** implement and prove everything on **`linux/`** (Ubuntu radio). Do **not** edit **`rpi/`** until that is working; then reconcile Pi from the proven `linux/` bits.

Related: [STEW-REMOTE-AUDIO.md](STEW-REMOTE-AUDIO.md), [README.md](README.md).

---

## Left off (2026-09-11, end of day)

Resume here. Radio: Ubuntu laptop **`linux/`** servers (`10.42.0.1`). Client: Windows WPF **9.11.2** in `C:\mscc-net9` (Connect-only, Launch Servers off).

### Done tonight

| Piece | State |
|-------|--------|
| **`linux/` recv + ms-sdr** | Live `0x25` HOST + `0x28` CTRL (enable/monitor/port). Mute local phones when remote RX on and monitor=0. INI still fallback. Trans still opcode **2** mic on **9101**. |
| **WPF rail** | Audio button **Phones ↔ Digital** only. **Remote** checkbox is the operator-seat master (not greyed on Digital). Sticky: `REMOTE_AUDIO` + local `AUDIO_DEVICE_MODE` 0/1 only — never persist 2/3 as radio boot. |
| **WPF wire** | Remote on → HOST (client IPv4 on route to Connect) then CTRL enable=1 port 9100 then `0x9B`=**2**. Remote off / Stop → enable=0 then local 0/1. |
| **WPF popup** | `RemoteAfWindow` — devices, phones volume, mute, EQ, mic, **Monitor at radio**. TX host = Connect IP (no extra IP box). Volume works. Does **not** launch `MsccRemotePhones.exe`. |
| **Process exit** | Close main window shuts down leftover AF windows (`ShutdownMode=OnMainWindowClose`). Task Manager leftover **fixed**. |

Field log (Windows `10.42.0.157` ↔ Ubuntu `10.42.0.1`): `0x25` payload `0A-2A-00-9D`, `0x28` enable=1 port 9100, `0x9B`=2, MSA1 play 48 kHz, mic TX `10.42.0.1:9101`. Popup first failed (`NullReferenceException` on slider `ValueChanged` during XAML init — volume label not created yet); fixed in **9.11.2**.

### Not done (next session)

1. **Avalonia** same checkbox + popup (Linux operator GUI).  
2. **Item 4** — `0x9B`=**3** R-Digital (recv digital AF tap, trans digital mic, client VAC, CAT proxy). Tonight **R-Digital is label only**; wire is still opcode **2**.  
3. **`rpi/`** — do not copy until Ubuntu path stays solid.  
4. Retire `MsccRemotePhones.exe` from the tree when Avalonia is in; WPF no longer needs it day-to-day.  
5. Popup chrome: copies owner brushes on open; not live-synced if Settings theme changes while open.

### Files touched (this WPF pass)

- `mscc-ui/windows-work-tree/.../MSCC.Core` — `MsccAudioProtocol.cs`, opcodes `0x25`/`0x28`/`REMOTE_DIGITAL=3`, `SetRemoteRxAsync` / `GetLocalIPv4ToRemote`  
- `mscc-ui/windows-work-tree/.../MSCC.Wpf` — `RemoteAudio/*`, `RemoteAfWindow.xaml`, rail checkbox, shutdown  
- `mscc-remote-audio/REMOTE-AUDIO-PUNCHLIST.md` (this file)

---

## What already works

| Path | How |
|------|-----|
| Control | WPF Connect → Ubuntu radio UDP **8888** |
| Operator **mic TX** | Remote on → `0x9B` **2**. WPF popup captures mic → MSA1 **9101** (Connect host). |
| Operator **phones RX** | Remote on → client sends `0x25`/`0x28`; recv streams MSA1 **9100** to this PC. Popup plays it. INI `remote-phones.ini` still works if the client never sent a host. |
| Local Digital / Phones | Remote **off** → `0x9B` 0 or 1 on the radio. Headless WSJT on Ubuntu unchanged. |
| Mute shack phones | Recv mutes operator speaker while remote RX is on unless **Monitor at radio**. |

**Windows:** in-UI popup in MSCC.Wpf (9.11.2). `MsccRemotePhones.exe` is leftover / fallback only.  
**Linux operator:** still no Avalonia player.

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
| Recv: when remote RX stream is active, do not play operator AF locally | **Done on `linux/`** (live CTRL + INI) |
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
| Recv starts/stops MSA1 live — no process restart | **Done on `linux/` + WPF sends HOST/CTRL** |
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
| Open when Remote checks on; close AF when Remote checks off | **WPF done (9.11.2).** X leaves Remote on. Avalonia not started. |
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

- WPF day-to-day path no longer launches it (in-UI popup). Keep the project as a reference / fallback until Avalonia ships.
- Linux never uses it (Avalonia popup, not started).

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

1. ~~**`linux/` recv** — item 1 mute + monitor.~~ **Done.**  
2. ~~**`linux/` recv + ms-sdr** — item 2 `0x25`/`0x28`.~~ **Done.**  
3. ~~**WPF Remote Phones popup + checkbox.**~~ **Done (9.11.2).** Next: **Avalonia** same UI. R-Digital label still sends opcode **2**.  
4. **Item 4 on `linux/`** — opcode 3, digital tap, VAC, CAT proxy. Keep Ron’s headless Digital (UI closed).  
5. **Reconcile `rpi/`** — copy proven `linux/` bits, Pi `.deb`. Not before this Ubuntu path stays solid.  
6. Retire `MsccRemotePhones.exe` from the tree after Avalonia.

Field notes:  
- 2026-09-10: WPF + Ubuntu with INI `HOST=` + MsccRemotePhones, opcode 2 mic.  
- 2026-09-11: WPF 9.11.2 in-UI popup + live `0x25`/`0x28`; Windows `10.42.0.157` ↔ Ubuntu `10.42.0.1`.
