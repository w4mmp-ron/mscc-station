# firmware-programmers

Flashers / burners for the radio firmware, by host.

| Folder | What | Burns |
|---|---|---|
| `rpi/psoc-usb-bootload/` | PSoC USB bootloader for the Pi: `make` → `psoc-bootloader` (C CLI, gitignored) + `psoc-bootloader.gui`; built for the Pi .deb by `linux-build/cross-arm64.sh` (moved from `rpi/psoc-usb-bootload-linux/` 2026-10-25) | PSoC radios |
| `rpi/stm32-flash/` | `stm32-app-flash.sh` (USB DFU, app), `stm32-bootloader-flash.sh` (ST-Link); packaged in **mscc** since 1.0.69 (`rpi/mscc-deb/build-deb.sh` takes them from here). Old separate package: `build-deb.sh`, `proficio-flash-tools_1.0.5_all.deb` | Black Pill (STM32F411) |
| `windows/stm32-flash/` | `stm32-app-flash.exe` (USB DFU, app) + `dfu-util.exe`; `stm32-bootloader-flash.ps1` / `.cmd` (ST-Link) | Black Pill (STM32F411) |
| `windows/stm32-dfu-downloader/` | Source of `stm32-app-flash.exe` (`dotnet publish` writes to `../stm32-flash/`) | |
| `windows/psoc-programmer/` | Shortcut to Cypress **PSoC Programmer 3.25.0** (needs it installed; MiniProg). Was one identical copy in each `radio-psoc-firmware/` radio folder (7), moved / removed 2026-10-25 | PSoC radios |
| `debian/psoc-usb-bootload/` | PSoC USB bootloader for Ubuntu x86_64 (same tool, own copy); built by `linux-build/mscc-linux.sh` (moved from `linux/psoc-usb-bootload-linux/` 2026-10-25) | PSoC radios |

Firmware images are not here: they are in the STM32 repo,
`psoc-replacement-stm32/proficio-stm32f411-25MHz/release/`.

Moved here 2026-10-25 from `psoc-replacement-stm32/proficio-stm32f411-25MHz/` (`rpi-tools/`,
`Win-downloader/`, and the tools in `release/`); their earlier history is in that repo.

Not moved (still in place):
- GUI copies that go into the mscc .debs: `linux/helpers/psoc-bootloader.gui`, `rpi/mscc-deb/packaging/`.
- Built Pi binary for the mscc .deb: `rpi/mscc-binaries/psoc-bootloader`.
- `installers/`.
- The 8 MHz pill folder (`psoc-replacement-stm32/proficio-stm32f411-8MHz/`) keeps its own copies.
