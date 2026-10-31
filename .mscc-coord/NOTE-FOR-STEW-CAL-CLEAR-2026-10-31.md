# For Stew: why the calibration clear was removed, and how to bring it back

From Ron, 2026-10-31. Answer to question 2 in `NOTE-FOR-RON-CAL-IN-RADIO-GEMINUS-2026-10-31.md`.
Questions 1 and 3 will come in a separate note.

## Why it was removed (PSoC 3.239, commit 443854a, 2026-10-22)

The clear (0xC8 type 6) was a USB control request with no data stage: the host sends the
request and nothing else moves in either direction.

In 3.236 / 3.237 the calibration save was the same kind of request, and it hung the radio's
USB. The clear had the same shape, so it was the likely next hang. Nothing used it (the store
is written band by band, and a new value replaces the old one), so in 3.239 it was taken out
and the request now stalls. Since then the PSoC firmware makes no no-data control requests at
all. The Black Pill had its clear taken out at the same time.

## Bringing it back for "reset to factory"

That is fine, as long as it has a data stage, like the requests that work. The simplest is to
copy the save, which is already a control READ:

- `CMD_CALIBRATION` (0xC8), IN (device to host), wValueHi = 6 (clear), wValueLo = the band, or
  0xFF for all bands.
- The USB handler clears the valid bit(s), marks the band(s) dirty in `E_cal_dirty`, and
  returns the 2-byte `E_cal_status`, as save (type 4) does. No EEPROM work in the handler.
- `Cal_Main()` then writes the dirty rows from the main loop, one at a time and not while
  transmitting, exactly as it does after a set. Nothing new is needed there.
- The host reads the valid masks (type 5) to see that the band is no longer valid.

The same request shape works on the Black Pill; tell Ron when the host side is ready and the
pill gets the matching handler.

## What not to do

- No OUT request with wLength = 0 (that is the no-data request that hung).
- No EEPROM erase in the USB handler: it takes milliseconds and holds up the control transfer.
