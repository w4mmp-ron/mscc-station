# Pi / Linux flash tools (Black Pill)

Run on the Raspberry Pi (or any Linux PC). Both scripts take the image file as argument (no
default, no images in the package since 1.0.5): copy the image from the STM32 repo's release folder
(`psoc-replacement-stm32/proficio-stm32f411-25MHz/release/`) to the Pi.

| Script | Burns | How | Needs |
|---|---|---|---|
| `flash-bootloader.sh <file>` | `boot-app-firmware-async.bin` (bootloader + app, new board in one step) or `bootload-PA8.bin` @ `0x08000000` | ST-Link (SWD) | `sudo apt install stlink-tools` |
| `flash-proficio.sh <file>` | `app-firmware-async.bin` @ `0x08008000` (app only) | USB DFU, no ST-Link; puts the pill into DFU itself (mscc stopped) | `sudo apt install dfu-util python3-usb` |

Same jobs as the Windows tools in `../../windows/stm32-flash/` (`flash-bootloader.ps1` and `ProficioDfuDownloader.exe`).

## Install as a package (Pi)

```bash
sudo apt install ./proficio-flash-tools_1.0.5_all.deb          # pulls in stlink-tools, dfu-util, python3-usb
sudo proficio-flash-bootloader ~/boot-app-firmware-async.bin   # ST-Link, new board
mscc stop; sudo proficio-flash-app ~/app-firmware-async.bin    # USB, pill stays in the rig
```

The package carries the scripts only (`/usr/share/proficio-flash-tools/`), no firmware: a
new firmware build needs no new package. Build it with `bash build-deb.sh <version>` (WSL
Debian or the Pi) when the scripts change.

## Use from the repo

```bash
cd firmware-programmers/rpi/stm32-flash      # from the mscc-station repo root
REL=../../../psoc-replacement-stm32/proficio-stm32f411-25MHz/release
sudo ./flash-bootloader.sh $REL/boot-app-firmware-async.bin   # ST-Link wired to SWDIO/SWCLK/GND/3.3V
sudo ./flash-proficio.sh $REL/app-firmware-async.bin          # mscc stopped; sends the pill to DFU (USB 0xFE)
```

`sudo` is only needed if your user can't open the USB device (no udev rule).
After a bootloader burn on a bare pill, the LED shows **C** (no codec), which is correct.

`flash-proficio.sh` (1.0.4+): radio running -> it sends USB command 0xFE and waits for the DFU
device; pill already in DFU -> it flashes and warns about the BOOT jumper. "Error during download
get_status" after the flash is normal (the pill resets at once). At the end it waits for the
radio and prints its firmware version, or says the pill is still in the bootloader (LED B):
remove the BOOT jumper (PA8) and press NRST (reset); no power cycle needed.
