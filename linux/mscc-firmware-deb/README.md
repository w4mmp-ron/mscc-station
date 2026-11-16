# mscc-firmware (packaged files)

Prebuilt PSoC **application** firmware for USB Bootloader upload. Built on **Shack** (Keil / PSoC Creator). This package redistributes those artifacts only.

## Packaged files

The `mscc-firmware` deb ships the **newest** versioned `.cyacd` and `.hex` per radio:

```text
/usr/share/mscc/firmware/<RadioName>/<Radio>-<major>.<minor>.cyacd
/usr/share/mscc/firmware/<RadioName>/<Radio>-<major>.<minor>.hex
```

The packager (`linux-build/build-mscc-firmware-deb.sh`) reads that pair from each tree's `Release/` (Geminus trees: `release/`). It accepts either directory name and does not require a dated `YYYYMMDD` filename. After the 2026-11-16 cleanup each tree has only the current pair, so that pair is the one packed. Unversioned `Radio.cyacd` / `.hex` snapshots live in `radio-psoc-firmware/original/` and are not packaged. The old `radio-psoc-firmware/release/` drop folder is gone.

**USB Bootloader Load File** uses the `.cyacd`. The `.hex` is the matching application image from the same dated build. Do not Program unless the BOOT jumper is on (LOADER `04b4:b71d`).

Radios: Proficio-Legacy, Proficio-MKII-PTT, Proficio-MKII-ATU, Geminus-Legacy, Geminus-MKII, Ultimus-Legacy, Ultimus-MKII-ATU, Ultimus-MKII-PTT.
