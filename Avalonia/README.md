# MSCC Avalonia (Ron's copy, mscc-avalonia 1.x)

This is Ron's copy of Stew's `mscc-ui/Avalonia-Migration` (0.6.75), with its own
`src/MSCC.Core`. It installs beside Stew's mscc-ui and doesn't touch his settings.
The notes further down are from Stew's copy and are partly out of date for this one.

## Build and install

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File build-mscc-avalonia-deb.ps1
```

Bump `<Version>` in `src/MSCC.Avalonia/MSCC.Avalonia.csproj` and `Version:` in
`packaging/mscc-avalonia/DEBIAN/control` first. The title bar shows the csproj version.
Copy the .deb to the Pi, then:

```bash
sudo apt install -y ./mscc-avalonia_<version>_arm64.deb
```

### Ubuntu / Debian PC (amd64)

```bash
pwsh -NoProfile -File build-mscc-avalonia-deb.ps1 -Arch amd64
sudo apt install -y ./mscc-avalonia_<version>_amd64.deb
```

Same script and same version numbers as the Pi; `-Arch amd64` builds for `linux-x64` and makes
`mscc-avalonia_<version>_amd64.deb`. It runs on Linux too (needs .NET SDK 9 or later,
`pwsh`, `python3`). On Ubuntu 26.04: `sudo apt install dotnet-sdk-10.0` and
`sudo snap install powershell --classic`; the .NET 10 SDK builds this .NET 9 app.

- App: `/opt/mscc-avalonia`, launcher `mscc-avalonia`, menu "MSCC Avalonia".
- Settings: `~/.config/MSCC-Avalonia/mscc-avalonia.ini`. Close Avalonia before editing it by
  hand, or it writes over your change when it closes.

## Remote Digital (WSJT-X on this computer, radio somewhere else)

The package makes two virtual sound devices at every login (user service
`mscc-avalonia-remote-audio.service`):

| Device | Who plays into it | Who listens |
|---|---|---|
| `MSCC_Remote_RX` | Avalonia (radio receive audio) | WSJT-X Input = `MSCC_Remote_RX.monitor` |
| `MSCC_Remote_TX` | WSJT-X Output | Avalonia = `MSCC_Remote_TX.monitor` |

If they're missing, run the script by hand:

```bash
/usr/share/mscc-avalonia/bin/mscc-avalonia-remote-audio.sh
```

WSJT-X settings:

| WSJT-X | Setting |
|---|---|
| Radio | Kenwood TS-2000 |
| Serial port | `/dev/ttyUSB11` (= `/dev/tnt1`; Avalonia holds `/dev/tnt0`) |
| PTT Method | RTS or CAT (not VOX: nothing keys on audio) |
| Audio Input | `MSCC_Remote_RX.monitor` |
| Audio Output | `MSCC_Remote_TX` |

RTS works when `comm-port.ini` has `PIN=1` (Avalonia watches CTS on its end).
In the Remote window, Phones and Digital each remember their own devices and the last path.

Things to know:

- Start Avalonia, connect, press Remote, then start WSJT-X (the CAT port opens with Remote).
- With a Windows host, the servers stop when Avalonia closes. Start them again before the next
  connect.
- Keep each virtual device's name and description the same. A different description crashed
  PortAudio.
- Don't add ALSA names for these devices unless `pipewire-alsa` is installed. Without it they're
  broken and hide the real device from WSJT-X.

---

# Avalonia-Migration — Linux MSCC UI (Stew's notes)

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
