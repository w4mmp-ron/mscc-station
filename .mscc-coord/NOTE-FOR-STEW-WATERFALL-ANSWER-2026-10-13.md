# For Stew — Waterfall after SMOOTHING: Ron's answer is option (a)

From Ron, 2026-10-13. Answer to `NOTE-FOR-RON-SMOOTHING-WATERFALL-2026-10-13.md`.

## Answer

Option (a), the WF SPEED setting in the client, is fine. Details are yours.

Option (b), unsmoothed waterfall data from recv, is not asked for now.

## What Ron saw (WPF 10.13.1 against the Pi, 20 m CW)

From a screen recording, SMOOTHING 3 for the first 15 s, then 10:

- **SMOOTHING 3:** no smear. CW shows as separate dits and dahs. A row carries over into
  the next rows for about 0.2 s.
- **SMOOTHING 10:** smear, as you describe. CW runs into continuous streaks and the noise
  shows vertical streaks. A row carries over for roughly 1.5 to 2 s.
- **Scroll speed is the same at both settings.** SMOOTHING is only the averaging done in
  recv; it does not change the frame rate.
- Ron finds the scroll a bit fast, so a speed setting is welcome.

## Things to know

- **The Pi sends the same frame rate as Windows.** `panadapter.c` is identical in the two
  recv trees and both run 3 blocks per frame (15.6 frames per second). In the recording the
  spectrum trace updates 15.6 times per second.
- **How the waterfall is drawn today** (`SpectrumDisplayControl.xaml.cs`): the last 300
  frames are kept (`MaxWaterfallHistory`) and those 300 rows are stretched over whatever
  height the waterfall has. So it always shows the last 19 s (300 / 15.6), and the scroll
  speed in pixels depends on the window size. On Ron's screen the waterfall is about 180
  pixels high: 0.6 pixel per row, 9.4 pixels per second (measured: 47 pixels between the
  5 s time marks).
- With WF SPEED N on that design, 300 rows would cover N x 19 s (38 s at 2, 58 s at 3).
  That is what Ron is after: more time on screen, slower scroll.
- **The stretch blends, it does not take the maximum.** Each screen line is a linear mix
  of the two history rows nearest to it. When the waterfall is less than 300 pixels high,
  as on Ron's screen, some rows hardly reach the screen, so a signal one row long can show
  weak or not at all. Your maximum-per-point idea for the skipped frames would be useful in
  this step as well.
- A row at WF SPEED N lasts N x 64 ms. The maximum keeps short signals, but a gap shorter
  than a row fills in, so fast CW merges at higher N. That comes with any slower waterfall.
- Ron runs SMOOTHING low (3 here), where the waterfall is sharp, so he has no need for (b).
- Nothing was changed on the Pi.
