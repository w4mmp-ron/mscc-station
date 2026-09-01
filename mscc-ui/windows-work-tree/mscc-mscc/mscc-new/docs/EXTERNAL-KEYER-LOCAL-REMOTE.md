# External electronic keyer / legacy CW — local & remote setups

## What the flag does

| `mscc.ini` on the **radio PC** | Meaning |
|--------------------------------|---------|
| **`PROFICIO-MKII=1`** (default if missing) | Proficio **MKII** internal PIC keyer: full keyer USB config, rear PTT sense thread |
| **`PROFICIO-MKII=0`** | **Legacy** / external electronic keyer: no PIC keyer USB train (except **HOLD**), no rear PTT sense thread |

ms-sdr reads this **once at process start**. Changing the file while ms-sdr is running does nothing until backends restart.

### Two layers (do not confuse them)

| Layer | Where | Purpose |
|-------|--------|---------|
| **Host** `PROFICIO-MKII` in radio PC `mscc.ini` | Windows: `%LocalAppData%\MSCC-NET9\mscc.ini` · Linux: `~/mscc.ini` | **Source of truth for the radio** |
| **Client** sticky checkbox “Use external electronic keyer (legacy)” | Windows: `MSCC_Client.ini` `EXTERNAL_ELECTRONIC_KEYER` · Avalonia: client settings | **UI only**: grays PIC keyer controls; HOLD stays live. Does **not** change a remote host’s file |

The Windows/Avalonia UI does **not** read host mode over UDP. If the host is already legacy and the client checkbox is unchecked, the radio still works in legacy mode; the UI may still show full keyer controls until you check the box for matching gray-out.

---

## Local setup (majority of users)

**Same PC:** Launch Servers **ON**, backend IP **127.0.0.1** (or localhost).

1. Open MSCC (WPF) → **CW** tab.
2. Check or uncheck **Use external electronic keyer (legacy)**.
3. If a session is running, accept **Restart local backends** → client writes local `mscc.ini`, Stop, Start.
4. Done. No separate bat/init step required.

| Checkbox | Local `mscc.ini` | Behavior |
|----------|------------------|----------|
| Unchecked (default) | `PROFICIO-MKII=1` | MKII internal keyer |
| Checked | `PROFICIO-MKII=0` | Legacy / external keyer |

---

## Remote / connect-only setup

**Client:** Launch Servers **OFF**, and/or Server IP is the radio PC (not loopback).

The client **must not** kill or restart host backends. PROFICIO-MKII must be set on the **radio computer**.

### When the client checkbox changes while connected

1. Client saves sticky UI preference.
2. Session **Stops** (disconnect only if Launch Servers was off).
3. Popup explains host restart (if needed).
4. Operator fixes host if required, then presses **Start** on the client.

### Radio PC is already in the desired mode

Example: Linux already configured legacy (`PROFICIO-MKII=0`) via mscc-init.

- No host restart required for that flag.
- On Windows client: check **external electronic keyer** so UI grays correctly (optional but recommended).
- If prompted: Stop → **Start** only reconnects. Host stays as-is.

### Radio PC needs a mode change — Windows host

On the **radio** machine (no full MSCC client required):

**Preferred (desktop icon):** run **`C:\mscc-net9\MSCC-Remote.exe`**

- Window: Start / Stop / Restart / Status / **Legacy CW** / **MKII**
- **Create desktop shortcut** once so it sits next to MSCC.Wpf
- CLI (Task Scheduler / scripts):  
  `MSCC-Remote.exe legacy` · `MSCC-Remote.exe mkii` · `MSCC-Remote.exe start` · …

**Or batch (same actions):**

```bat
cd /d C:\mscc-net9
Start-MsccServers.bat legacy     rem PROFICIO-MKII=0 + restart backends
Start-MsccServers.bat mkii       rem PROFICIO-MKII=1 + restart backends
Start-MsccServers.bat keyer      rem show current mode only
```

Then on the **client** PC: Launch Servers off, Server IP = radio PC → **Start**.

### Radio PC needs a mode change — Linux host

1. **mscc-init** (GUI or CLI): set **PROFICIO-MKII** 0 or 1 → writes `~/mscc.ini`.
2. Restart ms-sdr (and usual companions) on that host.
3. Client: **Start** / Connect.

---

## Decision guide

```
Is Launch Servers ON and Server IP loopback?
  YES → LOCAL: checkbox writes mscc.ini + can auto Stop/Start backends
  NO  → REMOTE / CONNECT-ONLY:
          checkbox = client UI sticky only
          host flag = radio PC mscc.ini (bat / mscc-init)
          client Stop → (restart host if flag changed) → client Start
```

---

## Windows remote host workflow (checklist)

| Step | Where | Action |
|------|--------|--------|
| 1 | Radio PC | `MSCC-Remote.exe` → Legacy CW or MKII (or `MSCC-Remote.exe legacy`) |
| 2 | Radio PC | Confirm Status / keyer line in the app |
| 3 | Client PC | Launch Servers **unchecked**; Server IP = radio PC |
| 4 | Client PC | CW tab: match checkbox to host mode (checked = legacy) |
| 5 | Client PC | **Start** |

Optional boot: `Install-MsccServers-AtBoot.bat` starts backends at logon; after a keyer mode change, run `legacy`/`mkii` once (or `restart` if ini already edited).

---

## Linux remote host workflow (checklist)

| Step | Where | Action |
|------|--------|--------|
| 1 | Radio PC | mscc-init → PROFICIO-MKII 0 or 1 |
| 2 | Radio PC | Restart ms-sdr (service/script as you normally do) |
| 3 | Client (Win or Avalonia) | Connect to radio IP; Launch Servers off if Windows |
| 4 | Client | Match external-keyer checkbox to host; Connect/Start |

---

## Avalonia notes

- Avalonia is **connect-only** (does not spawn Windows backends).
- Loopback host: checkbox may update this machine’s `mscc.ini` / `~/mscc.ini`; still **restart ms-sdr** yourself, then Connect.
- Remote host: sticky UI only; use host tools above.

---

## Troubleshooting

| Symptom | Likely cause |
|---------|----------------|
| Checkbox changed, keyer behavior unchanged | Host ms-sdr not restarted after `PROFICIO-MKII` change |
| Client checked, host still full keyer | Host still `PROFICIO-MKII=1` |
| Host legacy, client shows full keyer UI | Client checkbox unchecked — UI only; radio may still be correct |
| Two ms-sdr instances | Launch Servers ON while backends already running via bat |
| Remote client “restart” did nothing to radio | Expected: client only disconnects; use host bat/mscc-init |

---

## Related files

- Windows host GUI: `C:\mscc-net9\MSCC-Remote.exe` (source `src/MSCC.Remote/`)
- Windows host bat: `C:\mscc-net9\Start-MsccServers.bat` (source under `mscc-new/deploy/`)
- Windows host README: `Start-MsccServers-README.txt`
- Linux: mscc-init + `~/mscc.ini`
- Client sticky: CW tab checkbox (WPF & Avalonia)
