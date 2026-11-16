# For Ron: firmware folders — only the latest images in the Pi package

From Stew, 2026-11-16. This is a note only; we are not editing `rpi/` or `installers/rpi/` from here (your area). We are also not touching `radio-psoc-firmware/Proficio-Legacy - Copy/`; please see the request below.

Stew wants the Raspberry Pi / Linux packages to ship **only the current** PSoC and keyer firmware, the same set that is going into the next Windows installer. Dated and earlier versioned images are being removed from the repo. The old hand-off folder `radio-psoc-firmware/release/` (dated `YYYYMMDD` drops) is being deleted; it was already retired as an archive on 2026-11-02.

## Current images (tested, except Geminus Legacy)

| Radio | File prefix |
|---|---|
| Proficio MKII-PTT | `Proficio-MKII-PTT-3.246` |
| Ultimus MKII-PTT | `Ultimus-MKII-PTT-8.246` |
| Proficio MKII-ATU | `Proficio-MKII-ATU-4.247` |
| Ultimus MKII-ATU | `Ultimus-MKII-ATU-7.247` |
| Geminus MKII | `Geminus-MKII-2.156` |
| Proficio Legacy | `Proficio-Legacy-1.237` |
| Ultimus Legacy | `Ultimus-Legacy-6.237` |
| Geminus Legacy | `Geminus-Legacy-5.154` (built, not hardware-tested; no radio) |

Keyer PIC (daughter board), current:

- `Keyer-MCU-app-20261112.hex` — app, for `keyer-flash` over USB
- `Keyer-MCU-full-20261112.hex` — bootloader + app, for PICkit once

Please still note in the Pi instructions: **power-cycle the radio after a keyer PIC update** (no RF until then). That was confirmed on all MKII radios.

These images are in commit `dce2d93a` (`cmd-080/080b: Multus IQ USB name + 0x9D Farnsworth forwarding (MKII)`), which also put only those eight pairs into `mscc-ui/installer/windows/payload/PSoC Firmware\` and already has the two keyer hex files in `payload/Keyer Firmware\`. Stew will push when he is ready. A follow-up commit (`cmd-084`, not an amend of `dce2d93a`) removes the dated / older tree images — including the intermediate test images that were in `dce2d93a` (2.155, 3.245/8.245, 4.246/7.246) — and the top-level `radio-psoc-firmware/release/` folder, moves unversioned `Radio.cyacd` / `.hex` into `radio-psoc-firmware/original/`, and leaves only `Keyer-MCU-app-20261112.hex` and `Keyer-MCU-full-20261112.hex` in `keyer-buffer/Release/`.

## Please delete `radio-psoc-firmware/Proficio-Legacy - Copy/`

Stew asks you to remove this folder from git:

```
git rm -r "radio-psoc-firmware/Proficio-Legacy - Copy"
```

It is a full tracked snapshot (~630 files) of the old Proficio-Legacy tree, including unversioned `Release/Proficio-Legacy.cyacd` / `.hex` and `Proficio-Legacy-20261026.*`. It is not one of the eight live firmware trees. Stew says **it was a working folder for testing new firmware** and should be deleted, and that it is yours to delete. We left it untouched in cmd-084.

## What this means for the Pi `mscc` package

`rpi/mscc-deb/build-deb.sh` already does the right thing. It copies **one versioned** `<Radio>-<major>.<minor>.cyacd` / `.hex` per radio from the Windows payload `PSoC Firmware\<folder>\` into `/usr/share/mscc/firmware/<Radio>/`, and the current `Keyer-MCU-app` / `-full` pair into `/usr/share/mscc/firmware/keyer/`. There are **no** firmware `.cyacd` / `.hex` files stored under `rpi/mscc-deb/packaging/` (the package picks them up at build time). The bootloader GUI already opens `/usr/share/mscc/firmware`.

So you should not need a path change in `rpi/`. After you pull the commits, please **rebuild the `mscc` deb** so a fresh install gets only the versions in the table, and so `/usr/share/mscc/firmware/<Radio>/` does not keep an older pair. The last `mscc_*.deb` sitting in `rpi/mscc-deb/` on this clone is `1.0.77` (2026-11-13), which would still have the previous firmware if it was built before these images.

No change is requested to Black Pill packaging in this batch.

## The separate Linux `mscc-firmware` deb (not the Pi `mscc` package)

`linux-build/build-mscc-firmware-deb.sh` still packed **exactly one dated** `*-YYYYMMDD` pair per radio from the retired `radio-psoc-firmware/release/<Radio>/` folder into `/usr/share/mscc/firmware/<Radio>/`. That source folder is going away, and those dated files are not the current release. cmd-084 updates that packager to use the latest versioned files in each tree's `Release/` (same idea as `stage.ps1` and your `build-deb.sh`). We are not building a new `mscc-firmware_*.deb` in this step.

## Paths (after the cleanup)

| What | Where |
|---|---|
| Shipping PSoC images in git | `radio-psoc-firmware/<Tree>/Release/` (Geminus: `release/`), only the current `<Radio>-<major>.<minor>` pair |
| Unversioned snapshots | `radio-psoc-firmware/original/` (not for shipping) |
| Retired dated drop folder | `radio-psoc-firmware/release/` — removed |
| Windows installer staging (Stew) | `C:\mscc-net9\PSoC Firmware\<Radio>\` and `C:\mscc-net9\Keyer Firmware\` |
| Windows payload in git | `mscc-ui/installer/windows/payload/PSoC Firmware\` and `...\Keyer Firmware\` (already current) |
| Pi install location | `/usr/share/mscc/firmware/<Radio>/` and `/usr/share/mscc/firmware/keyer/` |
