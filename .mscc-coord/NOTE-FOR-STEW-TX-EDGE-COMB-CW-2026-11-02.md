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
  steps. For example, the firmware shapes the edge in the audio going to the DAC instead of using
  the DAC's mute, or there is a smoother attenuation setting in the PCM3060 that we haven't found.

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
