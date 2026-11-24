# Black Pill 3.24: Quisk and HDSDR don't work (SoftRock emulation bugs)

STOP: Don't press "Factory default" in HDSDR's ExtIO_Si570 on a Multus radio. On Multus, 0x33/0x3A are the PPM frequency cal, not a PE0FKO crystal write.
STOP: These are firmware fixes, so they're Ron's call. Nothing has been edited. MSCC must stay on its current SI5351_DLL plain-Hz path.

Date: 2026-11-24. Found by Stew and Build Commander on Shack (pill fw 3.24, Proficio MKII-PTT).

## Symptoms
- HDSDR (ExtIO_Si570.dll, PE0FKO V2.6) shows: "Xtal 0.000000 not in range (low=112, high=117MHz). Try to factory default the device!"
- Quisk (Multus/SoftRock hardware file) finds the radio but doesn't work.
- MSCC works on Windows and the Pi.
- The ExtIO registry shows it found the pill: VID 0x16C0 / PID 0x05DC, manufacturer "Multus", product "Multus Control", serial "MKII-PTT-F411". So this is not a string-match failure.

## Bugs (pill vs PSoC)

### 1. CMD_GET_XTAL (0x3D) returns 0. This is the HDSDR error.
- Pill `usb_vendor.c:119-122`: sets `E_dll_version = SI570_DLL`, then returns `s_result = 0` (4 bytes).
- PSoC `usbvend.c:80-84` returns `Si570_Xtal`, which is deliberately faked in `si5351.c:238-239` ("Fake Si570_Xtal so ExtIO_Si570 does not complain") as 56.32 * 112 / 56.0 * 2^24 = 112.64 MHz.
- The 0x3D "NOOP" comment in the PSoC header is stale. The PSoC really answers it.
- **Fix:** return `0x70A3D70A` (112.64 MHz, 2^24 fixed point) in native little-endian. Don't add `swap32`; the PSoC only needs it because the 8051 is big-endian.

### 2. CMD_SET_FREQ (0x32) is always treated as Hz. This is the Quisk failure.
- Quisk `hardware_usb.py:218-224` (`SetFreqByValue`) sends `freq_Hz / 1e6 * 2^21 * 4` (the SoftRock 11.21 format, 4x LO). For 14.08 MHz that's about 118,111,601.
- PSoC `si5351.c:155-164` switches on `E_dll_version`. For SI570_DLL (0, ExtIO/Quisk) it does `convert_from_host()` and divides by 4 (`convert.c:34-39`). For SI5351_DLL (1, MSCC) it uses raw Hz.
- Pill `usb_vendor.c:357-358` copies the raw payload into `E_current_LO_freq`, and `app.c` programs the Si5351 with it as Hz. So Quisk's 14.08 MHz request tunes the radio to about 118 MHz.
- Default differs: the pill starts as SI5351_DLL (`radio_state.c:26`, `usb_vendor.c:58`); the PSoC starts as SI570_DLL (`usbvend.c:28`). MSCC sets SI5351_DLL explicitly with DLL opcode 0xA0.
- **Fix:** honor `E_dll_version` on 0x32 SET_FREQ and 0x3A GET_FREQ as the PSoC does: SI570_DLL means 11.21 fixed point of 4x LO, SI5351_DLL means Hz, native little-endian. Consider matching the PSoC default (SI570_DLL) so programs that never send 0xA0 get the SoftRock behavior.

### 3. CMD_GET_STARTUP (0x3C) is byte-reversed.
- Pill `usb_vendor.c:114-117` hard-codes `0x713D0A07`, the PSoC's pre-swapped constant (`basic-plus.h:272` STARTUP_LO).
- On ARM that goes out on the wire backwards. The correct wire value is `0x070A3D71` (56.32 MHz in 11.21 format, which is 14.08 MHz RF after /4).
- Same kind of bug as the temperature endian fix in 3.10.
- **Fix:** return `0x070A3D71`.

### 4. Minor (only if ExtIO still complains after 1 to 3)
- 0x3F CMD_GET_SI570: the PSoC returns 6 bytes (`usbvend.c:86-89`, likely zeros on the Si5351 path). The pill has no handler, so the request stalls.
- 0x30 CMD_SET_FREQ_REG: the PSoC accepts a 6-byte register write. The pill doesn't implement it. Quisk doesn't use it (`si570_direct_control = False`).

## After the fix (Stew)
- HDSDR: re-pick the Multus I/Q sound devices (HDSDR stores them by index, and the index changed).
- Quisk: the "Proficio Pill" profile already uses `Microphone (7- Multus IQ Sound)` and `Speakers (7- Multus IQ Sound)` at 96000.
- Close MSCC and the servers before starting Quisk or HDSDR.

## Related
- See `QUESTIONS-FOR-RON-blackpill-usb-audio.md`: the pill should report Line terminals and the PSoC strings ("www.multus-sdr.com", "Multus SDR Control") so Windows names match the PSoC. That isn't the cause of this ExtIO error, but it's still wanted.
