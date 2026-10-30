# For Ron: SSB TX results on Windows (trans/recv 3.149, firmware 3.242)

From Stew, 2026-10-30. Thanks for the answers in `NOTE-FOR-STEW-ANSWERS-TX-EDGES-2026-10-29.md`.
Everything below was measured on the Signal Hound with a Proficio MKII PTT (FW 3.242),
Windows servers, WPF client. SSB and AM phone are now clean; Stew accepted all three fixes.

## 1. Broadband splatter on SSB voice: recv stopped its audio stream at PTT

Not the ALC and not the PA. Windows recv, `CMD_SET_TX_ON` (non-CW, MONITOR off), called
`Pa_StopStream` at TX on and `Pa_StartStream` at TX off. recv's RX input is the same radio
USB audio device trans sends TX I/Q to, so stopping it stalled trans' output callback: two
gaps of 75-140 ms with output underflow at about 0.15 s and 0.6 s after PTT, every over.
Speech chopped there = broadband click. Clue: MONITOR on (no stop) was clean, and it only
splattered when talking right after PTT.

**Fix, recv 3.149:** keep the stream running; while `G_tx_mode && !G_Monitor && G_mode != 'C'`
the callback outputs silence (and silence to remote phones) and skips DSP/AGC. CW tone path
unchanged. **Please check the Pi recv:** if it also stops the stream at PTT, it has the same
problem (the Pi may hide it if its audio device is separate).

## 2. ALC (trans 3.149): your rework plus a lower ceiling and a new meter

- Same envelope rework as your Pi 3.145 (|I + jQ|, 50 ms peak hold, 2 ms attack, 3 s
  release, `maxval` starts at 0; it was 1.0, which pulled the gain to 0.1 at start-up).
- **Ceiling 0.45 x drive** (`ALC_CEILING` in dsputils.c; CMD_SET_ALC_MULTIPLIER sets 0.45).
  The old 1.1 x drive only acted after the PA was already splattering. Measured: mic 10
  peaks 0.07-0.22 clean; mic ~49 peaks 0.43-0.57 splatter.
- **Voice ALC meter = peak envelope before the ALC / ceiling** (`G_ALC_peak_env`, read and
  reset every 50 ms in alc.c); 100 = ALC just starting to act. The old gain-reduction meter
  read 0 until the signal was already at the limit. Digital keeps the dB meter.
- Stew: mid-to-low yellow = clean; near red = mild splatter. No overshoot seen, so no
  look-ahead.

## 3. High LSB image: no audio low-pass before the SSB mixer (your question 5)

Not I/Q phase. On 40 m LSB, PTT with no modulation, the image was almost as strong as
the wanted side after a good TX I/Q cal; USB was fine, VOUTL/VOUTR equal on the scope.

trans multiplies the mic (real = imag) by the 12 kHz IF LO and then picks the sideband
with a filter. Mic content near 21-24 kHz (hiss; the 48 kHz mic resampled to 96 kHz goes
up to ~23.5 kHz) mixes into the LSB IF passband (9.3-11.9 kHz) from the other side of the
LO and comes out as the image. For USB the matching band is 24-27 kHz, above the mic
content, so USB looked fine. Reproduced in a Python model of the trans chain with
full-band input; a band-limited test signal hides it.

**Fix, trans 3.149:** 8th-order Butterworth 6 kHz low-pass (4 biquads, fs 96 kHz,
`tx_audio_lowpass()` in main.c) on the mic samples just before the modulator, AM/LSB/USB.
1 kHz 0 dB, 21 kHz -100 dB. Result: LSB image gone, USB unchanged, AM cleaner too.

**The Pi trans almost certainly has the same bug** (same framesToComplex -> ssb_modulate ->
fastconv chain). IQ_PHASE can't fix it, which may be why phase never seemed to matter
much on LSB. Suggest adding the same low-pass on the Pi.

## Committed

mscc-station `main`: "trans/recv 3.149 + FW 3.242: clean SSB/AM TX (no splatter, LSB image,
ALC meter)". Release builds, no debug logging.

## Next on Windows (in progress, not committed yet)

- recv 3.150: your AM receive fix and noise blanker fix (c9850ca).
- ms-sdr 3.184: PTT poll `r == 1` (+20 ms, key-status thread off), 0xC8 power cal in the
  radio, CW + TUN stuck TX, 0xFE 0 client leaving, core 0x30 messages forwarded, firmware
  type + NUL fix in the log, and your 10-30 PPM calibration start-point fix.
- IQ_PHASE on Windows: later.
- Firmware: MKII PTT 3.243 (RIT retune on `TX_Idle()`, QSK release mute), then Legacy from
  `Proficio-Legacy - Copy` as you suggested.
