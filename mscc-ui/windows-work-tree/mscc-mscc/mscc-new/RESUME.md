# MSCC — WinForms to WPF Migration (Resume / Handoff)

**Last updated:** 2026-07-18 (Nate + Grok session)  
**Workspace (Nate):** `C:\Users\n8vet\OneDrive\Documents\MSCC-Grok-Build\mscc-mscc\mscc-new`  
**Original WinForms reference:** sibling `..\mscc` (read-only)  
**Deploy folder:** `C:\mscc-net9` (post-build copies GUI + deps here; backends live alongside)

**First reads:** this file → **`PROGRESS.md`** (session log) → `README.md` → `MIGRATION_STRATEGY.md` (historical).

---

## Project status

| Item | Status |
|------|--------|
| Stack | .NET 9: **MSCC.Core** + **MSCC.Wpf** (MVVM / CommunityToolkit.Mvvm) |
| Radios | **Proficio (HF)** + **Geminus (2200/630)** in one GUI |
| Radio | Real UDP to ms-sdr; optional local launch of backends |
| Client version | `ClientVersion.txt` + pre-build bump → **M.D.I** (e.g. 7.18.x) |
| Layout | Operate-on-face commercial-rig style (stable) |
| Next major effort (planned) | Feature finish on Windows; later **Avalonia** |

### Nate session highlights (2026-07-18) — see PROGRESS.md

- Proficio/Geminus selector + band gray-out  
- 2200/630 band buttons (defaults 136.0 / 474.2 kHz); favorites  
- Startup/keep-alive harden (monitor)  
- **QRP CAL** (was TRANS CAL) + **AMP CAL**: LF bands 2200/630 functional  
- Geminus **GEN**: 198 / 660 / 880 kHz freq-cal rotation

---

## UI layout (current)

```
┌──────────────┬────────────────────────────────────────────┬──────────────┐
│ LEFT PANEL   │  Top: S-METER | Mode/VFO | VFO A | VFO B | ALC          │
│ (full height)│  [Proficio|Geminus] | 2200 630 160…10 GEN              │
│              ├────────────────────────────────────────────┼──────────────┤
│ Audio P/D    │  Tabs (MAIN, CW, RX/TX, FAVORITES, cal…)  │ RIGHT PANEL  │
│ Filters/step │  Tab content = spectrum * column only      │ (always on)  │
│ RIT, Server  │  QRP CAL / AMP CAL / IQ / FREQ CAL        │ PTT cluster  │
│ Start        │                                            │ CMP / AGC…   │
│ Auto Start / │                                            │ NB/NR/MON…   │
│ Launch Srv / │                                            │ CW / LOG     │
│ TIME DISPLAY │                                            │ VERSIONS bot │
│ TEMPS bottom │                                            │              │
└──────────────┴────────────────────────────────────────────┴──────────────┘
```

Labels under meters: **S-METER**, **ALC**.

**Band bar:** radio model left of bands; LF left of HF; GEN model-specific (see PROGRESS.md).

---

## Architecture highlights (handoff)

### Launch Servers vs connect-only
| **Launch Servers** (left panel) | Start | Close |
|----------------------------------|-------|-------|
| **ON** | Spawns ms-sdr / recv / trans from exe dir | Sends **CMD_SET_STOP (0xFF)** |
| **OFF** | UDP connect only (remote or already-running backends) | **No STOP** — leaves servers running |

Tracked as `_launchedSubsystemsThisSession` at `StartAsync` time.

### Keep-alive (client ↔ ms-sdr)
- Client **sends** `CMD_SET_KEEP_ALIVE` (0xF4) every **1 s**.
- Client **watches** replies; if none for **10 s** → warning dialog.
  - **Yes** = Continue (reset watch)
  - **No** = **Close MSCC** (normal window close path)
- High-rate RX log suppressed for: spectrum, smeter, keep-alive, temp, **ALC (0x4F)**.

### Versions (right panel)
| Label | Source |
|-------|--------|
| **MSCC:** | This client build |
| **Core:** | `CMD_GET_SET_MSSDR_VERSION` (0xB3) |
| **FW:** | `CMD_GET_SET_FIRMWARE_VERSION` (0xB2) |

**SDR:** line removed. Recv/trans version reports ignored (server no longer sends).

### Client vs server data ownership
| Client (`%LocalAppData%\MSCC-NET9\`) | Server (ms-sdr / backends) |
|--------------------------------------|----------------------------|
| `MSCC_Client.ini` (UI, IP, spectrum, powers, …) | Cal tables, power/iq/etc. cal data |
| `MSCC_LastUsed.ini` / `_VFOB.ini` | `startup.ini` (last freq/mode, pushed at start) |
| `MSCC_Favorites.ini` | Other backend inis |
| `client-settings.ini` (QRP CAL lamps: PROFICIO_B* + GEMINUS_B2200/630) | |
| `amp-cal-status.ini` (AMP CAL lamps: AMP_B* incl. 2200/630) | |

**None of the client files are required at first launch** — app creates defaults.  
**Caveat:** cal **status lamps** are client-local; moving the GUI to another PC can show wrong “cal done” lights even if radio cal data is fine. Future: push status from server (not implemented).

### Init / deploy helpers (`C:\mscc-net9`)
| File | Role |
|------|------|
| `Initialize.bat` | **One-time** config install; blocked by `MSCC_INIT_COMPLETE.flag` |
| `Start-MsccServers.bat` | Menu or silent: `start` / `stop` / `restart` / `status` |
| `Install-MsccServers-AtBoot.bat` | Logon task as **current user** (not SYSTEM); runs `start` |
| `runhidden.vbs` | Hidden process launch |

---

## Feature summary (implemented)

### Spectrum / S/W popup
- 72 kHz panadapter span (click-to-tune / scale).
- **AUTO SNAP** (1 kHz / 500 / 100 Hz) on spectrum click; not CW.
- **VIEW GRID**, **dB LABELS** (optional, independent).
- **PEAK MARKER** (optional): **right-click** places marker; left-click = tune only.
- Waterfall palettes, gain/zero, direction, time markers.
- S/W window: session-only placement.

### Operate / tabs
- MAIN spectrum; CW keyer; RX/TX powers/filters/TX options (2×2 QRP/Full/Tune/ALC; PROCESSING beside TX BW).
- **TX BW** > 2.7 kHz → warning Accept/Cancel.
- FAVORITES (client-only).
- TRANS CAL / AMP CAL / TX IQ / RX IQ / FREQ CAL.
- Tab enable: TRANS CAL + TX IQ when AMP off; AMP CAL when AMP on (dimmed headers).
- RX IQ fully wired (0x58/0x55 RX/0x52/0x57/0x8D, no IMAGE).

### Known limitations / parked
- **ms-sdr is single-session.** Parked design: client leave + busy reject (e.g. `CMD_CLIENT_SESSION` attach/detach/busy). Not implemented.
- Cal status lamps client-only (see ownership table).
- Freq Cal tab spectrum may still cost CPU when hidden.
- Side-panel vertical “air” left as design choice.

---

## Key files

| Path | Notes |
|------|--------|
| `src/MSCC.Wpf/MainWindow.xaml(.cs)` | Layout, bands, close, tab enter/leave |
| `src/MSCC.Wpf/ViewModels/MainViewModel.cs` | Commands, cal, favorites, keep-alive dialog |
| `src/MSCC.Core/Services/UdpRadioService.cs` | UDP, launch, STOP gate, keep-alive watch |
| `src/MSCC.Core/Protocol/Opcodes.cs` | Opcode map |
| `src/MSCC.Wpf/Controls/SpectrumDisplayControl.*` | Spectrum/waterfall, click/peak/grid/labels |
| `src/MSCC.Wpf/PanadapterControlsWindow.*` | S/W popup |
| `src/MSCC.Wpf/SpectrumWaterfallSettings.cs` | Client INI load/save |
| `src/MSCC.Wpf/PowerCal/*StatusStore.cs` | Cal lamp persistence |

---

## Build / run

1. Close `MSCC.Wpf.exe` if deploy copy locks.
2. `dotnet build MSCC.sln -c Release` (from `mscc-new`).
3. Run `C:\mscc-net9\MSCC.Wpf.exe`.
4. Prerequisites: **.NET 9 Desktop Runtime**; backends next to exe when Launch Servers is on.
5. Optional: `Initialize.bat` once; remote host: Launch Servers **off** + `Start-MsccServers.bat` / logon installer on server PC.
6. Logs: `%LocalAppData%\MSCC-NET9\logs\` and **LOG** popup.

### Installer keep-list (GUI folder)
`MSCC.Wpf.exe`, `.dll`, `.deps.json`, `.runtimeconfig.json`, `MSCC.Core.dll`, `CommunityToolkit.Mvvm.dll`, backends + `portaudio_x86.dll` / `pthreadVC2.dll`, optional bat/vbs helpers. See README.

---

## Resume prompt (paste for next session)

> Resume MSCC WPF in mscc-new (Nate workspace under MSCC-Grok-Build). One GUI for Proficio + Geminus: model button + band gray-out; 2200/630 defaults 136.0/474.2 kHz; GEN on Geminus = 198/660/880 kHz cal; QRP CAL (was TRANS CAL) + AMP CAL include 2200/630 with GEMINUS_B* / AMP_B* status. Startup/keep-alive hardened (monitor). Deploy C:\mscc-net9. Read PROGRESS.md + RESUME.md + README.md. Windows first; Avalonia later.

---

## Older history

Earlier work remains valid: real UDP, Freq Cal, GEN, step/digit wheel, window INI placement, favorites, power/amp/TX-IQ cal, gold L&F, etc. Detail in git history.
