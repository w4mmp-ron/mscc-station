# For Stew — spectrum: a steady carrier vanishes at some dial settings

From Ron, 2026-10-05. Found with a signal generator into the Proficio (about 14.0672 MHz),
Pi host, WPF 10.4.0. Two causes, one in the server and one in the client. The Pi server half
is fixed and tested; the rest is yours.

## What Ron sees

A steady carrier is strong at most VFO settings, weak at some and gone at others (only its
noise pedestal is left). It stays gone for as long as the dial is left there, so it is not a
tuning transient. It shows only since the full-FFT fix (cmd-064): before that the skirts were
wide enough to hide it.

## Cause 1 — server (`sdrcore-recv`, `dsputils.c`, `doPanadapter`)

Each display point took ONE FFT bin and the bins in between were never looked at:
3.84 bins per point at 800, 1.92 at 1600, 1 at 3200. A carrier is 3-4 bins wide, so it can
sit between two sampled bins.

Fix: each display point takes the largest of all the bins it covers. Done on the Pi in
`rpi/SDRcore-recv-linux/sources/dsputils.c` (the commit that adds this note). Tested on the
Pi at 800: the carrier is back at 14.046, .049, .064, .067 and .070, all gone before.

**To do:** the same change in Windows recv
(`mscc-ui/windows-work-tree/SDRcore-recv/sources/dsputils.c`) and Ubuntu recv
(`linux/SDRcore-recv-linux/sources/dsputils.c`). Copy the Pi hunk.

Side effect: at 800 and 1600 the noise floor reads a little higher (largest of several noise
bins), so the level differs slightly between resolutions. Not measured.

## Cause 2 — client (WPF and Avalonia)

The spectrum draw loop takes one data point per screen pixel and skips the rest:

- WPF `MSCC.Wpf/Controls/SpectrumDisplayControl.xaml.cs`, about line 725:
  `idx = (int)(dataFrac * (data.Length - 1) + cwOffsetBins)`, then `data[idx]` only.
  The waterfall does the same (loop about line 866, row sampler about line 1296).
- Avalonia `MSCC.Avalonia/Controls/SpectrumRenderer.cs`, about lines 89-96 (spectrum) and
  201-208 (waterfall): same, with `Math.Round`.

Ron's plot is about 964 pixels wide at full window and less when the window is smaller. With
more data points than pixels (always at 1600 and 3200; at 800 whenever the plot is under 800
pixels wide) some points are never drawn, and a carrier on one of them vanishes. Which points
are skipped depends on the window width.

**Proof (Ron, 2026-10-05):** 800 points, VFO 14.040, carrier missing. Slowly resizing the
client window makes the carrier come and go. Nothing else changed.

Fix: for each pixel, draw the largest of all the data points that fall in that pixel (from
this pixel's index up to, but not including, the next pixel's index; at least one point).
Same for the waterfall. When there are fewer points than pixels nothing changes.

## Not part of this

Two spurs seen at VFO 14.074 / 14.075 (about 16 kHz either side of the LO point) are real:
they stay with the generator off. They are a birdie from the Si5351 against its 25 MHz
crystal, visible only within about 1.5 kHz of 14.0745. Hardware, not the display.
