# psoc-usb-bootload-linux (Ubuntu x86_64 copy)

Guide original: [`../../rpi/psoc-usb-bootload-linux/`](../../rpi/psoc-usb-bootload-linux/). Edit **this** tree for Ubuntu; do not change `rpi/` for laptop fixes.

`bootloader-gui.py` is the **GUI**. `bootloader` is the **C CLI** (`make`); it is gitignored and CPU-specific (x86-64 here, aarch64 on the Pi).

Upload Proficio / Omnia **application** firmware (`.cyacd`) over USB HID.

Same job as Windows `bootloader.exe`, simplified: **BOOT jumper + one command**.

| | |
|--|--|
| Device mode | BOOT jumper → Morse **LOADER** → HID **`04b4:b71d`** |
| File | Creator **`.cyacd`** |
| Binary | `./bootloader` |

## Build (Ubuntu x86_64)

```bash
sudo apt-get install -y build-essential libhidapi-dev libusb-1.0-0-dev
cd linux/psoc-usb-bootload-linux   # from the mscc-station repo root
make clean && make
```

`./linux-build/mscc-linux.sh all` also builds this CLI and installs it as `$HOME/mscc/bootloader`.

If link fails: `make LIBS="-lhidapi-hidraw -lusb-1.0"`

## Use

**CLI**

```bash
# 1. BOOT jumper on, power on (LOADER)
# 2. Stop ms-sdr
./bootloader /path/to/Proficio-MKII-PTT-YYYYMMDD.cyacd
# 3. Power off, remove jumper, power on
```

**GUI** (Windows USBBootloaderHost-style; needs `python3-tk`)

```bash
./bootloader-gui.py
# or after mscc install:  bootloader-gui
```

Load File → `.cyacd`, wait for **Connected** (`04b4:b71d`), Program.

Optional udev (no sudo):

```text
# /etc/udev/rules.d/99-proficio-bootloader.rules
SUBSYSTEM=="usb", ATTR{idVendor}=="04b4", ATTR{idProduct}=="b71d", MODE="0666"
```

`sudo udevadm control --reload-rules && sudo udevadm trigger`

## License

- `cybootloaderutils/*` — Cypress Semiconductor (Creator EULA)
- `src/*`, `Makefile`, this README — project / Omnia-MSCC use
