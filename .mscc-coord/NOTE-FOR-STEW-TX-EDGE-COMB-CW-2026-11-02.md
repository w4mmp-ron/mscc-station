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
  its own 384-byte buffer (see "Change the DMA chain" below for the timing). It takes a new copy
  every receive period, so band, power or cal changes are picked up.
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

### How the TX audio gets to the DAC today (read from pcm3060.c, audio.c and the netlist)

- USB packet (1 ms) -> `Audio_Main` (main loop, polled) starts the USB DMA into `TxI2S[n]`,
  byte-swapped on the way, so RAM holds big-endian 16-bit I/Q in DAC order. `n` comes from the
  SyncSOF hardware counter; USB always fills the slot after the one playing.
- I2S "FIFO has room" -> **Stage** DMA: 3 TDs per word = stage byte, stage byte, zero byte
  (16-bit sample sent as 24-bit). After each data byte, Stage's done signal starts both:
  - **Buff**: fetches the next byte from `TxI2S[n]` into the stage byte;
  - **Zero**: writes 0 into the same byte of `TxI2S[n]`, so a slot is erased right behind the
    reader.
- `Sync_Main` steers the audio PLL to USB start-of-frame, so nothing slips.

So a USB slot holds good data only for less than 1 ms before it is played and erased. That makes
the grab tight.

### Change the DMA chain, not the Cypress code

The chain is set up in our own code (`DmaTxStart()` in pcm3060.c). The Cypress parts (DMA
hardware, `CyDma...` functions, generated `TxI2S_*_dma.c`) don't need to change; the generated
files are rewritten at every build anyway.

- In CW, point the **Zero** TDs at a dummy byte (no address increment) instead of `TxI2S[n]`.
  The USB data then stays in the slot and the 8051 can copy it any time. Put it back when
  leaving CW.
- Our own buffers (fade-up, carrier, fade-down) can be played with
  `PCM3060_SetTxBufAddress()` (already there, not used today), or added as extra TDs.
- The DMA only moves bytes; it can't scale them. The raised-cosine multiply is still done by the
  8051, into a buffer (192 multiplies per ms).

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
