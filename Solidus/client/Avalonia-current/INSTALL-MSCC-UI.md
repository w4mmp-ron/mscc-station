# Install MSCC UI on Raspberry Pi (64-bit)

> **Full Pi how-to (servers + UI + remote audio + firmware):**  
> **[`rpi/pi-install/INSTALL.md`](../../rpi/pi-install/INSTALL.md)** — start there.  
> Current UI package also lives in **`rpi/pi-install/packages/`** and `mscc-ui/Release/avalonia/arm64/`.

**Package:** `mscc-ui_0.6.44_arm64.deb` (use the filename in `rpi/pi-install/packages/` if newer)  
**Menu name:** **MSCC UI** (same **MSCC** menu group as Start / Stop / Init)

---

## On the Windows PC — build a new `.deb` (after UI code changes)

```powershell
cd "...\mscc-station\mscc-ui\Avalonia-Migration"
powershell -NoProfile -ExecutionPolicy Bypass -File .\build-mscc-ui-deb.ps1
```

Refresh the install kit:

```powershell
cd "...\mscc-station"
powershell -NoProfile -ExecutionPolicy Bypass -File .\rpi\pi-install\collect-packages.ps1
```

Copy to the Pi:

```powershell
scp ".\rpi\pi-install\packages\mscc-ui_*_arm64.deb" pi@PI_IP:~/Downloads/
```

Ubuntu amd64 UI package is separate: `mscc-ui/Release/avalonia/x86_64/` — see [`INSTALL-UBUNTU.md`](../../INSTALL-UBUNTU.md).

---

## On the Pi

Servers should already be installed (`rpi/pi-install/INSTALL.md` sections A). Then:

```bash
cd ~/Downloads   # or packages folder
sudo apt install -y ./mscc-ui_0.6.44_arm64.deb
```

Or:

```bash
sudo dpkg -i ./mscc-ui_0.6.44_arm64.deb
sudo apt-get install -f -y
```

## Run

1. **MSCC Start** (servers)  
2. **MSCC UI** — or terminal: `mscc-ui`

Host default: `127.0.0.1` port `8888`.

## Features (current UI)

| Feature | Notes |
|---------|--------|
| **Remote Audio** | Left rail — with **Phones**, sends audio device **2** (mic from Windows **MsccRemotePhones**). Disabled on **Digital**. See `mscc-remote-audio/STEW-REMOTE-AUDIO.md`. |
| **External electronic keyer** | CW tab — legacy / `PROFICIO-MKII=0`; restart servers after change. |
| **CQ memory** | CW tab R/P slots (MKII + keyer). |
| Connect / host | Point at remote Pi if UI is not local. |

## What gets installed

| Path | Purpose |
|------|---------|
| `/opt/mscc-ui/` | Self-contained Avalonia app (no .NET install needed) |
| `/usr/bin/mscc-ui` | Launcher |
| `/usr/share/applications/mscc-ui.desktop` | Menu entry (`Categories=X-MSCC`) |
| `/usr/share/icons/hicolor/*/apps/mscc-ui.png` | Icon |

## Settings (survive reinstall)

- `~/.config/MSCC/mscc-avalonia.ini`
- `~/.config/MSCC/mscc-favorites.ini`

## Uninstall

```bash
sudo apt remove mscc-ui
```

Does **not** remove servers package `mscc` or your sticky settings.

## Firmware upload

Not part of this UI package — use **MSCC → Firmware Upload** from the **`mscc`** server package (`bootloader-gui`). See `rpi/pi-install/INSTALL.md` section E.
