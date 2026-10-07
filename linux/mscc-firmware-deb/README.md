# mscc-firmware (packaged files)

Prebuilt PSoC **application** firmware for USB Bootloader upload. Built on **Shack** (Keil / PSoC Creator). This package redistributes those artifacts only.

## Packaged files

The `mscc-firmware` deb ships **one dated** `.cyacd` and **one dated** `.hex` per radio:

```text
/usr/share/mscc/firmware/<RadioName>/<Name>-YYYYMMDD.cyacd
/usr/share/mscc/firmware/<RadioName>/<Name>-YYYYMMDD.hex
```

The packager picks the (only) dated pair in `radio-psoc-firmware/release/<RadioName>/`. Undated copies in `release/` (same bytes, written by `copy-release.bat`) are **not** packaged.

**USB Bootloader Load File** uses the `.cyacd`. The `.hex` is the matching application image from the same dated build. Do not Program unless the BOOT jumper is on (LOADER `04b4:b71d`).

Radios: Proficio-Legacy, Proficio-MKII-PTT, Proficio-MKII-ATU, Geminus-Legacy, Geminus-MKII, Ultimus-Legacy, Ultimus-MKII-ATU, Ultimus-MKII-PTT.
