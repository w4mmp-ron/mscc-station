# For Stew: 2 TONE ported to the Pi, and 0x2E RESET is in the Pi ms-sdr

From Ron / Claude, 2026-11-01. Reply to your `NOTE-FOR-RON-2TONE-ALC-089-STARTECH-2026-11-01.md`
and `NOTE-FOR-RON-SWR-FAULT-CLIENT-AND-RESET-2026-11-01.md`. Thanks for the 0.89 measurements and
the StarTech find.

## 1. Pi trans 3.151: 2 TONE (port of your 3.155)

- `dsputils.c` `twoToneAudio()`: the same as yours (700 + 1900 Hz, 0.8 each, 5 ms fade-in).
- `main.c` `process_mic_to_iq()`: with `twoToneFlag` set and USB/LSB, the tones replace the mic
  audio after the input branch, then take the voice path (LPF, SSB, drive, ALC). `doMicProc` is
  skipped. The old `twoTone()` branch is removed.
- One difference: on the Pi it also runs with no mic open (`in == NULL`), so 2 TONE works
  without a sound device.
- The Pi has one ALC ceiling (0.89) for voice and digital, so nothing else changed.
- Built on the Pi. Not yet tried on the air.

## 2. Pi ms-sdr 3.189: 0x2E `CMD_SWR_METER_RESET` (your proposal, as written)

- Client → ms-sdr, data ignored. ms-sdr sends the HTTP reset to the meter: the learned IP, else
  `METER_IP`.
- It runs in its own short thread. With the meter gone the HTTP call can take about 8 s, and
  the client UDP thread must not wait.
- An explicit RESET is sent even with `HTTP_RESET=0`; that setting is for the automatic reset only.
- A second press while one is running is logged and dropped.
- No reply opcode. The result is in the ms-sdr log. The meter's next update (0x0F) tells the
  client whether the fault cleared.
- Your nit is fixed: the comments say 3.188 now.
- Windows ms-sdr: please ignore or log 0x2E (no meter code there).
- Client side: yours, as you proposed (send 0x2E when the meter state came from 0x0F; keep the
  direct HTTP reset for the direct path).
- Built on the Pi. Not tested (needs client 11.1.5+ with the 0x2E send).

## Files

- trans: `sources/dsputils.c`, `dsputils.h`, `main.c`, `extern.h` (151)
- ms-sdr: `source/swr_wifi_meter.c`, `main-controller.c`, `extern.h`, `usbavrcmd.h`, `version.h` (189)
