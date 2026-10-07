# Testing MSCC Avalonia (connect-only client)

**Version:** 0.1.0  
**What this build does:** Opens a window, connects over UDP to **ms-sdr** that is **already running**, sends GUI-ready + keep-alive, and shows basic reports (freq, mode, band, S-meter, core/FW versions when the server sends them).

**What it does *not* do yet:** Launch servers, spectrum/waterfall, full operate UI, `.deb` install.

---

## Before you start (Pi)

1. Raspberry Pi OS **64-bit** with desktop (so a GUI window can open).
2. MSCC servers already installed and known-good:
   - `mscc-portaudio` → `mscc` (and optional `mscc-init-gui`)
3. You can start/stop servers from the menu or CLI.

### Check servers are running

**Menu:** **MSCC Start** (Sound & Video), wait a few seconds.

**Or terminal:**

```bash
mscc start
# optional:
pgrep -a ms-sdr
pgrep -a sdrcore
```

You should see `ms-sdr` and the sdrcore processes.

### Config note (remote later; skip for on-Pi test)

For **GUI on the same Pi** as the servers, host **`127.0.0.1`** is correct.

If the server’s `mscc.ini` / Multus config has `MSCC_IP` set to another PC’s IP (for Windows remote client), pan/replies may be aimed at that PC. For this connect test, control packets to `127.0.0.1:8888` often still work; if you get connect but **no** reports, we may need `MSCC_IP=127.0.0.1` in `~/.local/mscc/` for local GUI tests. Tell Nate/Grok what you see.

---

## Option A — Test on Windows first (optional, easy)

Use this if you have Windows servers running (`C:\mscc-net9` or your usual Launch Servers path) **or** you only want to see the window open.

From a **Developer PowerShell** or normal PowerShell:

```powershell
cd "C:\Users\n8vet\OneDrive\Documents\MSCC-Grok-Build\Avalonia Migration"
dotnet run --project src\MSCC.Avalonia\MSCC.Avalonia.csproj -c Release
```

Or after a publish (see below), run `MSCC.Avalonia.exe`.

1. Start MSCC servers on Windows (Launch Servers / your batch).
2. In Avalonia: Host `127.0.0.1`, Port `8888`, **Connect**.
3. Expect: Status → Connected; packet count rising; Core/freq/mode if servers push them.

---

## Option B — Test on the Raspberry Pi (main goal)

### B1. Install .NET 9 runtime on the Pi (one time)

This first drop is **framework-dependent** (smaller). On the Pi:

```bash
# Follow current Microsoft docs if these packages move; goal is .NET 9 runtime + desktop deps.
wget https://dot.net/v1/dotnet-install.sh -O dotnet-install.sh
chmod +x dotnet-install.sh
./dotnet-install.sh --channel 9.0 --runtime dotnet
```

Add to `~/.bashrc` (adjust path if the install script printed a different one):

```bash
export DOTNET_ROOT=$HOME/.dotnet
export PATH=$PATH:$HOME/.dotnet
```

Then:

```bash
source ~/.bashrc
dotnet --list-runtimes
```

You want a **Microsoft.NETCore.App 9.x** line.

**Easier alternative:** use the **self-contained** publish folder (`publish/linux-arm64-sc`) so you do **not** need to install the .NET runtime. That folder is larger.

**Native libs Avalonia may need** (Pi OS):

```bash
sudo apt update
sudo apt install -y libice6 libsm6 libfontconfig1 libx11-6
# If the app complains about missing shared libs, paste the error — we'll install the right package.
```

### B2. Copy the published folder to the Pi

On the **Windows PC** (after Grok/you publish — see [Build & publish](#build--publish)):

Folder example:

`Avalonia Migration\publish\linux-arm64\`

Copy that whole folder to the Pi, e.g.:

- USB stick → `~/mscc-avalonia/`
- Or from Windows (Pi IP example `192.168.1.50`, user `ron`):

```powershell
scp -r "C:\Users\n8vet\OneDrive\Documents\MSCC-Grok-Build\Avalonia Migration\publish\linux-arm64" ron@192.168.1.50:~/mscc-avalonia
```

### B3. Run on the Pi desktop

Open **Terminal** on the Pi desktop (not a headless-only session if you want a window):

```bash
mscc start
cd ~/mscc-avalonia
chmod +x MSCC.Avalonia
./MSCC.Avalonia
```

If you get `permission denied` or `cannot execute`, check architecture:

```bash
uname -m
# should be aarch64
file MSCC.Avalonia
```

### B4. In the app

| Control | Value for on-Pi test |
|--------|----------------------|
| Host | `127.0.0.1` |
| Port | `8888` |
| Local RX | `8889` |
| Button | **Connect** |

### Pass criteria (good enough for this milestone)

- [ ] Window opens (no crash on start).
- [ ] **Connect** does not show an immediate exception in Status / log.
- [ ] Status moves toward **Connected**.
- [ ] **Packets** count increases (or stay 0 only briefly then rise).
- [ ] Prefer: **Core** version and/or **Freq** / **Mode** / **Band** fill in.
- [ ] **Disconnect** leaves servers running (`pgrep -a ms-sdr` still shows process).
- [ ] Log file path shown in UI exists and grows:

  - Linux: usually `~/.local/share/MSCC-Avalonia/logs/mscc.log`  
  - Windows: `%LocalAppData%\MSCC-Avalonia\logs\mscc.log`

### Failures and what to send back

| Symptom | What to try / capture |
|--------|------------------------|
| Window never opens | Terminal error text; `dotnet --list-runtimes`; `ldd MSCC.Avalonia` if binary |
| Connect failed: address in use | Something else bound 8889 — stop other MSCC client or set Local RX to `0` |
| Connected but packets stay 0 | Servers running? `ss -ulnp \| grep -E '8888\|8889'`; paste ms-sdr log if any |
| Keep-alive warning after ~10s | Servers died or wrong host; `pgrep -a ms-sdr` |
| Second client rejected | Expected if WPF or another client already holds the session — disconnect the other |

Paste into chat: **Status line**, last ~20 **Activity log** lines, and whether **packets** moved.

---

## Build & publish (Windows machine)

From the repo:

```powershell
cd "C:\Users\n8vet\OneDrive\Documents\MSCC-Grok-Build\Avalonia Migration"

# Quick run on Windows
dotnet run --project src\MSCC.Avalonia\MSCC.Avalonia.csproj -c Release

# Publish for Raspberry Pi 64-bit (framework-dependent — needs .NET 9 on Pi)
dotnet publish src\MSCC.Avalonia\MSCC.Avalonia.csproj -c Release -r linux-arm64 --self-contained false -o publish\linux-arm64

# Optional: self-contained (larger, no runtime install on Pi)
dotnet publish src\MSCC.Avalonia\MSCC.Avalonia.csproj -c Release -r linux-arm64 --self-contained true -o publish\linux-arm64-sc
```

There is no `.deb` yet. When the connect client is stable we can package it like `mscc-init-gui`.

---

## Architecture reminder

```text
Avalonia client  --UDP 8888-->  ms-sdr  (brain)
       ^                         |
       |                    recv / trans
  bind 8889 (pan later)
```

This milestone only proves the **client ↔ ms-sdr** session path using shared **MSCC.Core** (`UdpRadioService`, connect-only).
