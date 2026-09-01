# Install MSCC UI on Raspberry Pi (64-bit)

**Package:** `mscc-ui_0.6.32_arm64.deb`  
**Menu name:** **MSCC UI** (same **MSCC** menu group as Start / Stop / Init)

## On the Windows PC

Deb is built next to this file after:

```powershell
cd "…\Avalonia Migration"
powershell -NoProfile -ExecutionPolicy Bypass -File .\build-mscc-ui-deb.ps1
```

Copy to the Pi (example):

```powershell
scp ".\mscc-ui_0.6.32_arm64.deb" pi@PI_IP:~/
```

## On the Pi

```bash
sudo apt install -y ./mscc-ui_0.6.32_arm64.deb
```

Or:

```bash
sudo dpkg -i ./mscc-ui_0.6.32_arm64.deb
sudo apt-get install -f -y   # if deps missing
```

## Run

1. **MSCC Start** (servers)  
2. **MSCC UI** (this client) — or terminal: `mscc-ui`

Host default: `127.0.0.1` port `8888`.

## What gets installed

| Path | Purpose |
|------|---------|
| `/opt/mscc-ui/` | Self-contained Avalonia app (no .NET install needed) |
| `/usr/bin/mscc-ui` | Launcher |
| `/usr/share/applications/mscc-ui.desktop` | Menu entry (`Categories=X-MSCC`) |
| `/usr/share/icons/hicolor/*/apps/mscc-ui.png` | Icon |
| `/usr/share/pixmaps/mscc-ui.png` | Icon fallback |

## Settings (user home — survive reinstall)

- `~/.config/MSCC/mscc-avalonia.ini`
- `~/.config/MSCC/mscc-favorites.ini`

## Uninstall

```bash
sudo apt remove mscc-ui
```

Does **not** remove servers package `mscc` or your sticky settings.
