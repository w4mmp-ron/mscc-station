# For Stew: the 12 kHz comb, and what it means for CW

From Ron, 2026-11-02. Reply to `NOTE-FOR-RON-TX-EDGE-COMB-DAC-SOFT-MUTE-2026-11-02.md`.

Thanks for the measurements and the ATU timer fix. Before I pick option 1 or 2, my question is:
**how does this affect CW?**

## What I see from your note

- In CW the comb comes at every key-up, so once per dot and once per dash, not just once at the
  end of a transmission. At 25 wpm that is many bursts a second.
- Option 2 (trans fades out first) can't help CW. The radio keys CW by itself and trans sends a
  steady carrier the whole time. trans doesn't know when each dot or dash ends, so it can't fade
  out ahead of it.
- QSK takes the comb away, but then the PA switches with RF up, on every dot and dash. That is
  worse.
- So for CW the only real fix would be in the radio itself: a fade that doesn't come in 12 kHz
  steps.
- A raised cosine made by stepping the PCM3060 volume won't work. It would still be a staircase
  of 0.5 dB steps, just slower, so the comb moves closer to the carrier instead of going away. It
  also takes too many I2C writes to the PCM3060, and the I2C delays can't be tolerated in the main
  loop.

## My idea: the PSoC makes its own I/Q in CW TX

The only real fix I see for the PSoC is for the radio to make the CW I/Q itself, with a
raised-cosine fade, and not use the DAC soft mute at all. From reading the code (not tried):

- In CW, trans sends the carrier all the time, even while receiving (the radio keys it). It is
  a 12 kHz tone at 96 kHz, so exactly 8 samples per cycle. One 1 ms USB packet (96 I/Q pairs,
  16-bit, 384 bytes) is exactly 12 cycles, so every packet starts at the same point of the wave.
- That carrier already has the drive level, power cal and TX I/Q cal in it. The PSoC doesn't
  need to work any of that out.
- **Grab:** while receiving in CW, the 8051 copies one packet out of `TxI2S[n]` (pcm3060.c) into
  its own 384-byte buffer, after the USB DMA fills it and before the zero DMA clears it. It
  takes a new copy every receive period, so band, power or cal changes are picked up.
- **Play:** the hook is already there: `PCM3060_SetTxBufAddress()` points the DAC DMA at any
  buffer for the current millisecond. In CW TX, each ms it points at:
  - a fade-up buffer at key-down (stored carrier x raised-cosine gain, ~5 ms),
  - the stored carrier while keyed,
  - a fade-down buffer at key-up,
  - zeros after that.
- The fade buffers are built on the fly: 192 multiplies per ms, well inside the 8051's time.
  Only 1-2 extra 384-byte buffers, no big tables.
- The DAC stays unmuted in CW, so there is no 12 kHz staircase and no comb, and the PA still
  goes on and off with no drive on it.
- The same path could give QSK clean edges too.

Things to check:

1. trans really sends a steady, unbroken 12 kHz carrier during CW receive, on Windows and on
   the Pi (on the Pi the 12 kHz CW case is under `#ifdef RON` in dsputils.c).
2. The copy and the multiplies fit in each ms alongside USB and the keyer.
3. The fade lines up with the key delay (the fade-up has to start early enough that dot and dash
   lengths stay the same, as 3.240 does now).

## Questions for you

1. In CW the carrier is already there at key-down, so the fade-in should step over a full carrier
   too. Why does the comb show only at key-up? If key-down really is clean, that may tell us how to
   fix key-up.
2. On the air, are the CW bursts a problem you can hear or see on another receiver, or only on the
   analyzer?
3. If CW can't be fixed by trans, is it worth doing option 2 just for TUN, 2 TONE and SSB / DIG?
   Or should we look for one fix in the radio that covers everything, CW included?

## Other points

- Pi trans output latency: not measured yet. We can measure it if we go with option 2.
- The beep in Phones at the end of 2 TONE: your test (keep recv muted about 50 ms longer after TX
  off) sounds right. We can try it on the Pi too.
- The DAC kept muted in RX (3.243 / 4.244): fine, leave it in.
