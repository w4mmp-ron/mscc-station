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

## Where to keep them (suggestion)

`radio-psoc-firmware/stock/<Radio>/`, next to our own releases, one folder per radio. Put the
radio, the version and the date in each file name, for example
`Proficio-MKII-PTT-stock-3.130-2025-06-01.cyacd`. Add a short README that says, for each file,
where it came from and which radios it is for.

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
