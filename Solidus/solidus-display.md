# solidus-display — plan: a compact client layout for the Solidus 5" touch screen

Ron's idea, 2026-10-06. **Plan only: nothing is changed, nothing is built.**
Added to the Solidus archive at Ron's request; the rest of the archive is untouched.

## Goal

Run the MSCC client on the Solidus' own display: **5 inch, 800 x 480, touch screen.**

## Why the current client does not fit

- The Solidus is a Pi, so it runs the **Avalonia** client (`Solidus/client/Avalonia`), not the
  Windows WPF one.
- That client opens at 1280 x 720 and refuses to go below 1024 x 600
  (`src/MSCC.Avalonia/Views/MainWindow.axaml`, lines 13-14).
- Scaling it down to 800 x 480 means about 75 %, on a screen that is already small: the
  9-point labels become unreadable and the 20-pixel buttons end up about 2 mm tall.
- The layout is a desktop one: two fixed side rails, analog meter dials, many small controls.

So this is a **separate compact layout**, not a shrunken desktop one.

## Size budget

800 x 480 on a 5" panel is about 7 pixels per millimetre. A finger-sized button needs to be
about 60 pixels tall (about 9 mm). Six buttons across are about 130 pixels wide each.

## Main screen

```
+--------------------------------------------------------------+
| 14.074.000  USB   S ▮▮▮▮▮▮▯▯▯  S7          TX   12:34 UTC  |  64 px
+--------------------------------------------------------------+
|                                                              |
|                      spectrum                                | 140 px
|                                                              |
+--------------------------------------------------------------+
|                                                              |
|                      waterfall                               | 212 px
|                                                              |
+--------------------------------------------------------------+
| BAND  | MODE  | FILTER |  STEP  |  PTT   |  MORE             |  64 px
+--------------------------------------------------------------+
```

- **Top bar (64 px):** frequency in large digits, mode, the S-meter as a bar (it becomes the
  power / ALC bar on transmit), a TX indicator, the clock.
- **Spectrum (140 px) and waterfall (212 px):** full width, no side rails. Tap to tune to a
  signal, drag to move the dial.
- **Bottom row (64 px):** six buttons. BAND, MODE, FILTER and STEP each open a panel of large
  choices over the waterfall; the panel closes when one is picked. PTT is a latching button.
- **MORE:** pages for audio levels, CW, noise controls (NB / NR / AN), TUN, calibration and
  settings. These are set-and-forget, so they do not need to be on screen all the time.

## Physical controls

Fine tuning stays on the knob, so the screen needs no tuning buttons. ms-sdr already has the
front-panel command set for a knob and switches (`CMD_MFC_*`, knob / left / middle / right /
PTT switch, in `usbavrcmd.h`): tune, band, mode, step, RIT, CW and Hi bandwidth. Which of
those the Solidus hardware actually has is to be confirmed.

## A bonus at this size

The plot is exactly 800 pixels wide and the server's lowest spectrum setting is 800 points:
one data point per pixel. Nothing is skipped, so the vanishing-carrier fault seen on the
desktop client (too many points for the pixels) cannot happen here. The compact layout should
request 800 points and not offer 1600 / 3200.

## Left off the main screen on purpose

- VFO B
- the analog S-meter and ALC dials (replaced by the bar)
- the calibration tabs (QRP CAL, AMP CAL, RX IQ, TX IQ, FREQ CAL): moved to MORE pages, or
  done from a desktop client connected over the network
- favourites, keyer memories, UI colour settings: MORE pages or dropped; to be decided

## What is reused, what is new

Reused as they are (`src/MSCC.Avalonia`):
- `ViewModels/MainViewModel.cs` and `SpectrumWaterfallViewModel.cs`: radio state, commands,
  the link to ms-sdr
- `Controls/SpectrumDisplayControl`, `SpectrumRenderer.cs`, `WaterfallPalettes.cs`
- `Services/` (settings, band memory, favourites)

New:
- one window, for example `Views/CompactWindow.axaml`, with the layout above
- a bar meter control (S / power / ALC)
- the pop-up choice panels and the MORE pages
- a start-up choice between the desktop and the compact window: by screen size, a setting, or
  a command-line switch; the compact window runs full screen with no title bar

Not a rewrite: the same code underneath, a second window on top.

## To check before building

- Does the spectrum control already do tap-to-tune and drag with touch input on the Pi, or
  only with a mouse?
- Does the archived Avalonia client still match the current servers? The live client has
  moved on since this snapshot (2026-09-11); the compact window may be better built on the
  current `mscc-ui/Avalonia-Migration` and used for the Solidus from there.
- Which front-panel controls the Solidus has (see "Physical controls").
- CPU load of the waterfall at 800 x 212 on the Solidus' Pi.
- An on-screen keyboard is needed for the few text fields (host name, keyer memories), or
  those stay desktop-only.

## Order of work

1. Compact window with the top bar, spectrum, waterfall; read-only (no buttons yet). Proves
   the size, the readability and the CPU load on the real panel.
2. Bottom row with BAND, MODE, FILTER, STEP panels and PTT.
3. Touch tuning on the spectrum.
4. MORE pages, most-used first (audio levels, noise controls, TUN).
5. Full-screen start-up on the Solidus.
