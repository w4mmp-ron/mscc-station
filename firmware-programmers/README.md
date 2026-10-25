# firmware-programmers

Flashers / burners for the radio firmware, by host.

| Folder | What | Burns |
|---|---|---|
| `rpi/psoc-usb-bootload/` | PSoC USB bootloader for the Pi: `make` → `bootloader` (C CLI, gitignored) + `bootloader-gui.py`; built for the Pi .deb by `linux-build/cross-arm64.sh` (moved from `rpi/psoc-usb-bootload-linux/` 2026-10-25) | PSoC radios |
| `rpi/stm32-flash/` | `flash-proficio.sh` (USB DFU, app), `flash-bootloader.sh` (ST-Link), `build-deb.sh`, `proficio-flash-tools_*.deb` | Black Pill (STM32F411) |
| `windows/stm32-flash/` | `ProficioDfuDownloader.exe` (USB DFU, app) + `dfu-util.exe`; `flash-bootloader.ps1` / `.cmd` (ST-Link) | Black Pill (STM32F411) |
| `windows/stm32-dfu-downloader/` | Source of `ProficioDfuDownloader.exe` (`dotnet publish` writes to `../stm32-flash/`) | |
| `windows/psoc-programmer/` | Shortcut to Cypress **PSoC Programmer 3.25.0** (needs it installed; MiniProg). Was one identical copy in each `radio-psoc-firmware/` radio folder (7), moved / removed 2026-10-25 | PSoC radios |
| `debian/psoc-usb-bootload/` | PSoC USB bootloader for Ubuntu x86_64 (same tool, own copy); built by `linux-build/mscc-linux.sh` (moved from `linux/psoc-usb-bootload-linux/` 2026-10-25) | PSoC radios |

Firmware images are not here: they are in the STM32 repo,
`psoc-replacement-stm32/proficio-stm32f411-25MHz/release/`.

Moved here 2026-10-25 from `psoc-replacement-stm32/proficio-stm32f411-25MHz/` (`rpi-tools/`,
`Win-downloader/`, and the tools in `release/`); their earlier history is in that repo.

Not moved (still in place):
- GUI copies that go into the mscc .debs: `linux/helpers/bootloader-gui`, `rpi/mscc-deb/packaging/`.
- Built Pi binary for the mscc .deb: `rpi/mscc-binaries/bootloader`.
- `installers/`.
- The 8 MHz pill folder (`psoc-replacement-stm32/proficio-stm32f411-8MHz/`) keeps its own copies.
