# Remote audio — punch list

**Date:** 2026-09-12  
**Status:** R-Phones proven. **`linux/` `0x9B`=3 is on the wire** (recv/trans/ms-sdr). Next: WPF send 3 + VAC. CAT later. `rpi/` still read-only.  
**Working rule:** implement and prove everything on **`linux/`** (Ubuntu radio). Do **not** edit **`rpi/`** until that is working; then reconcile Pi from the proven `linux/` bits.

Related: [STEW-REMOTE-AUDIO.md](STEW-REMOTE-AUDIO.md), [README.md](README.md).

---

## Left off (2026-09-12)

**Ubuntu `linux/` opcode 3 is implemented.** WPF still sends **2** until it is switched to 3. Smoke: `set-remote-rx.py --enable --dest <client> --audio 3`. Do **not** edit `rpi/`.

Radio: Ubuntu **`linux/`** (`10.42.0.1`). Client: Windows WPF **9.11.2** (Connect-only). R-Phones is good.

### Linux opcode 3 — implementer spec

Today recv/trans only switch on 0 / 1 / 2. **3 is ignored.** `0x25`/`0x28` stay as-is (HOST/CTRL). 2 vs 3 is only which AF class.

| `0x9B` | Recv speaker | Recv MSA1 tap | Mute on radio | Trans mic |
|--------|--------------|---------------|---------------|-----------|
| 0 Digital | VirtualA | off unless CTRL enable (should not) | — | VirtualB |
| 1 Phones | operator | off unless CTRL | — | operator |
| 2 R-Phones | operator (like 1) | post-DSP AF → 9100 | operator phones (unless monitor) | MSA1 9101 → operator mic ring |
| **3 R-Digital** | **do not play VirtualA** | **same MSA1 9100**, but recv is in **Digital** DSP/levels (DIG-U/L, digital volume) | **VirtualA** (not operator phones) | **MSA1 9101 → digital mic ring** (not VirtualB) |

Same UDP ports as R-Phones. Client will later play/capture VAC; **do not** require VAC on the radio.

**Recv (`linux/SDRcore-recv-linux`)**

- `commands.h`: `#define REMOTE_DIGITAL_AUDIO 3`
- `udp_thread.c` `CMD_SET_AUDIO_DEVICE`: case **3** — close operator **and** VirtualA play streams (or open a dummy/IQ-only if the callback must keep running). DSP callback must still run so `remote_phones_feed` keeps sending. Mute **VirtualA** while CTRL enable=1; leave operator phones alone (R-Digital is not a voice seat).
- Keep feeding MSA1 from the current post-DSP `outcplx` when `remote_phones_enabled()` — that AF is whatever mode/filters the radio is in. Client will be in DIG-U when testing WSJT.
- `0x25`/`0x28` unchanged.

**Trans (`linux/SDRcore-trans-linux`)**

- `commands.h`: `REMOTE_DIGITAL_AUDIO 3`
- `udp_thread.c` case **3**: `G_audio_mode = 3`. **Do not** open VirtualB capture (same idea as DIGITAL+TUNE/CW: I/Q output-only, or operator I/Q out + no digi capture). Callbacks pull `remote_mic_fill_stereo_96k` when mode is **2 or 3**.
- `main.c`: today `remote_mic_fill` only if `G_audio_mode == REMOTE_AUDIO` (2). Extend to **2 || 3**.
- `remote_mic` already listens 9101 always — no HOST. Log “use when AUDIO_DEVICE=2 or 3”.

**ms-sdr (`linux/ms-sdr-linux`)**

- Already forwards any `0x9B` byte to recv **and** trans — 3 will arrive once cores handle it.
- `user_controls.c`: **3 uses Digital levels** (mic/speaker), not Phones. Today only `== 0` is Digital; `else` is Phones including 2. Treat **0 and 3** as Digital levels; **1 and 2** as Phones.
- **Do not** write 2 or 3 into `user_controls.ini` `AUDIO_DEVICE` (boot stays 0/1).

**Smoke (before WPF changes)**

- WPF R-Phones (2) still works (regression).
- From Ubuntu logs: `CMD_SET_AUDIO_DEVICE` **3** on recv and trans (can inject with a tiny UDP send of `0x9B` data=3 after Connect, or wait for WPF).
- Recv log: HOST/CTRL still start MSA1; VirtualA not playing.
- Trans log: REMOTE DIGITAL, `remote_mic` ready, no VirtualB open.

**After linux 3 is in the logs:** WPF sends 3 when Remote+Digital, popup uses `digital-speaker.ini` / `digital-microphone.ini`, no EQ. **CAT proxy later.** Then Windows servers get the same 3 when swapping client/server.

### Already done (keep)

WPF 9.11.2 popup, checkbox, `0x25`/`0x28`, opcode 2, process exit. Field: Windows `10.42.0.157` ↔ Ubuntu `10.42.0.1`.

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
| Recv + trans + ms-sdr handle **`0x9B` = 3** | **Next (Ubuntu `linux/`).** Spec in “Left off”. `rpi/` later. |
| Recv: Digital seat + MSA1; mute VirtualA | Do not play shack VirtualA. Operator phones not the mute target. |
| Trans: MSA1 9101 into digital TX path | No VirtualB capture while 3. `remote_mic_fill` for mode 2 **and** 3. |
| ms-sdr: Digital **levels** for 3 | Phones levels stay on 1/2. Never persist 3 in `user_controls.ini`. |
| WPF: send 3 + VAC devices | **After** linux 3 is in recv/trans logs. No EQ. |
| **CAT proxy** on the operator PC | Later. Audio 3 first. |
| Local Digital + Remote **off** | **Keep:** WSJT + CAT on the radio host. |
| Disconnect / Remote off | enable=0, `0x9B` = last local 0/1. |

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
3. ~~**WPF Remote Phones popup + checkbox.**~~ **Done (9.11.2).**  
4. ~~**`linux/` opcode 3**~~ **Done** (recv VirtualA tap + mute, trans MSA1 digital mic, ms-sdr digital levels, no persist 2/3). Next: WPF send 3 + VAC. CAT later.  
5. **Windows servers** same 3 when swapping client/server. **Avalonia** popup.  
6. **Reconcile `rpi/`** after Ubuntu 2 **and** 3 are solid.  
7. Retire `MsccRemotePhones.exe` from the tree after Avalonia.

Field notes:  
- 2026-09-10: WPF + Ubuntu with INI `HOST=` + MsccRemotePhones, opcode 2 mic.  
- 2026-09-11: WPF 9.11.2 in-UI popup + live `0x25`/`0x28`; Windows `10.42.0.157` ↔ Ubuntu `10.42.0.1`.
