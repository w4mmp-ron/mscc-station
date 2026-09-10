# ms-sdr on Linux / WSL (Debian)

## Build (WSL Debian)

```bash
sudo apt update
sudo apt install -y build-essential libusb-1.0-0-dev usbutils
cd /mnt/c/Users/Ron/.grok/worktrees/ms-sdr-linux
make                    # real USB (needs device attached — see README-USB-WSL.md)
# or handshake only:
make MS_SDR_NO_USB=1
```

## Run

```bash
./ms-sdr
```

- Listens on UDP **8888** (`MS_SDR_PORT` in `source/port_defines.h`).
- Log: `$HOME/ms-sdr.log` (or path from `log_file_dir.ini` if present).
- Override config home: `export MS_SDR_HOME=$HOME/mscc-data`

From Windows client, connect to **`127.0.0.1`** (WSL2 localhost forwarding).

## Files added for Linux

| File | Role |
|------|------|
| `source/platform.h` | Sockets, Sleep, types, serial stubs |
| `source/platform_linux.c` | Implementations |
| `Makefile` | gcc build of vcxproj source set |

## Not yet ported

- Real **USB** Proficio (libusb) — define path with `MS_SDR_NO_USB=0` later
- **Serial** COM / Elecraft — stubs fail open (OK for handshake)
- I2C legacy — not in this build list

## Clean

```bash
make clean
```

## Keyer CQ memory (handoff)

See **`RESUME.md`** in this tree and `worktrees/keyer/KEYER-MEMORY.md`.

- **`0x9C` `CMD_SET_KEYER_MEMORY`:** one UDP/USB param at a time; USB payload **2 bytes** `[param, seq]`; paced after each send.
- **`0x76` `SET_MEM_TEXT_WPM`:** sent to Proficio (needs firmware flash); PIC Farnsworth apply later.
- UDP bench: `keyer/keyer-mem-udp-test.py` (default host `192.168.12.199:8888`).

Windows ms-sdr must mirror the `0x9C` packing and pacing for the same radio firmware.
