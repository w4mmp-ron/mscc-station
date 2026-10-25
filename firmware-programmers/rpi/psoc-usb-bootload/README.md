# psoc-usb-bootload-linux (Raspberry Pi)

Ubuntu laptop copy (edit there, not here): [`../../debian/psoc-usb-bootload/`](../../debian/psoc-usb-bootload/).

**`psoc-bootloader.gui`** is the Python GUI. **`psoc-bootloader`** is the compiled C CLI (`make`; gitignored). They are not the same file. Pi binary: `../../../rpi/mscc-binaries/psoc-bootloader` (AArch64).

Upload Proficio / Omnia **application** firmware (`.cyacd`) from a Pi over USB HID.

Same job as Windows `bootloader.exe`, simplified: **BOOT jumper + one command**.

| | |
|--|--|
| Device mode | BOOT jumper → Morse **LOADER** → HID **`04b4:b71d`** |
| File | Creator **`.cyacd`** |
| Binary | `./psoc-bootloader` |

## Build (Pi)

```bash
sudo apt-get install -y build-essential libhidapi-dev libusb-1.0-0-dev
cd firmware-programmers/rpi/psoc-usb-bootload   # from the mscc-station repo root
make clean && make
```

If link fails: `make LIBS="-lhidapi-hidraw -lusb-1.0"`

## Use

**CLI**

```bash
# 1. BOOT jumper on, power on (LOADER)
# 2. Stop ms-sdr
./psoc-bootloader /path/to/Proficio-MKII-PTT-YYYYMMDD.cyacd
# 3. Power off, remove jumper, power on
```

**GUI** (Windows USBBootloaderHost-style; needs `python3-tk`)

```bash
./psoc-bootloader.gui
# or after mscc install:  psoc-bootloader.gui
```

Load File → `.cyacd`, wait for **Connected** (`04b4:b71d`), Program.

Optional package **`mscc-firmware`** installs shipping `.cyacd`/`.hex` under `/usr/share/mscc/firmware/<RadioName>/`. Load File opens there when that folder exists; otherwise the status log warns and the dialog still works. Browse anywhere. Do not Program unless the BOOT jumper is on (LOADER `04b4:b71d`).

Optional udev (no sudo):

```text
# /etc/udev/rules.d/99-proficio-bootloader.rules
SUBSYSTEM=="usb", ATTR{idVendor}=="04b4", ATTR{idProduct}=="b71d", MODE="0666"
```

`sudo udevadm control --reload-rules && sudo udevadm trigger`

## License

- `cybootloaderutils/*` — Cypress Semiconductor (Creator EULA)
- `src/*`, `Makefile`, this README — project / Omnia-MSCC use
