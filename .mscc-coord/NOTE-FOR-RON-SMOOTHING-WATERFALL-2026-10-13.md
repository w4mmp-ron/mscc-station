# For Ron: SMOOTHING is in (cmd-075); the waterfall now smears and runs fast

From Stew, 2026-10-13. For your review. **No change has been made yet.**

## What cmd-075 did (WPF 10.13.1, smoke-passed by Stew)

Implemented as asked in `NOTE-FOR-STEW-SMOOTHING-CONTROL-2026-10-12.md`:

- **SMOOTHING control:** the S/W window's REFRESH is now SMOOTHING, values 0, 1, 2, 3, 4, 6, 8, 10, 15, 20, 30. A change sends `CMD_GET_SET_PANADAPTER_SMOOTHING` (0xDA) to the host.
- **Default 3** (Stew's choice, not 4). It's kept in the client ini (`SPECTRUM_SMOOTHING`) and sent at every Start as well as on change.
  - The client cannot read the value back from the host: no ms-sdr (Windows, Ubuntu or Pi) sends 0xDA or `PANADAPTER_AVERAGE` to the client. The `PANADAPTER_AVERAGE=%d;` strings are only the ini file and a commented-out log line.
  - So the client's value replaces whatever the host had saved (your 9 becomes the client's setting).
- **Frame dropping removed** (`MainViewModel.cs` `SpectrumUpdated` / `_spectrumFrameCounter`). The spectrum and waterfall both run at the full 15.6 frames per second.
- Also in 10.13.1, and not related to this note: the waterfall now falls by default (a "Climbing" option gives the old direction), and BASELINE and the waterfall levels are remembered per band.

## What Stew sees

1. **The waterfall scrolls about 3× faster** than with his old REFRESH 3 (about 5.2 rows per second before, 15.6 now). REFRESH used to set the waterfall speed as well, because one row was drawn per displayed frame. That speed control is gone.
2. **At higher SMOOTHING the waterfall smears.** It's built from the same host-smoothed frames, so signals blur along the time axis. Smeared signals are useless, especially for CW, where the dots and dashes run together.
3. **The trade gets worse in both directions.** To get the calm trace he had with REFRESH 3, he now needs SMOOTHING about 8-10, and that makes the waterfall smear worse. Low SMOOTHING keeps the waterfall sharp but the trace is nervous, and the waterfall still runs fast.

So one control now sets two things that want different values: how calm the trace is and how sharp and fast the waterfall is.

## Options for you to review

**(a) Client WF SPEED divisor (client only, small)**
- A "WF SPEED" setting in the S/W window, values 1-10, default 3 (about 5.2 rows per second, Stew's old look); 1 = your full rate.
- The spectrum trace keeps every frame (15.6 per second, with server smoothing); only every Nth frame becomes a waterfall row.
- The row is the **maximum per display point across the skipped frames**, so short signals (CW elements, bursts) aren't lost between rows. This is the same idea as max-per-pixel.
- Client ini only; no host, server or protocol change.
- WPF about 25-35 lines (`SpectrumDisplayControl.xaml.cs` waterfall row push, `SpectrumWaterfallSettings`, S/W window); the same later for Avalonia.
- Limitation: it fixes the speed but **not the smear**. The waterfall is still built from smoothed frames, so Stew would have to keep SMOOTHING low (0-3) for a sharp waterfall and accept a less calm trace. A low SMOOTHING with WF SPEED 3 roughly gives back the old REFRESH 3 waterfall.

**(b) Host sends unsmoothed data for the waterfall (fixes the smear properly)**
- Recv sends the unsmoothed (or lightly averaged) frame for the waterfall, alongside the smoothed one for the trace, or the client gets a flag or second stream it can use for waterfall rows.
- That's a recv change (`panadapter.c`) and a protocol / client change, on **all platforms** (Windows, Ubuntu and Pi recv; WPF and Avalonia), plus compatibility with older clients and servers.
- It could be combined with (a) for the speed.

## Stew's decision

Left for your review. Nothing changed beyond cmd-075. Stew is packaging R10-13-1 as tested.
Please say which option you prefer (or another idea), and whether a smoothed waterfall is ever wanted.
