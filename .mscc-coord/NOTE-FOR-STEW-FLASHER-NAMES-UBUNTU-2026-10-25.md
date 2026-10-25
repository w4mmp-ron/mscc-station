# Note for Stew — flasher names changed; Ubuntu kit not updated (2026-10-25)

From Ron. For your information; the Ubuntu side is yours to handle when it suits you.

## What changed (Ron, 2026-10-25)

All radio firmware flashers now live in `firmware-programmers/` (by host: `rpi/`, `debian/`, `windows/`)
and have new names:

| Was | Now |
|---|---|
| `bootloader` (PSoC CLI) | `psoc-bootloader` |
| `bootloader-gui` (PSoC GUI) | `psoc-bootloader.gui` |
| `proficio-flash-app` (Black Pill, Pi, USB) | `stm32-app-flash` |
| `proficio-flash-bootloader` (Black Pill, Pi, ST-Link) | `stm32-bootloader-flash` |
| `ProficioDfuDownloader.exe` (Black Pill, Windows) | `stm32-app-flash.exe` |
| `flash-bootloader.ps1` / `.cmd` (Black Pill, Windows) | `stm32-bootloader-flash.ps1` / `.cmd` |

The PSoC source folders moved: `rpi/psoc-usb-bootload-linux/` -> `firmware-programmers/rpi/psoc-usb-bootload/`,
`linux/psoc-usb-bootload-linux/` -> `firmware-programmers/debian/psoc-usb-bootload/`.

## Pi: done

mscc 1.0.71 (`installers/rpi/`) ships the new names and also carries the Black Pill flash tools (the separate
`proficio-flash-tools` package is gone). Pi install guides updated.

## Ubuntu: NOT updated (Ron's choice)

Already changed in the repo for the new names (commits c58f7d5, 17e7f27):
- `linux-build/mscc-linux.sh`, `cross-arm64.sh`, `build-mscc-deb-amd64.sh`: build from the new folders and
  expect `psoc-bootloader` / `psoc-bootloader.gui`.
- `linux/mscc-deb/packaging` (postinst, prerm, desktop entry, control), `linux/helpers/psoc-bootloader.gui`,
  `linux-build/desktop/mscc-bootloader.desktop.in`.
- Text in `installers/linux/INSTALL.md` and `INSTALL-UBUNTU.md` (the burner names only).

Not done:
- The Ubuntu .deb was not rebuilt. `installers/linux/mscc_1.0.53_amd64.deb` still installs `bootloader` /
  `bootloader-gui`, so for now the Ubuntu docs name commands that kit does not have yet.
- `build-mscc-deb-amd64.sh` stops until `psoc-bootloader` has been built into `$HOME/mscc`
  (`./linux-build/mscc-linux.sh all` does that).
- The rest of the Ubuntu install docs (versions, package names) was not reviewed.

Options, your call: rebuild the amd64 .deb with the new names next time you build one; or, if you'd rather keep
the old names on Ubuntu, the linux-build / packaging changes above can be put back.
