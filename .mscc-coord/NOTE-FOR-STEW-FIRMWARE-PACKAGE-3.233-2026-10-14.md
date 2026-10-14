# For Stew — mscc-firmware package: new Proficio-MKII-PTT image (3.233)

From Ron, 2026-10-14. When it suits you.

Ron's firmware 3.233 (entering CW drops TX, see
`NOTE-FOR-STEW-FIRMWARE-CW-FIX-2026-10-14.md`) is built in PSoC Creator and tested on his rig.
The `mscc-firmware` package still ships the 20260822 Proficio-MKII-PTT image.

Ron left the package to you, since it is yours (`linux-build/build-mscc-firmware-deb.sh`).

## What would change

- `radio-psoc-firmware/release/Proficio-MKII-PTT/`: replace
  `Proficio-MKII-PTT-20260822.cyacd` / `.hex` with
  `Proficio-MKII-PTT-20261014.cyacd` / `.hex` from
  `radio-psoc-firmware/Proficio-MKII-PTT/Release/` (built 2026-10-14 12:31, 3.233).
- Package version 1.0.1 -> 1.0.2 (or as you like). Only the PTT image changes.

## Also

`installers/rpi` still has `mscc-firmware_1.0.0_all.deb`; `installers/linux` has 1.0.1.
If you build 1.0.2, `MSCC_FW_DROP_RPI=1` would put it in `installers/rpi` too.
