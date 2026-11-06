# Avalonia-Migration — Linux MSCC UI

**Linux GUI** (Avalonia `mscc-ui` for Raspberry Pi **and** Ubuntu).  
Does **not** own servers — Pi backends **`../../rpi/`**, Ubuntu backends **`../../linux/`**.  
Windows GUI lives under **`../windows-work-tree/mscc-mscc/mscc-new/`** (WPF).

Workspace map: [`../README.md`](../README.md).

---

## Role in the dual-platform setup

| Piece | Location |
|-------|----------|
| This UI (Avalonia / Pi) | **here** |
| Windows UI (WPF) | `../windows-work-tree/mscc-mscc/mscc-new/` |
| Shared UDP / opcodes | `../windows-work-tree/mscc-mscc/mscc-new/src/MSCC.Core/` (**project reference**) |
| Linux servers (Pi) | `../../rpi/` |
| Linux servers (Ubuntu) | `../../linux/` |

**Keep Avalonia in sync with WPF** for operator features (tabs, NB/NR/AN, CQ memory, etc.).  
**Protocol changes** go into **MSCC.Core** under the Windows tree first when possible, then rebuild Avalonia.

Ron often runs **Linux servers only** and uses **Windows WPF** as the client (not Avalonia). Avalonia is for a GUI **on** the Pi or for Linux-native UI testing.

---

## Layout

| Path | Purpose |
|------|---------|
| `MSCC.Avalonia.sln` | Solution |
| `src/MSCC.Avalonia/` | Avalonia UI |
| `../windows-work-tree/mscc-mscc/mscc-new/src/MSCC.Core/` | Shared protocol library |
| `KEYER-MEMORY-*.md` | Keyer CQ memory UI / UDP / handoff notes |
| `INSTALL-MSCC-UI.md`, `TEST-ON-PI.md`, `TESTING.md` | Install and test |
| `mscc-ui_*.deb`, `packaging/`, `publish/` | Pi packages and publish output |

---

## Quick start (Windows host, build Avalonia)

```powershell
cd "…\mscc-station\mscc-ui\Avalonia-Migration"
dotnet build MSCC.Avalonia.sln -c Release
dotnet run --project src\MSCC.Avalonia\MSCC.Avalonia.csproj -c Release
```

Pi install / test: **`TEST-ON-PI.md`**, **`INSTALL-MSCC-UI.md`**.

Connect-only to backends (local or remote). Server start is `mscc` (Pi package or Ubuntu `linux-build/mscc-linux.sh`), not this UI.

Debs: `../Release/avalonia/arm64/` (Pi) and `../Release/avalonia/x86_64/` (Ubuntu).
