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

## The code to build on: `Solidus/client/Avalonia-current`

Copied 2026-10-07 from the live client (`mscc-ui/Avalonia-Migration`, version 0.6.71) so the
compact layout has a current base inside the Solidus folder. The archived snapshot
`Solidus/client/Avalonia` (0.6.42) is untouched.

- Source only: `src/MSCC.Avalonia`, `packaging/`, the build scripts, the `.sln` and the
  documents. No `.deb` packages, no publish output, no build folders.
- `src/MSCC.Core` is a copy of the shared protocol library (it lives outside this folder in
  the live tree, under `mscc-ui/windows-work-tree/.../src/MSCC.Core`).
- Two changes from the originals, so the folder builds on its own: the Core reference in
  `src/MSCC.Avalonia/MSCC.Avalonia.csproj` and in `MSCC.Avalonia.sln` now points at
  `src/MSCC.Core`. Everything else is byte for byte the live code of that date.
- Checked: `dotnet build` of the copied solution on Windows, Release, 0 errors.
- The copied `README.md` and other documents still describe the live tree's paths.
- It is a copy: later changes Stew makes to the live client or to MSCC.Core do not arrive
  here by themselves.

## To check before building

- Does the spectrum control already do tap-to-tune and drag with touch input on the Pi, or
  only with a mouse?
- Which client to build on: settled 2026-10-07, the current one, copied to
  `Solidus/client/Avalonia-current` (see above). The archived 0.6.42 snapshot is not the base.
- Which front-panel controls the Solidus has (see "Physical controls").
- CPU load of the waterfall at 800 x 212 on the Solidus' Pi.
- An on-screen keyboard is needed for the few text fields (host name, keyer memories), or
  those stay desktop-only.

## Testing over VNC, without the panel

Ron's Pi is headless and he uses RealVNC or TigerVNC. The Avalonia client is an ordinary
desktop program on the Pi (Stew ships it as the `mscc-ui` .deb), so it shows in a VNC session
and a mouse click stands in for a tap. Not tried yet in an 800 x 480 session.

- **TigerVNC is the better fit:** it makes its own virtual desktop at a chosen size, so the
  screen can be exactly the panel's: `vncserver -geometry 800x480`.
- RealVNC mirrors the Pi's real desktop, so that desktop's resolution has to be set to
  800 x 480 first.

What a VNC test shows: whether everything fits and is readable at 800 x 480, whether the
buttons, panels and pages work, whether the client talks to the servers.

What it does not show:
- **Speed and CPU load.** A virtual VNC desktop has no graphics acceleration; the spectrum and
  waterfall are drawn by the CPU and sent over the network, so they look slower and cost more
  CPU than on the real panel. Do not judge performance from it.
- **Real size.** 800 x 480 on a PC monitor is much bigger than on a 5" panel. Shrinking the
  viewer window to about 11 cm wide gives a fair impression of the text size.
- **Touch behaviour.**

The client also builds for Windows from the same code (no Windows build is published; remote
audio, local server start and the local CAT port are Linux-only), so the layout can be tried
on the PC in an 800 x 480 window connected to the Pi as well. Read from the code, not built.

## Effort (estimate from reading the code, 2026-10-07)

Moderate: a new window on existing code, about 5 to 8 working sessions including test rounds.

| Step | Effort |
|---|---|
| 1. Compact window, display only | small, 1 session |
| 2. Bottom row, choice panels, PTT | small to medium, 1 session |
| 3. Touch tuning | small or medium, up to 1 session (depends on the existing control) |
| 4. MORE pages | largest part, 2 to 4 sessions (depends on how many pages) |
| 5. Full-screen start-up, packaging | small, under 1 session |

Could push it up: touch misbehaving under the Pi desktop, an on-screen keyboard, waterfall
CPU load, agreeing with Stew on building on the current client. Keeps it down: stopping after
step 3 already gives a usable radio screen; MORE pages can follow one at a time.

## Order of work

1. Compact window with the top bar, spectrum, waterfall; read-only (no buttons yet). Proves
   the size, the readability and the CPU load on the real panel.
2. Bottom row with BAND, MODE, FILTER, STEP panels and PTT.
3. Touch tuning on the spectrum.
4. MORE pages, most-used first (audio levels, noise controls, TUN).
5. Full-screen start-up on the Solidus.
