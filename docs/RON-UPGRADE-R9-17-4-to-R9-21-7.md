# Upgrade guide — R9-17-4 → R9-21-7 (Ron)

**Typical station:** Windows 11 **WPF client** → **Raspberry Pi** servers (radio on the Pi).  
**From:** Windows `mscc-net9-R9-17-4-install.exe`  
**To:** Windows **`mscc-net9-R9--21-7-install.exe`** + Pi **`mscc_1.0.44_arm64.deb`** (+ optional Avalonia **`mscc-ui_0.6.59_arm64.deb`**)

UI change summary: [`RON-2026-09-21-UI-CHANGES.md`](RON-2026-09-21-UI-CHANGES.md).

Kit folders in the repo:
- Windows: `installers/windows/`
- Pi drop: `installers/rpi/` **or** `rpi/Rpi-installers/` (same debs; use whichever folder Stew sends you)

> Note the Windows filename: **`R9--21-7`** (two dashes after `R9`). That is the current ship EXE.

---

## Before you start

1. Note your Pi IP and that WPF still Connects on UDP **8888**.
2. Optional backup (only if you care about rolling back):
   - Windows: copy `C:\mscc-net9` (or your install folder).
   - Pi: `cp -a ~/mscc ~/mscc.bak-$(date +%Y%m%d)` if you have custom files there.
3. Close **WSJT-X / digi apps**, **WPF**, and any Avalonia UI.
4. On the Pi: stop the stack (`mscc stop` or your usual Start/Stop desktop action).

You do **not** need to reinstall PortAudio or init-gui for this jump unless Stew includes new versions in the kit.

---

## A. Upgrade the Windows client (required)

1. Copy **`mscc-net9-R9--21-7-install.exe`** to the PC.
2. Run it (Advanced Installer). Keep the usual deploy folder (**`C:\mscc-net9`** unless you customized).
3. Let it finish; reboot only if the installer asks.
4. Confirm: start WPF → title / About shows **Client 9.21.7** (or build R9-21-7).

**Remote-only PC:** you only need the WPF client; you do **not** have to run Windows `ms-sdr` / recv / trans when the radio lives on the Pi.

---

## B. Upgrade Pi servers to **1.0.44** (required for today’s remote-audio fixes)

On the Pi, `cd` to the folder that has the new `.deb` files:

```bash
cd ~/Downloads/Rpi-installers    # or wherever you put the kit
# expect: mscc_1.0.44_arm64.deb  (and optionally mscc-ui_0.6.59_arm64.deb)
```

### 1) Install / reinstall the server package

Prefer the helper (avoids the apt `_apt` sandbox warning):

```bash
chmod +x install-mscc.sh
./install-mscc.sh --reinstall ./mscc_1.0.44_arm64.deb
```

Or plain apt:

```bash
sudo apt install -y --reinstall ./mscc_1.0.44_arm64.deb
```

### 2) Optional — Avalonia UI on the Pi

Only if you operate from the Pi screen:

```bash
sudo apt install -y ./mscc-ui_0.6.59_arm64.deb
```

If you **only** use Windows WPF, you can skip the UI `.deb`.

### 3) Restart servers

```bash
mscc start
# or use the MSCC Start desktop action
mscc status
```

### 4) Quick binary check (optional)

```bash
strings ~/mscc/sdrcore-trans | grep 'stream reset (REMOTE path)'
# expect a hit — confirms the cmd-031 reset is in the live binary
```

---

## C. First connect after upgrade

1. On Windows: start **MSCC WPF** only → **Connect** to the Pi IP, port **8888**.
2. Confirm FW / Core populate. If the “FW missing?” prompt appears, choose **Yes** once.
3. Start the radio; confirm band gate / title look right for that radio.
4. Digi: select **Remote Digital**, leave mic slider at **100%** unless the chain is hot, then TUNE / CQ as usual.
5. If audio sounds “mushy” after a **SESSION IN USE** fight with another PC: **Stop WPF fully**, on Pi run **`mscc stop` then `mscc start`**, reconnect from one client only.

---

## D. If something goes wrong

| Symptom | Try |
|---------|-----|
| Can’t Connect | Pi `mscc status`; firewall; same LAN; port 8888 |
| SESSION IN USE | Fully quit the other PC’s MSCC; Pi `mscc stop` / `mscc start`; one owner only |
| Digi TUNE mush, wire looks fine | Recycle Pi servers once; confirm `strings` reset line above; then re-enable Remote Digital |
| Want R9-17-4 back | Re-run `mscc-net9-R9-17-4-install.exe`; Pi reinstall previous `mscc_1.0.43_arm64.deb` if you kept it |

Ping Stew if FW major / band gray looks wrong for a specific radio — that is usually a **firmware identity** issue, not the client install.

---

## Version checklist (done when all true)

- [ ] Windows installer **R9-21-7** / Client **9.21.7**
- [ ] Pi package **mscc 1.0.44**
- [ ] (Optional) Pi UI **0.6.59**
- [ ] Remote Digital TUNE clean; mic slider usable
- [ ] Idle → Start restores DIG-U when that was last-used
