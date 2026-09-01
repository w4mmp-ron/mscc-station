# MSCC — WPF conversion (`mscc-new`)

Modernization of the Multus / MSCC WinForms radio GUI to **.NET 9 WPF** with MVVM, using the existing UDP protocol and the same subsystem binaries (ms-sdr / recv / trans).

| Doc | Purpose |
|-----|---------|
| **PROGRESS.md** | Session-by-session progress log (append after each work day) |
| **RESUME.md** | Full handoff status, ownership rules, features, key files, resume prompt |
| **README.md** | This overview — run, deploy, architecture |
| **MIGRATION_STRATEGY.md** | Original phased plan (historical) |

Original WinForms sources: sibling `..\mscc` (reference only).

---

## Architecture

| Project | Role |
|---------|------|
| **MSCC.Core** | Domain, `IRadioService` / `UdpRadioService`, opcodes, UDP transport, logging |
| **MSCC.Wpf** | UI: `MainViewModel`, `MainWindow`, spectrum, analog meters, S/W + LOG popups |

**Client version:** `ClientVersion.txt` + `bump-client-version.ps1` → **month.day.iteration** (bumped on WPF build).

**Deploy:** post-build copies GUI output to **`C:\mscc-net9`**.

---

## Current status (2026-07-18)

Usable real-radio client with operate-on-face layout; **Proficio + Geminus** in one app (see **PROGRESS.md**).

- **Left rail:** audio, filters, RIT, server IP/port, Start, Auto Start, **Launch Servers**, TIME DISPLAY; temps at bottom  
- **Top:** analog **S-METER** / **ALC** (labels under meters), Mode/VFO, VFO A/B  
- **Band bar:** **Proficio|Geminus** selector; **2200 / 630** + HF; **GEN** (HF beacons or LF 198/660/880)  
- **Center:** tabs + spectrum (MAIN); **QRP CAL** / AMP CAL / IQ / FREQ CAL / CW / RX-TX / favorites  
- **Right rail (always visible):** PTT cluster, compression, AGC fast release, NB/NR, MON, TX BW, CW speed/pitch, LOG, versions  

### Modes of operation

| Mode | How | Close client |
|------|-----|--------------|
| **Local all-in-one** | **Launch Servers** checked | Sends STOP (0xFF) if this session launched backends |
| **Remote / shared backends** | Launch Servers **unchecked**; backends via `Start-MsccServers.bat` or logon task | No STOP — servers keep running |

**External electronic keyer / legacy CW** (`PROFICIO-MKII` in host `mscc.ini`): CW tab checkbox. Local (Launch Servers + loopback) auto-restarts backends; remote stops the client only — set host mode with `Start-MsccServers.bat legacy|mkii` (Windows) or mscc-init (Linux). Full guide: [`docs/EXTERNAL-KEYER-LOCAL-REMOTE.md`](docs/EXTERNAL-KEYER-LOCAL-REMOTE.md).

Keep-alive: client sends 0xF4 ~1/s; no server reply for **10 s** → dialog Continue or **close app**.

### Versions (right panel)

- **MSCC:** this build  
- **Core:** ms-sdr (`0xB3`)  
- **FW:** radio firmware (`0xB2`)  

### Client data (`%LocalAppData%\MSCC-NET9\`)

App **creates defaults** if missing (no init files required to start).

| File | Role |
|------|------|
| `MSCC_Client.ini` | Connection + UI/spectrum/power prefs |
| `MSCC_LastUsed.ini` / `MSCC_LastUsed_VFOB.ini` | Per-band last used (VFO A/B) |
| `MSCC_Favorites.ini` | Favorites |
| `client-settings.ini` | TRANS CAL status lamps (client-local) |
| `amp-cal-status.ini` | AMP CAL status lamps (client-local) |

**Note:** Cal **lamps** are PC-local until a future server push; radio cal **tables** stay on the server.

Server-owned examples: `startup.ini` (last freq/mode), calibration data files — not managed by this GUI’s init set.

---

## How to build and run

```bat
cd mscc-new
dotnet build MSCC.sln -c Release
```

Close any running `MSCC.Wpf.exe` first if copy to `C:\mscc-net9` fails (file lock).

Run: `C:\mscc-net9\MSCC.Wpf.exe`  
Requires **.NET 9 Desktop Runtime** on the target PC.

### Real radio / backends

Place next to the GUI (typical `C:\mscc-net9`):

- `ms-sdr-MKII.exe` (or variant), `mscc-recv.exe`, `Mscc-trans.exe`  
- `portaudio_x86.dll`, `pthreadVC2.dll`  

**Optional helpers**

| Script | Purpose |
|--------|---------|
| `Initialize.bat` | One-time defaults → LocalAppData (blocked after first run by flag file) |
| **`MSCC-Remote.exe`** | Desktop-friendly WinForms host tool: start/stop/restart backends, legacy/MKII keyer, desktop shortcut button |
| `Start-MsccServers.bat [start\|stop\|restart\|status\|legacy\|mkii\|keyer]` | Same actions as script (Task Scheduler / CLI); bat still fine for boot |
| `Install-MsccServers-AtBoot.bat` | Logon task as **current user** (not SYSTEM) |

Remote client: set Server IP to the radio PC; **Launch Servers** off.

Logs: `%LocalAppData%\MSCC-NET9\logs\` and **LOG** popup.

### Installer (Actual Installer etc.)

**Keep:** `MSCC.Wpf.exe`, `MSCC.Wpf.dll`, `MSCC.Wpf.deps.json`, `MSCC.Wpf.runtimeconfig.json`, `MSCC.Core.dll`, `CommunityToolkit.Mvvm.dll`, subsystem exes + native DLLs above, optional bat/vbs helpers.

**Skip:** old `Uninstall.exe` / build junk. Prefer Actual Installer’s own uninstaller.

---

## Tabs (brief)

| Tab | Notes |
|-----|--------|
| **MAIN** | Spectrum / waterfall |
| **CW** | Keyer configuration |
| **RX/TX** | Powers, default filters, TX BW + AGC processing, QRP/Full/Tune/ALC 2×2 |
| **FAVORITES** | Client-only |
| **QRP CAL** | Transceiver/QRP power cal (was TRANS CAL); AMP off; HF + 2200/630 |
| **RX IQ** | Manual RX I/Q (wired) |
| **TX IQ** | Manual TX I/Q; QRP only |
| **AMP CAL** | Enabled when AMP on |
| **FREQ CAL** | Frequency calibration |

---

## Spectrum / S/W (client)

- Span **72 kHz**; click-to-tune; AUTO SNAP (1k/500/100; not CW)  
- VIEW GRID, dB LABELS, PEAK MARKER (**right-click** place; left-click tunes)  
- TX BW above 2.7 kHz warns before accept  

---

## Parked / future

- Server **single-session** client attach protocol (e.g. `CMD_CLIENT_SESSION`) — design discussed; not coded  
- Server-side cal status lamps  
- **Avalonia** UI migration (planned by maintainer after handoff)  
- Full multi-client session counter on ms-sdr  

---

## Known notes

- Freq Cal may still render spectrum when tab is hidden (CPU).  
- S/W and Debug Log window placement: **session-only** (not INI).  
- Window geometry / many prefs: `MSCC_Client.ini` via `SpectrumWaterfallSettings`.  

See **RESUME.md** for the full handoff checklist and resume prompt.
