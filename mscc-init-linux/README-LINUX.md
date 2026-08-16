# mscc-init (Linux)

**Linux-only tree.** Windows build: `mscc-init\` (unchanged).

Friendly interactive wizard that writes MSCC-MKII base config under
**`$HOME/.local/mscc/`** (same directory ms-sdr / sdrcore use).

## What it does

Guided steps with **numbered menus**, **Enter = default**, and a final summary:

1. **Transceiver USB** (optional) — Multus/Proficio serial via libusb  
2. **Keyer** — yes/no → `cw.ini`, plus `mscc.ini` / `i2c.ini`  
3. **CAT + PTT pin** → `comm-port.ini`  
   - Numbered list: PTY, `/dev/ttyUSB*`, `ttyACM*`, `ttyS*`, **`tnt*`** (tty0tty)  
   - Optional custom path override  
   - **PIN:** 0 = off, 1 = CTS (default for `tnt`), 2 = DCD  
4. **Operator audio only** — headphones/speaker + mic (PortAudio)  
   - **Digi is fixed** (not selectable):  
     - `digital-speaker.ini` → **VirtualA**  
     - `digital-microphone.ini` → **VirtualB.monitor**  
   - Virtual* / monitors are hidden from the operator pick lists  

Digi devices are created by **`mscc-virtual-audio`** at install/login.

## Build

Same PortAudio link style as **sdrcore-recv** / **sdrcore-trans**:
default **`/usr/local`** + rpath (Pulse+ALSA MSCC build).

```bash
# once: PortAudio with Pulse → /usr/local (see portaudio/BUILD-MSCC.txt)
sudo apt-get install -y build-essential libusb-1.0-0-dev
cd /path/to/mscc-init-linux
make clean && make
ldd $HOME/mscc/mscc-init | grep portaudio   # expect /usr/local/lib
# copy into package:  cp $HOME/mscc/mscc-init …/mscc-binaries/
```

Override: `make PORTAUDIO_PREFIX=/opt/mscc-portaudio`

## Typical tty0tty PTT setup

| Role | Device | Notes |
|------|--------|--------|
| ms-sdr CAT | `/dev/tnt0` | PIN=**1** (CTS) |
| Digi app | `/dev/tnt1` | Assert **RTS** for PTT |

## Digi vs operator

| Role | Config | Who sets it |
|------|--------|-------------|
| Operator speaker/mic | `operator-*.ini` | **mscc-init** menu |
| Digi speaker | `digital-speaker.ini` = VirtualA | Fixed by mscc-init / seed |
| Digi mic | `digital-microphone.ini` = VirtualB.monitor | Fixed by mscc-init / seed |
| WSJT In/Out | Pulse UI | VirtualA.monitor / VirtualB |

## Notes

- Multus **I/Q** appears in lists with a “not operator phones” hint — do not pick for AF.  
- Multus **identity** = libusb control once at init.  
- Without radio attached, config still works (serial `UNKNOWN`).  
- USB open may need `sudo` or a udev rule for `16c0:05dc`.  
- Restart the MSCC stack after running so servers reload config.
