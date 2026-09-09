# Build MSCC servers on the Raspberry Pi

**Working repo (all changes):** `mscc-station`  
Windows path example: `C:\Users\n8vet\OneDrive\Documents\GitHub\mscc-station`

This tree is **flat** (servers sit next to `mscc-deb`, not under a `Linux-work-tree/` folder).

```text
mscc-station/
  SDRcore-recv-linux/     → sdrcore-recv
  SDRcore-trans-linux/    → sdrcore-trans
  ms-sdr-linux/           → ms-sdr
  mscc-binaries/          ← copy built binaries here before packaging
  mscc-deb/               ← ./build-deb.sh → mscc_<Version>_arm64.deb
  pi-install/packages/    ← drop finished debs here for install kits
```

**Current packaging version:** see `packaging/DEBIAN/control` (`Version:`).  
As of FM work: control is **1.0.42**, but you still need a **Pi rebuild** of recv/trans/ms-sdr (FM + `0x9E`) before that `.deb` exists with the new binaries.

**UI is separate:** Avalonia `mscc-ui_*.deb` (e.g. 0.6.44). Updating the UI does **not** update the servers.

---

## Why on the Pi?

`mscc_*.deb` ships **prebuilt AArch64** binaries. Compile on a **64-bit Raspberry Pi OS** Pi 4/5.

`build-deb.sh` checks that `mscc-binaries/{ms-sdr,sdrcore-recv,sdrcore-trans,mscc-init,bootloader}` are **AArch64** and aborts if they are not (so an Ubuntu laptop amd64 build cannot be packaged as `*_arm64.deb`).

---

## Before you start (once)

1. On the Pi, open the **same** `mscc-station` tree (OneDrive sync, `git clone`/`git pull`, or USB copy).

2. Set the tree root (adjust if your Pi path differs):

   ```bash
   export MSCC="$HOME/OneDrive/Documents/GitHub/mscc-station"
   # other common layouts:
   # export MSCC="$HOME/Documents/GitHub/mscc-station"
   # export MSCC="$HOME/mscc-station"
   ls "$MSCC/ms-sdr-linux/Makefile"
   ls "$MSCC/SDRcore-recv-linux/Makefile"
   ls "$MSCC/SDRcore-trans-linux/Makefile"
   ```

3. PortAudio + build tools:

   ```bash
   # if not already installed:
   sudo apt install -y ./mscc-portaudio_19.8.2_arm64.deb   # from pi-install/packages
   sudo apt update
   sudo apt install -y build-essential g++ libusb-1.0-0-dev libhidapi-libusb0
   ldconfig -p | grep portaudio
   # expect /usr/local/lib/libportaudio.so
   ```

4. Stop servers:

   ```bash
   mscc stop
   ```

---

## Path A — Fast test (no new `.deb`)

Plain `make` (no extra script):

```bash
export MSCC="$HOME/OneDrive/Documents/GitHub/mscc-station"   # ← your real Pi path
mscc stop

cd "$MSCC/SDRcore-recv-linux"
make clean && make
ldd $HOME/mscc/sdrcore-recv | grep portaudio
# MUST show /usr/local/lib/...

cd "$MSCC/SDRcore-trans-linux"
make clean && make
ldd $HOME/mscc/sdrcore-trans | grep portaudio
# MUST show /usr/local/lib/...

cd "$MSCC/ms-sdr-linux"
make clean && make

mscc start
mscc status
```

`make` writes into **`$HOME/mscc/`**. Connect UI **0.6.44+** and test FM / FM Power.

Later installing an **old** `mscc_1.0.41` deb will overwrite these — use Path B when you want a lasting package.

---

## Path B — Package `mscc_1.0.42` (or next version)

### 1) Build (same as Path A)

### 2) Copy into `mscc-binaries/`

```bash
cp -a "$HOME/mscc/sdrcore-recv"  "$MSCC/mscc-binaries/"
cp -a "$HOME/mscc/sdrcore-trans" "$MSCC/mscc-binaries/"
cp -a "$HOME/mscc/ms-sdr"        "$MSCC/mscc-binaries/"

ls -la "$MSCC/mscc-binaries/"{ms-sdr,sdrcore-recv,sdrcore-trans}
file "$MSCC/mscc-binaries/ms-sdr"
# expect: ARM aarch64
```

### 3) Confirm version

```bash
grep '^Version:' "$MSCC/mscc-deb/packaging/DEBIAN/control"
# expect: Version: 1.0.42   (or bump if that deb already exists)
```

### 4) Build and install the `.deb` **on the Pi**

```bash
cd "$MSCC/mscc-deb"
chmod +x build-deb.sh install-mscc.sh
./build-deb.sh
# → ./mscc_1.0.42_arm64.deb

./install-mscc.sh ./mscc_1.0.42_arm64.deb
# or: cp ./mscc_1.0.42_arm64.deb /tmp/ && sudo apt install -y /tmp/mscc_1.0.42_arm64.deb

mscc start
mscc status
```

### 5) Optional: refresh the install kit

```bash
cp -a "$MSCC/mscc-deb/mscc_1.0.42_arm64.deb" "$MSCC/pi-install/packages/"
```

---

## What you do **not** need for this server refresh

| Skip | Why |
|------|-----|
| Rebuild Avalonia / `mscc-ui` | Already updated (e.g. 0.6.44) |
| Rebuild `mscc-portaudio` | Unchanged |
| Rebuild `mscc-init-gui` | Unchanged |
| Rebuild `mscc-init` | Only if you changed `mscc-init-linux` |

---

## Smoke checks

```bash
mscc status
ldd $HOME/mscc/sdrcore-recv | grep portaudio
ldd $HOME/mscc/sdrcore-trans | grep portaudio
ls -la $HOME/mscc/{ms-sdr,sdrcore-recv,sdrcore-trans}
```

In the UI: connect → **FM** → FM Power → TX/RX as you verified on Windows.

Logs if needed: `~/sdrcore-recv.log`, `~/sdrcore-trans.log`, `~/ms-sdr.log`, or `$HOME/mscc/logs/`.

---

## Common mistakes

| Mistake | Result |
|---------|--------|
| Building from **MSCC-Grok-Build** instead of **mscc-station** | Wrong / old sources |
| Building only on Windows | No arm64 servers for the Pi |
| Forgetting `mscc stop` | “Text file busy” / weird crashes |
| `ldd` without `/usr/local` PortAudio | Digi Virtual* often broken |
| Leaving old binaries in `mscc-binaries/` then running `build-deb.sh` | New version number, **old** FM-less binaries inside |
| Installing `mscc_1.0.41` after a Path A FM build | Overwrites your new `~/mscc/*` |

---

## One-page cheat sheet

```bash
export MSCC="$HOME/OneDrive/Documents/GitHub/mscc-station"  # ← fix path
mscc stop

cd "$MSCC/SDRcore-recv-linux"  && make clean && make
cd "$MSCC/SDRcore-trans-linux" && make clean && make
cd "$MSCC/ms-sdr-linux"        && make clean && make

# test now:
mscc start && mscc status

# or package 1.0.42:
cp -a $HOME/mscc/{sdrcore-recv,sdrcore-trans,ms-sdr} "$MSCC/mscc-binaries/"
cd "$MSCC/mscc-deb" && ./build-deb.sh
./install-mscc.sh ./mscc_1.0.42_arm64.deb
mscc start && mscc status
```

Operator install of finished packages: **[../pi-install/INSTALL.md](../pi-install/INSTALL.md)** and **[INSTALL-FOR-PI.md](INSTALL-FOR-PI.md)**.

Optional: an Ubuntu **x86 laptop** can cross-compile these same trees to AArch64 (`linux-build/cross-arm64.sh`) without changing this Pi `make` path. That output must never replace `$HOME/mscc` on the laptop (that dir is the x86 station).
