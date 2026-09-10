# MSCC on Ubuntu Desktop (x86_64)

**This is not the Raspberry Pi kit.** Pi operators: [`rpi/pi-install/INSTALL.md`](rpi/pi-install/INSTALL.md).

Verified on **Ubuntu 26.04.1**, kernel `7.0.0-31-generic` (`stew-HP-Notebook`).

Ron’s Pi trees are under **`rpi/`** (guide only — do not edit those for this laptop).  
Ubuntu working copy is **`linux/`**. Scripts: [`linux-build/`](linux-build/).

Release drop: [`mscc-ui/Release/avalonia/`](mscc-ui/Release/avalonia/)

| Folder | Use |
|--------|-----|
| `mscc-ui/Release/avalonia/arm64/` | Pi packages only |
| `mscc-ui/Release/avalonia/x86_64/` | Ubuntu UI + init-gui |

Do **not** `apt install` an `*_arm64.deb` on this PC except you may use **`mscc-init-gui_*_all.deb`**. Do **not** install `pulseaudio` (PipeWire is already the audio server). Do **not** copy `$HOME/mscc` binaries into `rpi/mscc-binaries/` (that folder is AArch64 for the Pi `.deb`).

---

## 1. Prerequisites

```bash
sudo apt update
sudo apt install -y \
  build-essential g++ git dpkg-dev \
  libusb-1.0-0-dev \
  libhidapi-dev \
  libhidapi-libusb0 \
  pkg-config \
  libportaudio2 \
  portaudio19-dev \
  pulseaudio-utils \
  python3-tk \
  python3-pyaudio \
  python3-usb \
  linux-headers-generic

sudo usermod -aG dialout,audio "$USER"
```

Log out and back in. `id -nG` must include `dialout`, `audio`, and `plugdev`.

Check: `pactl info` should say **PulseAudio (on PipeWire …)**.

Ubuntu’s `libportaudio2` already includes Pulse — do not install `mscc-portaudio_*_arm64.deb` here.

---

## 2. Build servers (x86_64)

From the repo root:

```bash
./linux-build/mscc-linux.sh all
```

That compiles `ms-sdr`, `sdrcore-recv`, `sdrcore-trans`, `mscc-init`, and `bootloader` into **`$HOME/mscc`**, copies start/stop helpers, and installs user menu entries (Start / Stop / Status / Firmware). Audio link is `PORTAUDIO=distro`.

Details: [`linux-build/README.md`](linux-build/README.md).

Seed config only if `~/.local/mscc` is missing or empty:

```bash
mkdir -p "$HOME/.local/mscc"
cp -a linux/mscc-init-files-linux/. "$HOME/.local/mscc/"
```

Enable digi sinks:

```bash
mkdir -p "$HOME/.config/systemd/user"
cat > "$HOME/.config/systemd/user/mscc-virtual-audio.service" << 'EOF'
[Unit]
Description=MSCC virtual digi audio (Pulse/PipeWire VirtualA/B)
After=pipewire.service pipewire-pulse.service sound.target
Wants=pipewire.service

[Service]
Type=oneshot
ExecStart=%h/mscc/mscc-virtual-audio.sh
RemainAfterExit=yes

[Install]
WantedBy=default.target
EOF
systemctl --user daemon-reload
systemctl --user enable --now mscc-virtual-audio.service
pactl list short sinks | grep Virtual
```

---

## 3. tty0tty, udev, PATH, Init GUI (root)

Skip the Pi AudioInjector `dtoverlay`. Build tty0tty in `/tmp` so the repo stays clean.

```bash
sudo bash -s << 'EOF'
set -euo pipefail
MSCC="$(pwd)"                    # run from the mscc-station clone
USER_MSCC="/home/${SUDO_USER:-$USER}/mscc"

STAGE=/tmp/tty0tty-mscc-$$
cp -a "$MSCC/linux/tty0tty-master/module" "$STAGE"
make -C "$STAGE"
make -C "$STAGE" install
rm -rf "$STAGE"

install -m 644 "$MSCC/linux/udev/99-proficio.rules" \
  /etc/udev/rules.d/99-proficio.rules
udevadm control --reload-rules
udevadm trigger

mkdir -p /usr/local/bin
ln -sfn "$USER_MSCC/mscc.sh"                /usr/local/bin/mscc
ln -sfn "$USER_MSCC/mscc-init"              /usr/local/bin/mscc-init
ln -sfn "$USER_MSCC/bootloader"             /usr/local/bin/bootloader
ln -sfn "$USER_MSCC/bootloader-gui"         /usr/local/bin/bootloader-gui
ln -sfn "$USER_MSCC/mscc-virtual-audio.sh"  /usr/local/bin/mscc-virtual-audio

apt-get install -y "$MSCC/mscc-ui/Release/avalonia/x86_64/mscc-init-gui_1.0.13_all.deb"
EOF
```

`/dev/tnt0` should be `crw-rw---- root dialout`.

---

## 4. Install Avalonia UI (amd64 `.deb`)

```bash
sudo apt install -y ./mscc-ui/Release/avalonia/x86_64/mscc-ui_0.6.44_amd64.deb
```

Menu **MSCC UI**, or `mscc-ui`. Default host **127.0.0.1** port **8888**.

Rebuild later (does not touch the Pi arm64 UI):

```bash
./linux-build/mscc-ui-x64.sh
./linux-build/build-mscc-ui-deb-amd64.sh
sudo apt install -y ./mscc-ui/Release/avalonia/x86_64/mscc-ui_0.6.44_amd64.deb
```

Needs a user-local **.NET 9** SDK at `$HOME/.dotnet` (Ubuntu 26.04 apt has SDK 10; the project stays net9.0).

---

## 5. First run

1. **MSCC Init** (`mscc-init-gui`) — pick **this PC’s** speaker and mic. Digi stays VirtualA / VirtualB.monitor. Pulse or ALSA operator devices both work; Proficio I/Q stays on the radio USB device.
2. **MSCC Start**
3. **MSCC UI** — Connect `127.0.0.1:8888`
4. **MSCC Status** if something fails — logs: `~/.local/mscc/*.log`

Everyday:

```bash
mscc start
mscc status
mscc stop
```

---

## Pi path (unchanged)

On the Pi, still (Ron's `rpi/` tree):

```bash
cd rpi/SDRcore-recv-linux && make clean && make
# … trans, ms-sdr
```

Optional: build Pi binaries **on this laptop** without replacing `$HOME/mscc`:

```bash
./linux-build/cross-arm64.sh prereqs --install
./linux-build/cross-arm64.sh build
./linux-build/cross-arm64.sh stage
./linux-build/cross-arm64.sh deb
```

`rpi/mscc-deb/build-deb.sh` refuses non-AArch64 server ELFs.
