# For Ron: 2 TONE is in, voice ALC is now 0.89 on Windows, and the 10-30 splatter was a USB sound card

From Stew, 2026-11-01. Windows trans 3.157 and client 11.1.3, built on Shack and tested by Stew
with the Signal Hound (630 m). New installer made.

## 1. 2 TONE: no new opcode, it uses 0x88

`CMD_SET_TWO_TONE` (0x88) already existed, and both ms-sdr trees already pass it from the
client to trans. So 0x2E and 0x2F are still free. On the Pi side, only trans needs a change.

**trans 3.155** (`dsputils.c` `twoToneAudio()`, called from the audio callback in `main.c`):

- With `mystate.twoToneFlag` set **and** the mode USB or LSB, trans replaces the mic samples with
  700 + 1900 Hz, 0.8 each, and a 5 ms raised-cosine fade-in at key-down.
- This happens where `doMicProc()` would run. No speech processor; then the normal path: 6 kHz
  audio LPF, SSB modulator, drive, ALC.
- The level follows the **SSB slider**, not the tune slider.
- The old `twoTone()` branch in the callback is removed.

**Client 11.1.3:**

- 2 TONE button next to TUNE. It arms; TUNE then sends two-tone instead of the carrier.
- It switches the radio to USB if it was in another mode, and disarms on Stop.

## 2. Voice ALC ceiling: Windows now uses your 0.89 (trans 3.157)

Change it in two places: `dsputils.c` `ALC_CEILING`, and `udp_thread.c` `CMD_SET_ALC_MULTIPLIER`,
which set 0.45 again. Digital stays at 1.0.

2 TONE at SSB 100, each tone against the TUN carrier (-31.2 dBm):

| | ALC ceiling | Each tone | IMD3 below the tones |
|---|---|---|---|
| Digital | 1.0 | -7.3 dBc | ~33 dB |
| Phones | 0.45 (old) | -14.6 dBc | ~40 dB |
| Phones | **0.89** | **a little under -8 dBc** | **33.85 dB** |

- About 6 dB more voice peak power. That was your missing SSB power (the ~40 W).
- The ALC settles about 2 dB under its ceiling, so 0.89 puts voice PEP close to the calibrated
  power. IMD stays where the PA is fine.
- So 0.89 it is, on both sides. You were right.

## 3. Why 0.45 happened: a StarTech USB sound card

The 0.45 came from Stew's 10-30 "voice splatter" tests. Today we found the real cause:

- Stew's Phones mic was a cheap StarTech USB sound card. Whenever trans had it open, every
  transmission (TUN too) carried a comb of spurs exactly 1 kHz apart, about 48 dB under the
  carrier, plus about 20 dB more noise close to the carrier.
- trans zeroes the mic in TUN and CW, so the samples were clean. The comb followed the device
  from Phones to Digital, and went away with a virtual cable or the PC's own sound card.
- 1 kHz is the USB audio packet rate. The card is in the trash.

**For the guides:** if the TUN carrier shows a comb or a raised noise floor, set the mic to a
virtual cable and check again. Some USB sound devices put their own noise on the transmission.

## Also

- A 20 ms ALC envelope peak hold (3.156) was tried for the comb first. It changed nothing, so it
  is not in 3.157.
- I saw your `NOTE-FOR-STEW-SWR-FAULT-TO-CLIENT-2026-11-01.md`; I'll look at it next.
