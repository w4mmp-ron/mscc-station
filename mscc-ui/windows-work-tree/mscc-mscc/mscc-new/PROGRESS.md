# MSCC WPF — Progress log

**Purpose:** Track what was done across sessions so Nate / Ron / Grok can resume cleanly.  
**Primary handoff doc:** still read `RESUME.md` + `README.md` first.  
**Workspace (Nate):** `C:\Users\n8vet\OneDrive\Documents\MSCC-Grok-Build\mscc-mscc\mscc-new`  
**Deploy / test:** `C:\mscc-net9\MSCC.Wpf.exe` (post-build auto-copy)  
**Radios:** Proficio MKII (HF) + Geminus (2200 / 630)

Newest entries at the **top**.

---

## 2026-07-18 — Nate + Grok (Build Heavy)

**Client version at end of day:** ~**7.18.6** (see `src/MSCC.Wpf/ClientVersion.txt`; bumps on each WPF build).

### Goals
- Pick up Ron’s WPF MSCC handoff; verify build/deploy.
- One GUI for **Proficio + Geminus** (no separate app).
- LF bands, radio-model UI, QRP/AMP cal LF, GEN cal carriers.

### Done

| Area | Detail |
|------|--------|
| **Compile smoke** | Title bar `MSCC` → `MSCC WPF`; confirmed `dotnet build -c Release` → `C:\mscc-net9`. |
| **Radio model control** | Button left of band bar: **Proficio ↔ Geminus** (UI label). |
| **Band gray-out** | Proficio → 2200/630 grayed; Geminus → 160–10 grayed; GEN always enabled. |
| **LF band buttons** | **2200** / **630** wired. Defaults: **136.000 kHz**, **474.200 kHz** (digital). Last-used + favorites include `2200m` / `630m`. |
| **Startup race** | Keep-alive / double-start flakiness: longer settle (2 s), cold-start keep-alive grace (20 s), orphan wait/adopt, longer STOP wait for ms-sdr. *Monitor if intermittent returns.* |
| **QRP CAL rename** | Tab **TRANS CAL** → **QRP CAL**. |
| **QRP CAL + AMP CAL LF** | Bands **2200** and **630** on both tabs (functional). Opcode band #s **2200** / **630**. Status keys: `GEMINUS_B2200` / `GEMINUS_B630` (QRP) and `AMP_B2200` / `AMP_B630` (AMP). Cal table freqs (original): 135.750 / 475.000 kHz. AMP still requires matching QRP lamp green. |
| **GEN (Geminus)** | When Geminus selected, GEN button rotates **198 / 660 / 880 kHz** (freq-cal carriers). Proficio GEN still WWV/CHU/RWM/USER. |

### Verified by Nate (in-session)
- Proficio + Geminus operate as expected for band select / gray-out.
- Favorites work with LF bands.
- QRP cal status appears in client `.ini` (`GEMINUS_B*` / `PROFICIO_B*`); deeper cal TX verification deferred to shack (better test gear).
- Geminus GEN 198/660/880 works.

### Not done / parked
- Radio model **not** sent to hardware as a model opcode (UI + gray-out only; PSoC still enforces TX).
- Radio model choice **not** persisted to INI yet.
- Auto-detect radio type from server (optional later).
- Full shack power-cal verification (QRP/AMP TX paths on LF).
- TX IQ / RX IQ LF bands (if needed).
- Avalonia / Linux port (later; stay on Windows until feature set is done).
- Freq Cal tab spectrum CPU when hidden (known note from Ron).

### How we build / test (reminder)
1. Close `MSCC.Wpf.exe` if running.  
2. `dotnet build MSCC.sln -c Release` from `mscc-new`.  
3. Run `C:\mscc-net9\MSCC.Wpf.exe`.  
4. Prefer **Launch Servers ON** for local all-in-one.  
5. Logs: `%LocalAppData%\MSCC-NET9\logs\mscc.log` and in-app **LOG**.

### Key files touched this session
- `MainWindow.xaml` / `MainWindow.xaml.cs` — band bar, radio model, GEN lists, gating  
- `ViewModels/MainViewModel.cs` — band names, QRP/AMP cal, favorites, defaults  
- `PowerCal/PowerCalStatusStore.cs` — LF + GEMINUS keys  
- `PowerCal/AmpCalStatusStore.cs` — LF bands  
- `Services/UdpRadioService.cs` — startup/keep-alive harden  

---

## Earlier (Ron handoff, 2026-07-18 morning / prior)

Ron’s tree already included (see `RESUME.md` for full detail):

- Operate-on-face layout, real UDP, Launch Servers on/off  
- Spectrum 72 kHz, S/W popup, favorites, CW, RX/TX, Freq Cal MANUAL/CHECK  
- Power/amp/TX-IQ/RX-IQ cal tabs (HF), analog meters  
- Client INI under `%LocalAppData%\MSCC-NET9\`  
- Post-build deploy to `C:\mscc-net9`  
- Planned long-term: **Avalonia**  

---

## Suggested next sessions (unordered)

1. Persist **Proficio/Geminus** in `MSCC_Client.ini`.  
2. Shack-test QRP/AMP cal on Geminus with instruments; fix any server/edge cases.  
3. Optional: gray GEN presets or auto-select model from firmware/report.  
4. Continue feature parity / polish; Avalonia only after Windows feature goals are met.  

---

*Append new dated sections at the top of this file after each work session.*
