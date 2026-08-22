# psoc-usb-bootload (Linux)

Linux **USB HID** bootloader host for Multus / Proficio PSoC — same job as Windows  
`mscc-net9/utilities/bootloader.exe` (USBBootloaderHost), **not** a UART tool.

| | |
|--|--|
| App USB (running firmware) | **VID `0x16C0` PID `0x05DC`** — `--enter-bootloader` / `--reboot-app` |
| Bootloader HID | **VID `0x04B4` PID `0xB71D`** — program `.cyacd` |
| File | Creator **`.cyacd`** (from `copy-release.bat`) |
| Protocol | Cypress `cybootloaderutils` + **hidapi** + **libusb** (app cmds) |

## Build (Pi / Debian / Ubuntu)

```bash
sudo apt-get update
sudo apt-get install -y build-essential libhidapi-dev libusb-1.0-0-dev
cd ~/psoc-usb-bootload-linux
make clean && make
# binary: ./bootloader
```

If link fails: `make LIBS="-lhidapi-hidraw -lusb-1.0"`

## Use

```bash
./bootloader --help
```

Examples:

```bash
# Soft-reboot running app (CMD 0x0F)
./bootloader --reboot-app

# Enter bootloader via USB (CMD 0x0E) — needs app firmware that implements 0x0E
./bootloader --enter-bootloader

# List HID devices (expect 04b4:b71d after BOOT / -e)
./bootloader --list

# Program .cyacd (device already in bootloader)
./bootloader /path/to/Proficio-....cyacd

# Enter bootloader, wait 3s, then program
./bootloader -e -w 3 /path/to/Proficio-....cyacd
```

BOOT jumper still works without `--enter-bootloader`.

Optional udev (no sudo for USB):

```text
# /etc/udev/rules.d/99-proficio-bootloader.rules
SUBSYSTEM=="usb", ATTR{idVendor}=="04b4", ATTR{idProduct}=="b71d", MODE="0666"
SUBSYSTEM=="usb", ATTR{idVendor}=="16c0", ATTR{idProduct}=="05dc", MODE="0666"
```

Then `sudo udevadm control --reload-rules && sudo udevadm trigger`.

## Relation to Windows tool

Your Windows `bootloader.exe` is a C# **USBBootloaderHost** using `CyUSB` / HID + `Bootloader_Utils.dll`.  
This Linux tool uses Creator `cybootloaderutils` with **hidapi**, plus **libusb** for app vendor commands `0x0E` / `0x0F`.

## License

- `cybootloaderutils/*` — Cypress Semiconductor (Creator EULA; see `cybootloaderutils/readme.txt`)
- `src/*`, `Makefile`, this README — project / Omnia-MSCC use
