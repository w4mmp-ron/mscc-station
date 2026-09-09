# MSCC on Ubuntu (amd64 laptop)

**Audience:** Ubuntu Desktop **x86_64 / amd64** (this is not the Raspberry Pi kit).  
**Pi operators:** use [`pi-install/INSTALL.md`](pi-install/INSTALL.md) instead.

Verified on **Ubuntu 26.04.1** (`resolute`), kernel `7.0.0-31-generic`, 2026-09-09.

This file is the working install guide. Prerequisites below are done on `stew-HP-Notebook`. Later steps (build servers, init, UI) will be filled in as we do them.

---

## Architecture (read this first)

| | Pi | This Ubuntu laptop |
|--|----|-------------------|
| CPU | **arm64** (AArch64) | **amd64** (x86_64) |
| Shipping server/UI `.deb` in `pi-install/packages/` | Yes | **Will not install** (`Wrong architecture`) |
| Init GUI `mscc-init-gui_*_all.deb` | Yes | Yes (`Architecture: all`) |

**One Linux source tree** (`ms-sdr-linux`, `SDRcore-*-linux`, …).  
**Two package artifacts** when you want both machines: `*_arm64.deb` (build on the Pi) and `*_amd64.deb` (build here).

Do **not** fork Linux sources into amd64 vs arm64 branches.

Do **not** install `pulseaudio` on modern Ubuntu Desktop — **PipeWire** + `pipewire-pulse` is the audio server. Install `pulseaudio-utils` only (for `pactl`).

---

## 1. Prerequisites (done)

### Typical Ubuntu Desktop already has

`python3`, `git`, `alsa-utils`, `usbutils`, `udev`, `kmod`, PipeWire, `pipewire-pulse`, X11/GL libraries, group `plugdev`.

This machine also already had `build-essential`, `g++`, `make`, `dpkg-dev`, and kernel headers for the running kernel.

### Install

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
```

`build-essential`, `g++`, `git`, and `dpkg-dev` are listed so a truly fresh Ubuntu gets them; skip is fine if they are already installed.

| Package | Why |
|---------|-----|
| `libusb-1.0-0-dev` | Compile `ms-sdr`, `mscc-init`, firmware `bootloader` |
| `libhidapi-dev`, `libhidapi-libusb0` | Firmware upload (`-lhidapi-libusb`) |
| `pkg-config` | Build helper |
| `libportaudio2`, `portaudio19-dev` | Recv/trans/init audio. Ubuntu’s PortAudio **includes Pulse**, so the Pi `mscc-portaudio_*_arm64.deb` is not used here |
| `pulseaudio-utils` | `pactl` (VirtualA/B and `mscc status`) |
| `python3-tk`, `python3-pyaudio` | MSCC Init GUI |
| `python3-usb` | USB serial in the init wizard |
| `linux-headers-generic` | tty0tty follows kernel upgrades |

Apt will pull extras (`libasound2-dev`, `libhidapi-hidraw0`, `python3.14-tk`, `pkgconf`, …). That is expected.

### Groups (CAT / tty0tty / audio)

```bash
sudo usermod -aG dialout,audio "$USER"
```

**Log out and back in** (or reboot) before using `/dev/tnt*` or radio serial. New groups do not apply to an already-open session.

`plugdev` is usually already assigned (Proficio USB).

### Check

```bash
pkg-config --modversion libusb-1.0
pkg-config --modversion portaudio-2.0
python3 -c 'import tkinter, pyaudio, usb; print("python OK")'
pactl info | head
id -nG    # after re-login should include: dialout audio plugdev
```

Expect `pactl` to show **PulseAudio (on PipeWire …)**.

### Recorded on this laptop (2026-09-09)

Newly installed (requested + apt extras):  
`libusb-1.0-0-dev`, `libhidapi-dev`, `libhidapi-libusb0`, `libhidapi-hidraw0`, `pkg-config`, `libportaudio2`, `portaudio19-dev`, `pulseaudio-utils`, `python3-tk`, `python3-pyaudio`, `python3-usb`, `linux-headers-generic`, plus `libasound2-dev`, `libjack-jackd2-dev`, `libportaudiocpp0`, `libpulsedsp`, `libtk8.6`, `python3.14-tk`, `pkgconf`.

User `stew` added to **`dialout`** and **`audio`**. Current Grok/terminal session still needs a **re-login** before those groups appear in `id`.

---

## 2. Next (not done yet)

- [ ] Log out / in so `dialout` and `audio` apply
- [ ] Build Linux servers into `~/mscc` (`SDRcore-recv-linux`, `SDRcore-trans-linux`, `ms-sdr-linux`, `mscc-init-linux`, `psoc-usb-bootload-linux`)
- [ ] Seed `~/.local/mscc` and install init GUI (`mscc-init-gui_*_all.deb` is OK on amd64)
- [ ] VirtualA / VirtualB (PipeWire / `mscc-virtual-audio`)
- [ ] tty0tty module
- [ ] Proficio udev rule
- [ ] Avalonia UI as **linux-x64** (.NET — Ubuntu 26.04 repos have SDK **10**, project is **net9.0**; decide at UI step)
- [ ] Optional later: amd64 `.deb` packaging parallel to the Pi arm64 debs

Pi arm64 server/UI packages stay built **on the Pi** (or an arm64 host). This laptop builds **amd64** for itself.
