# For Stew — S/W window: BASELINE kept per band

From Ron, 2026-10-12. An idea, client only. You decide whether and when.

## What Ron sees

With BASELINE set for 20 m, the noise sits where he wants it on the spectrum. On 15 m the
noise is higher, so each time he changes to 15 m he moves BASELINE to bring it down, and
moves it back for 20 m. BASELINE is the only S/W setting he touches.

Checked into a dummy load: the noise is almost identical on 20 m and 15 m. So the rig is
the same on both bands, and the extra noise on 15 m comes in through his antenna. It is a
station matter, not a fault, and it will be different for every user.

## The idea

Remember the BASELINE value per band: save it when the user changes it, restore it when the
band changes.

## What is there today (WPF 10.11.3)

- BASELINE is a client display setting. `Controls/SpectrumColorSettings.cs`
  `GetDisplayGridWindow` uses it to raise or lower the bottom of the dB window, 0.3 dB per
  step on a Proficio. The dB labels stay true; only the visible window moves.
- One value is kept per radio family: `SPECTRUM_BASELINE_HF` and `SPECTRUM_BASELINE_LF` in
  `SpectrumWaterfallSettings.cs`. Nothing is kept per band.
- The client already keeps a record per band for frequency, mode and filters
  (`SaveLastUsedForBand`, `MSCC_LastUsed.ini`). BASELINE could be one more field there, or
  its own per-band keys beside the HF / LF ones.

## Things to know

- No server change on any platform.
- A user who never moves BASELINE sees no difference: a band with no saved value keeps
  using the HF / LF value as today.
- The scale stays absolute, so the real difference between bands can still be read on the
  dB labels.
- Ron does not ask for the waterfall LOW / HIGH or the grid to follow per band. He only
  adjusts BASELINE.
- Avalonia was not looked at.
