# Note for Ron: Ubuntu mscc 1.0.102 and Avalonia 1.7.08 (2026-11-19)

Facts from stew-HP. Stew publishes the release from Shack (cmd-093). No GitHub release was made here.

## Facts

- `linux/ms-sdr-linux`, `linux/SDRcore-recv-linux`, and `linux/SDRcore-trans-linux` match `rpi/` at `fb60810e` (R11-19-1). `diff -rq` of each tree is empty apart from `*.o`.
- Ubuntu `mscc` 1.0.102 amd64 is built: `installers/linux/mscc_1.0.102_amd64.deb`. The servers inside it are ms-sdr 3.215, recv 3.157, trans 3.163. Shared files (scripts, firmware, `mscc-status-report`, desktop files) match the Pi `mscc_1.0.102_arm64.deb`. The Pi package also contains a Python cache file and eight extra PortAudio headers; the amd64 package does not.
- Ubuntu `mscc-avalonia` 1.7.08 amd64 is built: `installers/linux/mscc-avalonia_1.7.08_amd64.deb`. It was not rebuilt for this package step. It is the deb from `74b21e1b`.
- Both packages are installed on stew-HP. Stew's SA checks passed on 2026-11-19. The radio session was to the Pi at 192.168.1.193, which reported Core (ms-sdr) 3.211 and FW 8.246. This PC's installed ms-sdr file is 3.215.
- Stew will publish R11-19-2 with all five installers.
- The Pi `mscc-avalonia` on the current release stays 1.7.04.
- The Windows servers are still behind the Pi (ms-sdr 3.214, recv 3.155, trans 3.159) under the Windows hold.

## Options

- Ron builds Pi `mscc-avalonia` 1.7.08 arm64: `pwsh Avalonia/build-mscc-avalonia-deb.ps1` (default arch is arm64).
- Or Stew builds that arm64 deb from `Avalonia/` on stew-HP, if Ron prefers.
