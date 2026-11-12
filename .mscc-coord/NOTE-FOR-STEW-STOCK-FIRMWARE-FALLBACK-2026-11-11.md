# For Stew: keep the stock radio firmware, as a fallback

From Ron, 2026-11-11. This one needs you: Ron doesn't know where all the original firmware files are.

## Why

All our new radio firmware stores the calibration in the radio and has the new CW. If a new
version ever causes trouble on a radio, the user needs a way back to the firmware that radio had
before our changes. Right now we only keep our own builds, so there is nothing to fall back to.

## What we need

For every radio, the last stock firmware from before our changes (the `.cyacd` file, and the
`.hex` too if you have it):

- Proficio Legacy, Proficio MKII PTT, Proficio MKII ATU
- Geminus Legacy, Geminus MKII
- Ultimus Legacy, Ultimus MKII PTT, Ultimus MKII ATU (if they ever had their own stock files)
- The Black Pill board, if a stock version of it was ever handed out

## Where to keep them

`radio-psoc-firmware/original/<Radio>/`, next to our own releases, one folder per radio. The
folder is kept out of git for now (the repo is public; tell Ron if these files may be shared).
Once you know what a file is, put the radio, the version and the date in its name, for example
`Proficio-MKII-PTT-original-3.130-2025-06-01.cyacd`, and update the README in that folder
(where each file came from, which radios it is for).

## Found so far (Ron's PC, 2026-11-11), please confirm

Three old files turned up in `C:\mscc-net9\PSoC Firmware`, in folders the installer no longer
uses. They are now copied, unchanged, to `radio-psoc-firmware/original/` (details and MD5 in
its README). That folder is not in git, so **Ron is sending you the files directly** as
`original-firmware-for-Stew-2026-11-11.zip` (the folder with its README):

- `Proficio/Firmware-122-01` (.cyacd + .hex), dated 2026-08-02: never in the repo, older than it.
  Looks like Proficio version 1.22. Is it stock, and for which Proficio (Legacy or MKII)?
- `Proficio-MKII-ATU/Proficio-MKII-ATU`, dated 2026-07-22: never in the repo either. Stock?
- `Geminus-MKII/Geminus`, dated 2026-07-22: the build that came with your original Geminus MKII
  project (commit 000b1fc). Is it the same as the stock Geminus MKII firmware?

Still missing: Proficio Legacy or MKII PTT (whichever Firmware-122-01 isn't), Geminus Legacy,
and all three Ultimus.

## Things to keep in mind

- An upload with the USB bootloader never touches the radio's EEPROM, so the calibration stays.
  But stock firmware doesn't know about the calibration store, so MSCC will show "RADIO FIRMWARE
  UPDATE REQUIRED" and use the calibration files on the PC.
- The bootloader can't read the firmware back out of a radio. So a radio that still has its stock
  firmware is the only way to get a copy if a file is missing: with a MiniProg3 and PSoC
  Programmer, read the chip to a `.hex` (our builds have read protection off; check that the stock
  ones do too).
- Once the files are in place, we can put them in the installers in their own "stock" folder, so
  users have them too. Ron will decide that.
