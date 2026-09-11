# MSCC Raspberry Pi packaging — project memory

**Last updated:** 2026-07-26  
**Status:** Working end-to-end on Pi OS (install → audio loopback → mscc-init → mscc start).  
**D + TUNE RF:** fixed in `SDRcore-trans-linux` (rebuild binary into `mscc-binaries` before next .deb).  
**Platform (only):** Raspberry Pi OS **64-bit**, Pi **4/5**. No multi-distro support.

Use this file to resume work later. Paths are under the Windows worktree root unless noted.

---

## Worktree root

```
C:\Users\Ron\.grok\worktrees\
```

On the Pi this is often mirrored under a home path or copied as needed.

---

## Important locations

| Role | Path |
|------|------|
| **Packaging project** | `mscc-deb\` |
| **Built packages** | `mscc-deb\mscc_1.0.14_arm64.deb` (current) |
| **Build script** | `mscc-deb\build-deb.sh` |
| **Install helper** (avoid `_apt` home notice) | `mscc-deb\install-mscc.sh` |
| **Plain-English install card (MD)** | `mscc-deb\INSTALL-FOR-PI.md` |
| **Word install card (LibreOffice → PDF)** | `mscc-deb\INSTALL-FOR-PI.docx` |
| **This memory file** | `mscc-deb\PROJECT-MEMORY.md` |
| **Prebuilt arm64 apps** | `mscc-binaries\` |
| **Config seed .ini files** | `mscc-init-files-linux\` |
| **tty0tty sources** | `tty0tty-master\module\` |
| **mscc-init wizard source** | `mscc-init-linux\sources\main.c` |
| **Linux app sources (not in .deb)** | `ms-sdr-linux\`, `SDRcore-recv-linux\`, `SDRcore-trans-linux\`, etc. |

### Binaries shipped in the package (`mscc-binaries\`)

| File | Purpose |
|------|---------|
| `ms-sdr` | Control / CAT / hub (UDP 8888) |
| `sdrcore-recv` | RX core (UDP 9000) |
| `sdrcore-trans` | TX core (UDP 9200) |
| `mscc-init` | Interactive device/config wizard |
| `mscc.sh` | Start/stop/status/restart stack |
| `bootloader` | Proficio app `.cyacd` upload (BOOT jumper → HID `04b4:b71d`) |
| `audio-setup` | *(removed from package — digi is Pulse VirtualA/B only)* |

All four main ELFs are **aarch64**. Package is **binaries only** for apps (no compile of MSCC apps on install).

**Before next .deb:** rebuild `sdrcore-trans` from `SDRcore-trans-linux` and copy into `mscc-binaries/`; ensure `audio-setup` is the current script (rate/format fixed), not the old bare-plug copy.

### On the Pi after install

| Path | Contents |
|------|----------|
| `$HOME/mscc/` | Runtime binaries + `logs/` |
| `$HOME/.local/mscc/` | `.ini` config (ms-sdr / cores / seeds) |
| `/usr/local/bin/mscc` | → `$HOME/mscc/mscc.sh` |
| `/usr/local/bin/mscc-init` | → `$HOME/mscc/mscc-init` |
| `/usr/local/bin/mscc-audio-setup` | → `$HOME/mscc/audio-setup` |
| `/usr/share/mscc/` | Package payload (binaries, init-files, tty0tty sources) |
| `/dev/tnt0`…`tnt7` | tty0tty pairs (e.g. tnt0↔tnt1) |
| `MSCC_Cable_In` / `Out` | ALSA names from audio-setup / `~/.asoundrc.mscc-virtual` |
| `~/.asoundrc` | Include only: `<…/.asoundrc.mscc-virtual>` (real PCMs are in the snippet file) |

`mscc.sh` uses **`MSCC_DIR="${MSCC_DIR:-$HOME/mscc}"`** (not hardcoded `/home/ron`).

Config path both apps and init use: **`$HOME/.local/mscc`** via `My_getenv` → `$HOME/.local/mscc`.

Logs: **`$HOME/.local/mscc/sdrcore-trans.log`** (and recv similarly).

---

## Digital audio / D + TUNE (critical — 2026-07-26)

### Hardware / format (fixed)

Transceiver I/Q USB audio (Multus/Proficio): **2 channels, 16-bit, 96 kHz**.  
`sdrcore-trans` / PortAudio duplex is locked to **`SAMPLERATE = 96000`**.

### PortAudio vs Windows

- **PortAudio does not resample.** It opens the rate you request.
- **Windows** often resamples in MME/WASAPI so a 48k digi device still “works” at 96k.
- **Linux ALSA:** no free conversion unless a **`type plug`** (or similar) does it. Wrong rate → `Invalid sample rate` (−9997) or a dead stream.

### ALSA virtual cable (`audio-setup`)

| Name | Role |
|------|------|
| **`MSCC_Cable_Out`** | Digi app **play** into cable; **sdrcore-recv** digital speaker |
| **`MSCC_Cable_In`** | **sdrcore-trans** digital mic; digi app **record** |

Must be generated as:

```text
format S16_LE
rate 96000
channels 2
```

on both In and Out (`slave { pcm "hw:N,…" … }` under `type plug`).  
Snippet file: **`~/.asoundrc.mscc-virtual`** (not only `~/.asoundrc`).

**Verify on Pi:**

```bash
grep -E 'format|rate|channels' ~/.asoundrc.mscc-virtual
arecord -D MSCC_Cable_In -f S16_LE -r 96000 -c 2 -d 1 /dev/null && echo OK
aplay -L | grep -i MSCC
```

**Do not use** for MSCC digi: raw `hw:MSCCLoop`, `plughw:`, `dmix`, `default` — they skip the fixed 96k plug.

**mscc-init / inis:**

| Setting | Value |
|---------|--------|
| Digital mic (trans) | `MSCC_Cable_In` |
| Digital speaker (recv) | `MSCC_Cable_Out` |
| Operator mic/speaker | Real hardware (not the cable) |
| I/Q | Proficio/Multus only |

### D + TUNE no RF (root cause + fix)

**Symptom:** P + TUNE has RF; switch to **D** (or TUNE on D) → **power drops**; PortAudio often still reports open OK.

**Causes stacked:**

1. Digital device not truly **96k** under ALSA (old `audio-setup` without rate/format; USB digi at 48k) → open fail or bad duplex.
2. **Idle loopback full duplex:** capture on `MSCC_Cable_In` with nothing playing into `MSCC_Cable_Out` can **stall ALSA** so the PortAudio callback never runs → **no I/Q**, even though TUNE synthesizes the carrier and does not need mic samples.
3. Channel count: app must open digi as **2ch** to match cable + radio (not mono from inflated ALSA max-channel reports like 128).

**App fix (`SDRcore-trans-linux` — ship rebuilt binary):**

| Mode | DIGITAL path |
|------|----------------|
| **TUNE / CW** | **Output-only** Proficio I/Q (`manage_stream` with device &lt; 0 — no digi capture) |
| **Voice digi (USB/LSB/AM)** | Full duplex **2ch @ 96k** on `MSCC_Cable_In` |

Also: on mode enter/leave TUNE while DIGITAL, reopen stream (output-only ↔ full duplex).  
TUNE callback still handles NULL mic input when capture is present.

**Log markers (healthy D+TUNE):**

```text
DIGITAL+TUNE/CW: output-only I/Q
# or
MODE T/C + DIGITAL: reopen OUTPUT-ONLY I/Q
… stream_status=0
```

**Bad markers:**

```text
Open Stream Failed: … Invalid sample rate
# or DIGITAL open success but RF gone with no output-only line (old binary)
```

### Packaging pitfalls

| Mistake | Result |
|---------|--------|
| Ship **old** `audio-setup` (bare plug, no rate/format) | Cable wrong; greps on `~/.asoundrc` miss PCMs; 96k fails |
| Build `.deb` **before** updating `mscc-binaries/audio-setup` | Install keeps old script (happened with 1.0.3 timeline) |
| Rebuild script but not run `mscc-audio-setup restart` | Stale `~/.asoundrc.mscc-virtual` |
| Next .deb without new **sdrcore-trans** binary | D+TUNE regression returns |

---

## Packaging rules (agreed)

1. **One `.deb`**, architecture **arm64**.
2. **Apps:** prebuilt only from `mscc-binaries` → install to **`$HOME/mscc` only** (not `/opt`).
3. **Config:** seed from `mscc-init-files-linux` → **`$HOME/.local/mscc`** only if folder **missing or empty**; **never overwrite** existing `.ini`.
4. **User:** real login user (`SUDO_USER`), not root as runtime identity.
5. **System-wide links:** `/usr/local/bin` → `mscc`, `mscc-init`, `mscc-audio-setup`.
6. **tty0tty:** build on Pi with `make` + `make install` in packaged module sources; **no DKMS for now** (kernel bumps deferred).
7. **Virtual audio:** install **must** run loopback setup (digital apps need it).
8. **Postinst prompts** to run **`mscc-init`** before first use; must use **absolute path** to binary (runuser PATH lacks `/usr/local/bin`).
9. **Platform:** Raspberry Pi OS only; ports to other distros are unsupported DIY.
10. **Depends (runtime):** `libportaudio2`, `libusb-1.0-0`, `build-essential`, `kmod`, `udev`, `passwd`, **`alsa-utils`**, `libc6`.  
    **Recommends:** `linux-headers-rpi-v8 | linux-headers-rpi-2712`.

---

## What postinst does (order)

1. Resolve non-root user + `$HOME`.
2. Copy binaries → `$HOME/mscc`, chmod, chown.
3. Seed config if missing/empty.
4. Symlinks in `/usr/local/bin`.
5. Add user to **dialout**.
6. **Virtual audio:**  
   - `/etc/modprobe.d/mscc-aloop.conf` (snd-aloop options)  
   - `/etc/modules-load.d/mscc-aloop.conf`  
   - `modprobe snd-aloop` as root  
   - `audio-setup start` as user → `MSCC_Cable_*` in `~/.asoundrc`
7. **tty0tty:** `make` + `make install` if headers present.
8. Banner + optional interactive **`mscc-init`** (Y/n, default Y).

---

## Version history (deb)

| Ver | Notes |
|-----|--------|
| 1.0.0 | First install: binaries, config seed, tty0tty, links |
| 1.0.1 | Prompt for mscc-init (PATH bug: bare `mscc-init` failed under runuser) |
| 1.0.2 | Absolute path for mscc-init |
| **1.0.3** | **audio-setup / snd-aloop / MSCC_Cable_*** wired into postinst; alsa-utils Depends |
| **1.0.4** | postinst runs **`$HOME/mscc/audio-setup restart`** (installed copy); audio-setup **2ch S16_LE 96k**; verify asoundrc; build checks S16_LE marker. |
| **1.0.5** | Same installer + **updated arm64 binaries** from `mscc-binaries` (incl. larger `sdrcore-trans` 287480). |

Rebuild on Linux/WSL (Windows home mounts break DEBIAN mode 777): stage under `/tmp`, then `dpkg-deb --build`. See successful pattern in chat / use `build-deb.sh` adjusted for WSL, or the inline WSL stage used for 1.0.x builds.

---

## Operational commands (Pi)

```bash
# Install
sudo apt install -y ./mscc_1.0.3_arm64.deb
# or: ./install-mscc.sh ./mscc_1.0.3_arm64.deb

mscc-init              # required once (devices/CAT)
mscc start|stop|status|restart
mscc-audio-setup start|status|stop

# Checks
ls /dev/tnt*
aplay -L | grep -i MSCC
mscc status
```

### Ports

| Service | Port |
|---------|-----:|
| ms-sdr | 8888 |
| MSCC client | 8889 |
| sdrcore-recv | 9000 |
| sdrcore-trans | 9200 |

### CAT / PTT (tty0tty)

- Typical: **ms-sdr** on `/dev/tnt0`, digi app on `/dev/tnt1`
- **PIN=1** = CTS (digi asserts RTS on other end)
- Seed `comm-port.ini` already has `/dev/tnt0` + `PIN=1`

### Related app fixes already done in trees

- **mscc-init-linux:** lists `tnt*`, prompts PIN (default 1 for tnt), writes real `PIN=`
- **ms-sdr pin check:** needs `PIN≠0`; case PIN=2 has fDCD bug (uses fDCD never filled; DCD is in fRLSD) — known, separate from packaging
- **SDRcore-trans-linux:**
  - TUNE ignores mic content (`gain = 0`); NULL `inputBuffer` still runs `tune_modulate` for TUNE/CW
  - Digital mic index bounds / seed when digital not found
  - Digital open **2ch @ 96k**; P/D switch with ALSA settle
  - **DIGITAL + TUNE/CW → output-only I/Q** (verified on Pi: RF holds when switching P→D in TUNE)
- **audio-setup:** fixed **S16_LE / 2ch / 96k** plug (do not regress to old bare-plug script)

---

## Open / later

| Item | Notes |
|------|--------|
| **Next .deb (1.0.4?)** | Include new `sdrcore-trans` + current `audio-setup`; bump version; retest D+TUNE |
| **`_apt` unsandboxed notice** | When `.deb` is under `$HOME`, apt warns; install still works. `install-mscc.sh` stages to `/tmp`. Circle back to polish UX. |
| **Kernel upgrades + tty0tty** | One-shot `make install` breaks after new kernel reboot until rebuild. DKMS deferred. |
| **Word/PDF card** | Update `INSTALL-FOR-PI.docx` if digi cable / 96k notes should appear for operators |
| **Multi-user** | Links embed one user’s `$HOME`; single-operator Pi assumed. |
| **Init_Proficio_table log OOB** | After `mode` loop, log used `mode[5]` (garbage on band 11); log clamped to last valid mode — keep if still present |

---

## Success criteria (verified on Pi)

- [x] `apt install` package  
- [x] Config seed when `~/.local/mscc` missing  
- [x] tty0tty builds and loads  
- [x] mscc-init runs (absolute path)  
- [x] mscc start works  
- [x] audio-setup wired in **1.0.3** (test after upgrade)  
- [x] `MSCC_Cable_In` @ 96k/S16/2ch (`arecord` OK)  
- [x] **D + TUNE RF** with rebuilt sdrcore-trans (output-only path)

---

## Quick resume checklist for Grok

1. Read this file (especially **Digital audio / D + TUNE**).  
2. Latest deb: `mscc-deb/mscc_1.0.3_arm64.deb` — **does not yet** include D+TUNE binary fix; source fix is in `SDRcore-trans-linux`.  
3. Payload sources: `mscc-binaries`, `mscc-init-files-linux`, `tty0tty-master/module`.  
4. Maintainer scripts: `mscc-deb/packaging/DEBIAN/{control,postinst,prerm,postrm}`.  
5. Pi OS only; `$HOME/mscc` + `$HOME/.local/mscc`; digital needs **MSCC_Cable_*** (96k) + **tnt*** for typical digi/PTT.  
6. Never re-port audio from Windows without re-applying Linux 96k + D+TUNE output-only behavior.
