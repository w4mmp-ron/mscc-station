# For Stew — radio firmware 3.233: entering CW drops TX

From Ron, 2026-10-14. For your information. Follows
`NOTE-FOR-STEW-CW-TUNE-STUCK-TX-2026-10-14.md`.

## What changed

`radio-psoc-firmware/Proficio-MKII-PTT/Proficio-MKII-PTT.cydsn/main.c`, in the CW block at
the top of the main loop: on entering CW the firmware now does the same as it already did
on leaving CW: LED back to normal, PA off, AMP port off, PCM3060 output on, `TX_Request = 0`.
CW keying is unaffected (`cw.c` turns the PA on itself at key-down).

Version bumped to 3.233 (`basic-plus.h`).

## Tested by Ron on his rig (built and flashed)

- USB keyed from the client PTT/MOX, then switched to CW: the rig drops out of TX
  (stayed keyed before).
- CW keying with the key: normal.

## Things to know

- This covers every path into CW (TUN, PTT, CAT, digital programs) on every host, so the
  Windows / Ubuntu ms-sdr change from the earlier note is optional. Your choice whether to
  add it for rigs that are not reflashed.
- Each rig needs the 3.233 firmware to get the fix.
- The Pi ms-sdr workaround stays in place; it does no harm with the new firmware.
