# For Stew: answers to your TX edges note (firmware 3.242, servers 3.148)

From Ron, 2026-10-29. Answers to the five questions in
`NOTE-FOR-RON-TX-EDGES-FW-3.242-SERVERS-3.148-SSB-SPLATTER-2026-10-29.md`.

## 1. ALC change (`doALC()`)

Yes, go ahead. There is no reason for `fabs(I)`; it is old code.

I made the change on the Pi trans (3.145) and it is running there now. I have not
tested it on the air yet and have not committed it. It does what you proposed:

- level from `sqrtf(I*I + Q*Q)` instead of `fabs(I)`
- peak hold falling 40 dB in 50 ms (was 1 ms)
- gain moves toward target / peak with a 3 ms attack (one pole), instead of
  0.4 dB cut per sample
- 3 s release, target, gain limits (0.1 to 1.0), ALC meter and AM / LSB / USB only
  are unchanged

If you see overshoot on sudden peaks, a short look-ahead (about 3 ms delay on the
audio) would fix it. I left that out for now.

I have no analyzer, so the splatter check is yours. Your test of lowering PWR until
the ALC never acts would confirm the cause first.

## 2. Trans ramp at TX off for QSK amps

No trans ramp. Trans gets TX off at about the same time the radio drops the PA, and
its last audio is still in the USB buffers, so it cannot finish a ramp before the PA
goes off. Put the release mute in the firmware QSK path too, the same as your
non-QSK release: DAC mute, wait for the ramp, then AMP and PA off. Only TUN / PTT
release changes; CW QSK keying stays as it is.

## 3. Port order for 3.241 / 3.242

Legacy first. People are asking how to use those radios.

Start from `radio-psoc-firmware/Proficio-Legacy - Copy` (Legacy 1.235, commit dc67bdc,
on your list to test), not the old Legacy tree. Have Build or Code examine it first.
It already has:

- DIN reopened after CW (SSB had no TX I/Q after CW until a power cycle)
- non-QSK key delay, AMP -> delay -> PA as MKII 3.234
- CW key-click shaping with the DAC soft mute (`cw_dac_mute()`)
- entering CW drops PTT / TUNE TX, as MKII 3.233
- calibration store (0xC8) from MKII 3.240

Legacy keys CW with DIN, not the PA line, and will never have a keyer or QSK. What is
left to add from 3.241 / 3.242 is the DAC mute before PA off on TUN / PTT release and
the non-CW mode change no longer dropping TX.

After Legacy: Ultimus MKII PTT, then the ATU lines, then Geminus MKII.

## 4. RIT retune during the ramp

Fix it. On the pill I changed it so the LO goes back to the RX frequency (with RIT)
only after the TX state machine is idle, not when `TX_Request` drops. Tested on my
rig: TUN, PTT, CW, and TUN with RIT +500 Hz; receive came back on the RIT frequency.

For the PSoC: in `si5351a.c`, retune to the RIT frequency only when `TX_Idle()` is true
(your 3.242 function), not on `!TX_Request`.

## 5. LSB / USB image on the Pi with IQ_PHASE

I cannot answer it, I have no analyzer. The Pi trans has the phase correction
(IQ_PHASE, per band, `iq.c`) and my `iq.ini` has non-zero values on most bands. Port
IQ_PHASE to Windows and check LSB on your analyzer; that answers it either way.

## Also

- Pi recv already reopens the Digital output after CW TX off (`Reopen_Current_Output`
  in `udp_thread.c`), so your recv 3.148 change is not needed on the Pi.
- The pill (firmware 3.6) now ramps the TX I/Q with a 5 ms raised cosine in all modes:
  up after PA on, down before PA off, QSK path included. A non-CW mode change no
  longer drops TX, and entering CW ramps down first. So both TUN edges are handled in
  the pill and the Pi trans has no TUNE ramp. Your Windows trans ramp does no harm
  with the pill.
