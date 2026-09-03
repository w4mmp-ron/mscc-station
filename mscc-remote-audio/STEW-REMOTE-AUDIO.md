# Remote Audio — handoff for Stew (Windows MSCC client)

**From:** Ron  
**To:** Stew (Windows MSCC / WPF client)  
**Date:** 2026-08-24  
**Also see:** `SDRcore-trans-linux/STEW-REMOTE-AUDIO.md` (server / opcode notes)

---

## Goal

Add a **Remote Audio** checkbox so the operator can use network operator mic (SSB/Phones) without editing Pi INI files.

**Digital stays local on the Pi** (digi apps / VirtualA–B). Remote is for standard SSB voice only (for now).

---

## Client UI

| Control | Behavior |
|---------|----------|
| **Remote Audio** checkbox | Recommended label (not “Server Audio”) |
| **Phones** selected + checkbox **on** | Send `CMD_SET_AUDIO_DEVICE` = **2** |
| **Phones** selected + checkbox **off** | Send **1** (local operator mic) — as today |
| **Digital** selected | Always send **0** — ignore / grey the checkbox if you want |

Tooltip idea: *When checked with Phones selected, operator mic comes from the remote PC (MsccRemotePhones). Digital stays on the server.*

---

## Opcode (existing)

| Item | Value |
|------|--------|
| Opcode | `CMD_SET_AUDIO_DEVICE` = **`0x9B`** |
| Payload | one data byte (client already sends a single byte; ms-sdr reads low byte) |

| Data | Meaning (servers) |
|------|-------------------|
| **0** | Digital — local digi mic/speaker |
| **1** | Phones — local operator mic |
| **2** | Remote — MSA1 UDP mic into `sdrcore-trans` |

Add something like:

```csharp
public const byte REMOTE_SOUND_DEVICE = 2;  // or REMOTE_AUDIO = 2
```

Update `SetAudioDeviceAsync` / D↔P UI so Phones + Remote Audio → **2**.

---

## What the appliance already does

All three servers accept **2**:

| Server | On data **2** |
|--------|----------------|
| **ms-sdr** | Forwards to recv + trans; applies **Phones** mic/speaker levels |
| **sdrcore-trans** | Operator TX mic from MSA1 UDP (default port **9101**) |
| **sdrcore-recv** | Same speaker path as Phones (**1**); does not special-case remote RX |

Remote **phones RX** (Pi → Windows) is separate: Pi `remote-phones.ini` + MsccRemotePhones listen **9100**. Opcode **2** does not gate RX; UDP can keep flowing if nothing is listening.

Pi seed (port only): `~/.local/mscc/remote-mic.ini` → `PORT=9101` (`ENABLED` ignored).

---

## Companion app (already in this tree)

**MsccRemotePhones** — Windows operator AF:

- **RX:** UDP **9100** (from Pi `sdrcore-recv` when remote-phones enabled)
- **TX:** UDP **9101** → Pi `sdrcore-trans` (used when client sends **2**)

Set **TX host** to the Pi hostname/IP (not `127.0.0.1` on the PC).

---

## Test without the checkbox (for now)

With MSCC client connected (live session), from this folder:

```powershell
.\Set-AudioDevice.ps1 -HostName proficio -Mode remote   # 2
.\Set-AudioDevice.ps1 -HostName proficio -Mode phones   # 1
.\Set-AudioDevice.ps1 -HostName proficio -Mode digital  # 0
```

No GUI handshake — does not steal the client session. No ms-sdr changes required.

Pi log (trans): `CMD_SET_AUDIO_DEVICE REMOTE done` and, with MsccRemotePhones TX running, `remote_mic: pkt ok=…`.

---

## Out of scope (later)

- Remote Digital / digi over MSA1  
- Driving recv remote-phones on/off from opcode **2**  
- Embedding MsccRemotePhones inside the WPF client  

---

## Checklist for Stew

1. [x] Constant `REMOTE_SOUND_DEVICE = 2` (or equivalent) next to `DIGITAL` / `PHONES`  
2. [x] **Remote Audio** checkbox on UI (WPF left rail + Avalonia left rail)  
3. [x] Phones + checked → send **2**; Phones + unchecked → **1**; Digital → always **0**  
4. [x] Optional: disable/grey checkbox while Digital is selected  
5. [ ] Confirm Phones mic gain still applies when mode is **2** (server uses Phones levels) — field test  

Sticky: WPF `REMOTE_AUDIO` in `MSCC_Client.ini`; Avalonia `REMOTE_AUDIO` in `mscc-avalonia.ini`.
