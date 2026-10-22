# For Stew — radio firmware 3.239 / 3.240: clear removed, CW key click shaping

From Ron, 2026-10-22. Release binaries in `radio-psoc-firmware/Proficio-MKII-PTT/Release/`
(`Proficio-MKII-PTT-20261022.hex` = 3.240; the 3.238 binary of the same date is replaced).

## 3.239: calibration clear removed

`CMD_CALIBRATION` 0xC8 type 6 (clear) is gone; it now stalls. It was a no-data control
request, the same kind as the old save command that hung the radio's USB in 3.236/3.237.
The firmware no longer has any no-data control request. Black Pill build 9 does the same
(clear removed; its save command, type 4 write, is still there: your call). The 3.238 note
table is updated.

## 3.240: CW key click shaping (non-QSK only)

Your spectrum capture showed wide skirts at every key up and key down; the steady carrier
was clean. Cause: in CW, SDRcore-trans sends a steady carrier and the radio cut it up by
switching the PA hard (about 1 ms rise on your scope). The I/Q goes USB to DAC by DMA, so
the 8051 cannot shape the samples.

3.240 uses the PCM3060 DAC soft mute (register 0x44), the same method as the Legacy 1.232
copy:

- In non-QSK CW the DAC is muted between elements.
- Key down: 14 ms before the delayed key down the PA goes on and the DAC unmutes; the RF
  ramps up and is at -6 dB on the edge, so element lengths are unchanged.
- Key up: the DAC mutes, the RF ramps down, and the PA goes off 18 ms later with no drive.
- The ramp is the chip's own, 6 dB per ms (about 3 ms 10-90 %, was about 1 ms). Not a
  raised cosine, but no hard edges.
- The key delay ISR only asks for the mute or unmute; the main loop does the I2C write (two
  per element).
- QSK is unchanged (no key delay to ramp ahead in; DAC at full). Leaving CW unmutes the DAC.
- Spaces shorter than 14 ms merge (about 80 WPM and up).

## Tested by Ron on his MKII

Non-QSK CW, TX hold 310 ms (key delay 20 ms): RX audio normal between overs, no recv /
trans errors or underruns, no USB errors or resets.

## Please check

1. The same spectrum capture (key up and key down) on 3.240: the skirts should be much
   narrower.
2. If you can, a scope shot of one key-down and one key-up edge at 1 ms/div (10-90 % rise
   and fall).
3. If 6 dB/ms is not enough, the next step is stepping the DAC volume register from the
   main loop for a raised-cosine ramp of about 5 ms.
