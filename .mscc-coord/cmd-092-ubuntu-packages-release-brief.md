# cmd-092: Ubuntu (amd64) packages to release status (stew-HP)

**STOP notes (read first)**
- Work on **stew-HP only**. Do not touch `rpi/`, `Solidus/`, `installers/rpi/`, the WPF tree, or `mscc-ui/Avalonia-Migration`. Reading `rpi/` is fine; this job copies FROM `rpi/` INTO `linux/`.
- No GitHub release and no `gh` in this job. Stew publishes from Shack in the next step (cmd-093).
- Do not commit any `.deb`.
- Pull first: `git pull --rebase --autostash`. Report what came in. If Ron changed `rpi/*-linux`, `rpi/mscc-deb/build-deb.sh` or `rpi/mscc-deb/packaging/DEBIAN/control` since `74b21e1b`, stop and tell Stew.

## Why
The release must carry all five installers (repo-root `CLAUDE.md`, "Finished" step 5). The Ubuntu servers deb (`mscc_1.0.91_amd64.deb`, built Nov 13 in `f09daede`) is behind the Pi. `linux-build/build-mscc-deb-amd64.sh` takes its version from `rpi/mscc-deb/packaging/DEBIAN/control` (now **1.0.102**). It builds the servers from `linux/*-linux`, and the package files and `mscc-status-report` come from `rpi/mscc-deb`. If we built it today, the deb would say 1.0.102 but contain the older Ubuntu servers (ms-sdr 3.210, recv 3.156, trans 3.160). So the `linux/` trees get ported first.

The Pi server commits that are not in `linux/` yet are `f3fa796d` (R11-18-1), `c9c22429` (R11-18-3), `e6df98a9` (R11-19-0) and `fb60810e` (R11-19-1). `diff -ru linux/<tree> rpi/<tree>` shows only those changes. There are no Ubuntu-only lines in these files, and the Makefiles and everything outside `source(s)/` are identical.

## Steps

1. **Port the Pi servers into `linux/` (copy these 17 files from `rpi/` to the same path under `linux/`):**
   - `ms-sdr-linux/source/`: `amplifier.c`, `comm-port.c`, `extern.h`, `main.c`, `power_calibration.c`, `proficio-temperature.c`, `swr_wifi_meter.c`, `usbavrcmd.h`, `version.h` (3.215)
   - `SDRcore-recv-linux/sources/`: `extern.h` (3.157), `main.c`
   - `SDRcore-trans-linux/sources/`: `amplifier.c`, `commands.h`, `extern.h` (3.163), `main.c`, `power.c`, `udp_thread.c`

   Afterwards, `diff -rq rpi/<tree> linux/<tree> -x '*.o'` must print nothing for all three trees.
   This brings over: the SWR-fault transmit stop, the CAT S-meter `SM0;`, QRO calibration kept in the Black Pill (3.24+), the "RECEIVE FIXED ITSELF" message, cal files written once per calibration, and the `MSCC-VERSION` tag.

2. **`linux/helpers/mscc-status-report`:** replace it with `rpi/mscc-deb/packaging/usr/share/mscc/bin/mscc-status-report`. The Pi copy is a strict superset: the Versions block, the operator device match and `_fold`. This copy is only used by `linux-build/mscc-linux.sh helpers`. The deb already ships the Pi copy.

3. **Build the servers deb:** `./linux-build/build-mscc-deb-amd64.sh` -> `installers/linux/mscc_1.0.102_amd64.deb`.
   - The log must show `ms-sdr version (not bumped): 3.215` and `arch: x86-64 OK`.
   - `strings` on the three servers inside the deb must show `MSCC-VERSION ms-sdr 3.215`, `sdrcore-recv 3.157` and `sdrcore-trans 3.163`.
   - Move `installers/linux/mscc_1.0.91_amd64.deb` to `installers/linux/old/`.

4. **File-by-file check against the Pi deb:**
   - Download `mscc_1.0.102_arm64.deb` from https://github.com/w4mmp-ron/mscc-station/releases/tag/R11-19-1 with `curl -L -O`.
   - Unpack both with `dpkg-deb -R`. The file lists must be the same apart from CPU-specific files: the three servers, `psoc-bootloader`, PortAudio libs and the control file.
   - md5 every non-ELF file. Scripts, firmware, `mscc-status-report` and desktop files must match.
   - Also compare against `mscc_1.0.91_amd64.deb`. Report any file added or dropped.

5. **Avalonia amd64:** reuse `Avalonia/mscc-avalonia_1.7.08_amd64.deb` (built 2026-11-19 12:44 from `74b21e1b`, installed and SA-checked by Stew). Don't rebuild it. Run `dpkg-deb -f` on it and confirm Package mscc-avalonia, Version 1.7.08, Architecture amd64. Copy it to `installers/linux/` and leave the 1.6.96 copy in `installers/linux/old/`.
   - Do not build the Pi arm64 mscc-avalonia. That is Ron's call (see the note in step 8).

6. **Install and smoke test on stew-HP:**
   - `sudo apt install ./installers/linux/mscc_1.0.102_amd64.deb`
   - `mscc status`: the first section is Versions. It should show mscc 1.0.102, mscc-avalonia 1.7.08, ms-sdr 3.215, recv 3.157, trans 3.163 and the firmware list. The GitHub line will say "not attached to R11-19-1" for the amd64 packages. That is expected before the release.
   - Stew's SA checks:
     - Connect in MSCC Avalonia.
     - WSJT-X TUNE and FT8 are still clean (cmd-090 guard).
     - A CAT S-meter reading shows in WSJT-X or a logger.
     - TX and RX work on the Proficio with Black Pill 3.24.
     - QRO cal still loads.
   - Then `sudo apt remove mscc-ui`. The old 0.6.75 client is not replaced by `mscc-avalonia` and only causes confusion.

7. **Docs (Ubuntu only):**
   - `installers/linux/INSTALL.md`: rewrite for two files.
     - `sudo apt install ./mscc_1.0.102_amd64.deb` (servers, PortAudio, MSCC Init and Volume, firmware and flash tools; it replaces the old `mscc-portaudio`, `mscc-firmware` and `mscc-init-gui`).
     - Then `sudo apt install ./mscc-avalonia_1.7.08_amd64.deb`.
     - Optional: `sudo apt remove mscc-ui`.
     - Keep the dialout/audio/plugdev step, First run (MSCC Init, MSCC Start, MSCC Avalonia, Connect 127.0.0.1:8888) and the groups.io link.
     - Say "see the Software Versions Addendum" rather than listing version numbers, except the file names.
   - `installers/linux/CHANGELOG.md`: update the "Currently shipped" table to `mscc` 1.0.102 and `mscc-avalonia` 1.7.08. Mark `mscc-ui`, `mscc-firmware`, `mscc-init-gui` and `mscc-portaudio` as retired (inside `mscc` since 1.0.79, or replaced by `mscc-avalonia`). Add a "mscc 1.0.102 / mscc-avalonia 1.7.08 — 2026-11-19" entry covering:
     - the servers now match the Pi (ms-sdr 3.215, recv 3.157, trans 3.163): SWR-fault transmit stop, CAT S-meter, QRO cal in the Black Pill, version tag
     - the `mscc status` Versions section
     - the Avalonia 1.7.07 and 1.7.08 changes (digital TX guard, Remote Audio fit, connect keeps the server's settings, CAT mode, FW/Core ask, USB defaults)
   - Do not edit the Addendum, `installers/windows/*` or `installers/rpi/*` here. Shack does those in cmd-093.

8. **Note for Ron:** add `.mscc-coord/NOTE-FOR-RON-UBUNTU-1.0.102-AND-AVALONIA-1.7.08-2026-11-19.md`. Word it as facts and options:
   - `linux/*-linux` now matches `rpi/` at `fb60810e`.
   - Ubuntu `mscc` 1.0.102 and `mscc-avalonia` 1.7.08 are built.
   - Stew will publish R11-19-2 with all five installers.
   - The Pi `mscc-avalonia` on the release stays 1.7.04 (unchanged). Options: Ron builds 1.7.08 arm64 (`pwsh Avalonia/build-mscc-avalonia-deb.ps1`, default arm64), or Stew builds it from `Avalonia/` on stew-HP if Ron prefers.
   - The Windows servers are still behind the Pi (ms-sdr 3.214, recv 3.155, trans 3.159) under the Windows hold.

9. **Finished means commit only (Stew pushes), with no installer upload and no GitHub release.** Commit only after Stew's SA checks in step 6 pass.
   - One commit, `git add` of specific paths only:
     - the 17 `linux/` files
     - `linux/helpers/mscc-status-report`
     - `installers/linux/INSTALL.md`
     - `installers/linux/CHANGELOG.md`
     - the Ron note
   - Message: `Ubuntu mscc 1.0.102 (servers in step with Pi R11-19-1) + mscc-avalonia 1.7.08 amd64 ready for R11-19-2`, ending with a "With Ron" list.
   - `git pull --rebase --autostash` before committing. Do not push; Stew pushes.
   - Tell Stew the two file paths and sizes to carry to Shack:
     - `installers/linux/mscc_1.0.102_amd64.deb`
     - `installers/linux/mscc-avalonia_1.7.08_amd64.deb`

## Test checklist (Stew, SA)
- [ ] `mscc status` Versions section correct (versions above)
- [ ] TX and RX on the Proficio (Black Pill 3.24), QRO cal present
- [ ] WSJT-X TUNE + FT8 clean, no bumps (Avalonia 1.7.08)
- [ ] CAT S-meter reads in WSJT-X / logger
- [ ] Remote Audio Mic vol reachable on 1366x768
