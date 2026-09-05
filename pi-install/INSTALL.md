# MSCC on Raspberry Pi — one how-to

**Pi:** 4 or 5, **64-bit Raspberry Pi OS** (desktop recommended).  
**Packages:** use files in **`pi-install/packages/`** (this folder).  
**Updated:** 2026-09 — servers **1.0.40**, init-gui **1.0.13** (hides Proficio/Multus I/Q from speaker/mic pickers), portaudio **19.8.2**, UI **0.6.39**.

If a filename in `packages/` differs, **use the real name on disk**.

---

## Big picture

| Role | Where it runs |
|------|----------------|
| **Servers** (`ms-sdr`, `sdrcore-recv`, `sdrcore-trans`) | **Always on the Pi** (next to the radio) |
| **Operate UI** | **Windows WPF** on a PC, and/or **Avalonia MSCC UI** on the Pi |
| **Digi apps** (WSJT, etc.) | Prefer **on the Pi** (VirtualA / VirtualB) |
| **Remote operator voice** | Optional: phones AF + mic on a **Windows PC** via **MsccRemotePhones** |

```text
Windows WPF client  ──UDP──►  Pi servers  ──USB──►  Proficio / Geminus
       ▲                         │
       │                         ├── digi: VirtualA/B (local)
       └── optional remote AF ◄──┘
            MsccRemotePhones
            RX 9100 / TX 9101
```

**Typical setups**

1. **Headless Pi + Windows WPF** — Pi runs servers only; you operate from the PC.  
2. **Pi with Avalonia UI** — servers + **MSCC UI** on the Pi (local `127.0.0.1`).  
3. **Remote voice** — digi on Pi; operator phones/mic on Windows (**Remote Audio**).

---

## What you need

1. Pi on the network; desktop Terminal for install.  
2. Contents of **`pi-install/packages/`** on the Pi (`~/Downloads` or USB).  
3. Proficio/Geminus powered when configuring (recommended).  
4. Operator audio plan (especially **Pi 4**): prefer **I²S HAT** or digi-only — avoid USB headset + Proficio USB on Pi 4 under load.

---

## A. Install servers (required)

On the Pi Desktop Terminal, `cd` to the folder with the `.deb` files:

```bash
cd ~/Downloads/packages    # or wherever you copied pi-install/packages
```

### 1) PortAudio (first)

```bash
sudo apt install -y ./mscc-portaudio_19.8.2_arm64.deb
ldconfig -p | grep portaudio
# expect: /usr/local/lib/libportaudio.so.2
```

### 2) Main MSCC package (servers + firmware upload tools)

```bash
sudo apt update
sudo apt install -y ./mscc_1.0.40_arm64.deb
```

Optional helper (if present):

```bash
chmod +x ../install-mscc.sh   # if you copied install-mscc.sh next to packages
./install-mscc.sh
```

This installs:

| Piece | Notes |
|-------|--------|
| Servers | `$HOME/mscc/` — `ms-sdr`, `sdrcore-recv`, `sdrcore-trans` |
| **Firmware Upload** | `bootloader` + **`bootloader-gui`** — menu **MSCC → Firmware Upload** |
| Start / Stop / Status | Desktop **MSCC** menu + `mscc start\|stop\|status` |
| Digi sinks | VirtualA / VirtualB |
| Config seed | `$HOME/.local/mscc/` if empty |

### 3) Setup wizard

```bash
sudo apt install -y ./mscc-init-gui_1.0.13_all.deb
```

### 4) Log out / log in once (first install)

Needed so `dialout` / `audio` / `plugdev` groups apply (CAT and devices).

Check digi sinks:

```bash
pactl list short sinks | grep Virtual
# if empty:
systemctl --user enable --now mscc-virtual-audio
```

### 5) Configure once — **MSCC Init**

Menu **MSCC → MSCC Init** (or `mscc-init`):

- Keyer, CAT / PTT  
- Operator speaker & mic (not Proficio I/Q devices)  
- Digi fixed to VirtualA / VirtualB  
- **Proficio MKII vs Legacy** (`PROFICIO-MKII`) — use **Legacy (0)** for semi-break-in / external keyer radios  
- Optional: start servers at end of wizard  

Config lives in `$HOME/.local/mscc/` (upgrades do **not** overwrite).

### 6) Everyday servers

| Menu | Action |
|------|--------|
| **MSCC Start** | Start servers |
| **MSCC Stop** | Stop servers |
| **MSCC Status** | Health check |
| **MSCC Init** | Reconfigure |
| **Firmware Upload** | Flash Proficio/Geminus `.cyacd` (BOOT jumper → LOADER) |

```bash
mscc start
mscc status
mscc stop
```

---

## B. Install Avalonia UI on the Pi (optional)

Use when you want the operate GUI **on the Pi** (or a second Linux box).

```bash
sudo apt install -y ./mscc-ui_0.6.39_arm64.deb
```

| Item | Detail |
|------|--------|
| Run | Menu **MSCC UI**, or `mscc-ui` |
| Default host | `127.0.0.1` port `8888` (start servers first) |
| Settings | `~/.config/MSCC/mscc-avalonia.ini` (survive reinstall) |
| Uninstall | `sudo apt remove mscc-ui` (does not remove servers) |

### UI features to know (0.6.39+)

| Feature | Where / notes |
|---------|----------------|
| **CQ / keyer memory** | CW tab — 4 slots, R store / P play (MKII + keyer). |
| **Remote Audio** | Left rail checkbox — with **Phones** selected, sends audio device **2** (remote mic). Greyed on **Digital**. Sticky in ini. |
| **External electronic keyer (legacy)** | CW tab — sets `PROFICIO-MKII=0` in host `mscc.ini`; restart servers to apply. HOLD stays live. |
| **Spectrum / waterfall** | MAIN tab. Heals pan refresh on connect. Packet line shows `D5` / `Spec`. **0.6.39** denser keep-alives + larger UDP buffers so pan flood does not trip “keep-alive lost”. |
| Connect | Host / ports for remote Pi if UI is not on the radio machine |

**Note:** Rebuild the UI `.deb` after client code changes (`Avalonia-Migration/build-mscc-ui-deb.ps1`), then refresh `pi-install/packages/` with `collect-packages.ps1`.

More detail: `mscc-ui/Avalonia-Migration/INSTALL-MSCC-UI.md`.

---

## C. Windows client (WPF) against the Pi

1. Pi: **MSCC Start**; note Pi IP (`hostname -I`).  
2. Windows: run **MSCC.Wpf** from `C:\mscc-net9` (or your install).  
3. Settings: Server IP = Pi; **Launch Servers** usually **off** (servers already on Pi).  
4. **Start** / Connect.

Legacy CW: check **Use external electronic keyer** on CW tab, or set `PROFICIO-MKII=0` on the Pi (`mscc.ini` / Init / `MSCC-Remote` if used on Windows host).

---

## D. Remote audio (operator voice on Windows, digi on Pi)

**Servers already support it.** You need the companion app + UI checkbox.

### On the Pi

Edit (or create) phones stream target — Windows PC IP:

`$HOME/.local/mscc/remote-phones.ini`

```ini
ENABLED=1
HOST=192.168.x.x
PORT=9100
```

Mic listen port (usually default is enough):

`$HOME/.local/mscc/remote-mic.ini`

```ini
PORT=9101
```

Restart servers after editing. Digi stays on VirtualA/B.

### On Windows

1. Build/run **MsccRemotePhones** from `mscc-remote-audio/` (see that folder’s README).  
2. **RX:** listen **9100** (phones AF from Pi).  
3. **TX:** host = **Pi IP**, port **9101**, start mic TX.  
4. In **WPF or Avalonia**: Audio = **Phones**, check **Remote Audio** → client sends **`0x9B` / data 2**.  
5. Digital path: leave **Digital** selected for digi; Remote Audio is ignored (always device **0**).

Full handoff: `mscc-remote-audio/STEW-REMOTE-AUDIO.md`.

Test without UI (live session):

```powershell
cd ...\mscc-remote-audio
.\Set-AudioDevice.ps1 -HostName PI_HOSTNAME -Mode remote
```

---

## E. Firmware upload (bootloader)

Included in the **`mscc`** package — no separate `.deb`.

1. Power off radio → install **BOOT** jumper → power on → Morse **LOADER** / USB `04b4:b71d`.  
2. **Stop** MSCC servers.  
3. Pi menu **MSCC → Firmware Upload** (`bootloader-gui`) or CLI `bootloader /path/to/file.cyacd`.  
4. Use the correct tree’s `.cyacd` (Proficio/Geminus MKII or Legacy under `Release-*`).  
5. Power off → **remove BOOT jumper** → power on → Proficio `16c0:05dc`.

Details: `Release-Proficio-Legacy/STEW-FIRMWARE-UPDATE.md` (same procedure for MKII trees).

---

## F. Updating packages later

1. On the Windows/dev PC, put new builds in the usual trees (`mscc-deb/`, `mscc-ui/Avalonia-Migration/`, etc.).  
2. Refresh the kit:

```powershell
cd C:\Users\n8vet\OneDrive\Documents\GitHub\mscc-station
powershell -NoProfile -ExecutionPolicy Bypass -File .\pi-install\collect-packages.ps1
```

3. Copy new files from `pi-install/packages/` to the Pi.  
4. On the Pi:

```bash
cd ~/Downloads/packages
sudo apt install -y ./mscc-portaudio_….deb ./mscc_….deb ./mscc-init-gui_….deb ./mscc-ui_….deb
```

`apt install ./file.deb` upgrades in place. **Config under `~/.local/mscc/` and UI `~/.config/MSCC/` is kept.**

Then:

```bash
mscc restart
# or MSCC Stop / Start from the menu
```

Update the version table at the top of this file when you change the kit.

---

## Quick reference — where things live in the repo

| Need | Path |
|------|------|
| **This kit + how-to** | `pi-install/` |
| Server packaging / older install prose | `mscc-deb/` |
| Init GUI sources | `mscc-init-gui/` |
| Avalonia UI + build script | `mscc-ui/Avalonia-Migration/` |
| Windows WPF | `mscc-ui/windows-work-tree/` |
| Remote phones app | `mscc-remote-audio/` |
| Radio `.cyacd` | `Release-Proficio-*`, `Release-Geminus-*` |

---

## Checklist (first Pi)

- [ ] PortAudio → main `mscc` → init-gui → log out/in  
- [ ] VirtualA/B present; **MSCC Init** done (`PROFICIO-MKII` correct)  
- [ ] **MSCC Start**; Status OK; Proficio USB seen  
- [ ] Optional: **mscc-ui** on Pi, or Windows WPF to Pi IP  
- [ ] Optional: Remote Audio + MsccRemotePhones  
- [ ] Optional: Firmware Upload with correct `.cyacd`  
