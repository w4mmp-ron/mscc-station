# cmd-095: Avalonia left/right panel sliders — PowerSDR Classic physical look

Host: stew-HP (Ubuntu amd64, host id ubuntu-stew). Maintained client: `Avalonia/` (`MSCC.Avalonia.sln`,
package mscc-avalonia, now 1.7.08). Do NOT edit `mscc-ui/Avalonia-Migration` (frozen), `rpi/`,
`Solidus/`, `installers/rpi/`, or the WPF tree. WPF is the reference for wheel behavior only; do not
change it.

## STOP

- Commit only. Do not push. Stew pushes.
- No installer, no `.deb`, no GitHub Release.
- No version bump. Leave `MSCC.Avalonia.csproj` at 1.7.08. Do not edit
  `packaging/mscc-avalonia/DEBIAN/control`, the Addendum, or `installers/windows/CHANGELOG.md`.
- Do not touch `rpi/`, `Solidus/`, `installers/rpi/`, or WPF.
- Pull first only if the tree is not already ahead of origin with Stew's unpushed work. If
  `origin/main` has commits you do not have, stop and report; do not merge or rebase over them.
- Do not change bindings, min/max, SmallChange, tick snap, IsEnabled, or tooltips. This cmd is
  look and layout only.
- Scope is the left and right main-window rails only. Leave Settings, S/W, Remote Audio, RX/TX,
  and the cal tabs alone (see Excluded below).

## Findings (tree at b0afc766, after cmd-094)

There is no custom half-moon or arc slider class. Every slider in the app is a stock Avalonia
`Slider`. The "half-moon" look on the rails is Fluent's default circular thumb and rounded track
(`Slider.mscc-slider` in `App.axaml` 182–186 only sets MinHeight 18, VerticalAlignment Center, and
Margin 0). The approved replacement is a PowerSDR Classic linear physical slider: label on the
left, a dark inset/recessed horizontal track, a glossy shiny-black/silver rectangular thumb, and
the numeric value in yellow on the right, on a dark panel, about 26–30 px per row.

### Wheel handling (keep this behavior)

WPF (`MainWindow.xaml.cs` 857–866, `Slider_PreviewMouseWheel`): one wheel notch = `SmallChange`
(or 1 if SmallChange is 0), clamped to min/max, event handled.

Avalonia already matches that in `MainWindow.axaml.cs` 317–327 (`Slider_OnPointerWheelChanged`).
`WheelDirection` is at 309. Every left/right rail slider already wires
`PointerWheelChanged="Slider_OnPointerWheelChanged"`. Keep one notch = one SmallChange. The
comment at 318 that "AGC FAST RELEASE is not wired" is stale: the right-rail AGC slider is wired
and uses SmallChange 10.

There is no attached behavior and no Avalonia.Xaml.Behaviors package. Add a small attached
property (no new NuGet) so the physical style turns the wheel on by itself. See the change plan.

### In scope — left and right rails (10 stock Sliders)

All use `Classes="mscc-slider"` and the existing wheel handler. Rows are already
label | slider | value, except ZOOM (slider | value, no left label). Rails are 160 px wide
(`MainWindow.axaml` 23: `ColumnDefinitions="160,*,160"`). Window default 1280×720, min 1024×600.
The left rail comment at line 25 is "dense — aim: no scrollbar at 1024×600". It already has a
ScrollViewer.

**Left rail** (`MainWindow.axaml`, current row Height 20):

| Line | Label | Binding | Min | Max | SmallChange | Notes |
|------|-------|---------|-----|-----|-------------|-------|
| 70 | Vol (Phones) | `PVolume` | 0 | 100 | 1 | Grid `40,*,28` |
| 78 | Mic (Phones) | `PMicGain` | 0 | 100 | 1 | same row |
| 89 | Vol (Digital) | `DVolume` | 0 | 100 | 1 | Enabled=`DigitalControlsEnabled` |
| 97 | Mic (Digital) | `DMicGain` | 0 | 100 | 1 | Enabled=`DigitalControlsEnabled` |
| 153 | (none) ZOOM | `SpectrumZoom` | 1 | 4 | 1 | Snap to tick 1. Grid `*,28`. Readout `{0}×` |
| 165 | Offset (RIT) | `RitOffset` | −500 | 500 | 10 | Grid `40,*,32` |

**Right rail** (`MainWindow.axaml`, POWER/COMP/AGC Height 22, Pls Height 20):

| Line | Label | Binding | Min | Max | SmallChange | Notes |
|------|-------|---------|-----|-----|-------------|-------|
| 1617 | `{Binding RfPowerBankLabel}` | `RfPower` | 0 | 100 | 1 | Grid `40,*,28`. Drive for the active mode |
| 1625 | COMP | `Compression` | 0 | 24 | 1 | dB, phones path |
| 1633 | AGC | `AgcFastRelease` | 0 | 2000 | 10 | Grid `40,*,36`. Readout `{0}ms`, FontSize 9 |
| 1690 | Pls | `NbPulse` | 50 | 510 | 1 | Grid `32,*,26`. NB pulse width µs |

### Excluded — follow-up cmd (do not change these now)

All of these are also stock `Slider`s. They are out of this cmd because Stew asked to start with
the panels, and Settings / S/W need layout work (they are not a drop-in of the 26–30 px physical
row).

| Area | Count | File:lines | Why not this cmd |
|------|-------|------------|------------------|
| RX/TX POWER LEVELS | 5 | `MainWindow.axaml` 607, 616, 625, 634, 644 (Tune / CW / SSB / AM / FM). Already `mscc-slider`, wheel, row Height 26, Grid `88,*,40` | Same class will apply later with almost no work. Not a rail control. |
| QRP CAL POWER | 1 | `MainWindow.axaml` 896. 0–100, snap 1, wheel, calibrating only | Cal tab layout. Later. |
| AMP CAL POWER | 1 | `MainWindow.axaml` 1002. −99–0, snap 1, wheel | Cal tab layout. Later. |
| RX IQ | 2 | `MainWindow.axaml` 1097 Offset (−200–200, step 1), 1115 LO FINE (−1000–2009, step 1) | Cal tab. Later. |
| TX IQ | 3 | `MainWindow.axaml` 1187 Power 0–100, 1200 Offset −200–200, 1214 Phase −100–100 snap 1 | Cal tab. Later. |
| SETTINGS UI Appearance RGB | 9 | `MainWindow.axaml` 1389/1394/1399 (Bg R/G/B), 1425/1430/1435 (Btn), 1451/1456/1461 (Accent). 0–255, no `mscc-slider`, no wheel | Packed under 90 px ListBoxes, Spacing 2. A 26–30 px physical thumb needs layout surgery. |
| S/W window (`SpectrumWaterfallWindow.axaml`) | 12 | 98/103/108 CUSTOM RGB Height 18; 118 dB CAL −20–20 step 0.5; 130 GRID MAX and 136 MIN on one row; 154 BASELINE; 173 ZOOM 1–4 snap; 228/236/244/252 waterfall High/Low/Gain/Zero; 275 SPECTRUM SIZE 20–80 snap | Dense, mixed layouts (two sliders on one row, Height 18), no wheel. Needs its own pass. |
| Remote Audio (`RemoteAfWindow.axaml`) | 5 | 40 Play volume 0–100; 60/64/68 EQ Low/Mid/High −12–12; 77 Mic vol 0–100. Code-behind `ValueChanged`, no wheel | Separate window, not a rail. Later. |

No sliders on MAIN, CW, FAVORITES, FREQ CAL, or Debug Log. Analog S-meter / ALC are meters, not
sliders — leave them.

## Change plan

1. Add theme resources in `Avalonia/src/MSCC.Avalonia/App.axaml` (with the existing `Ui*` brushes)
   so a later bling pass edits one place. Suggested keys (names can vary, they must be
   `DynamicResource`):

   - `UiSliderTrackFillBrush` — dark recessed trough
   - `UiSliderTrackBorderBrush` — inset edge
   - `UiSliderTrackHighlightBrush` — light inner lip at the top of the trough
   - `UiSliderThumbFillBrush` — glossy black face
   - `UiSliderThumbHighlightBrush` — silver / specular highlight
   - `UiSliderThumbBorderBrush` — chrome/silver edge
   - `UiSliderValueBrush` — yellow/gold for the rail numeric (mock is yellow; current
     `mscc-readout` is mint `UiAccentBrush` — do not recolor every readout in the app)

2. Add a `ControlTheme` or Style `Slider.mscc-physical` in `App.axaml` (prefer `ControlTheme`
   based on the Fluent Slider theme, or a full template if based-on cannot restyle the thumb and
   track enough). Horizontal only. Look:

   - Dark inset/recessed track, a few pixels tall, running the width of the slider.
   - Glossy rectangular thumb, a little taller than the track, shiny black with a silver highlight
     and a silver edge. Not a circle, not a Fluent pill.
   - Clicking the track still moves the thumb (keep the usual Slider track/repeat behavior).
   - Disabled: dim the track and thumb (Digital Vol/Mic when Remote is on; all rails when
     disconnected).
   - Thumb width about 10–14 px: the rail is only 160 px, and the label + value already use ~70 px.

   You may keep `Slider.mscc-slider` as a size helper or fold it into `mscc-physical`. Do not
   restyle the Fluent Slider globally (that would change Settings, S/W, and the cal tabs).

3. Add `Avalonia/src/MSCC.Avalonia/Controls/SliderWheel.cs`: a small attached bool property
   (e.g. `SliderWheel.OneStepPerNotch`). When true, hook `PointerWheelChanged` with the same logic
   as `Slider_OnPointerWheelChanged` (one `SmallChange` per notch, or 1 if SmallChange is 0,
   clamped, event handled). No new NuGet. Set the attached property from the `mscc-physical` style
   (`<Setter Property="…SliderWheel.OneStepPerNotch" Value="True"/>`). Then remove
   `PointerWheelChanged="Slider_OnPointerWheelChanged"` from the 10 rail sliders only, so a notch
   does not apply twice. Leave the existing handler in `MainWindow.axaml.cs` for the tab sliders
   that still use it.

4. On the 10 rail sliders in `MainWindow.axaml`, add `Classes="mscc-physical"` (keep or drop
   `mscc-slider` as the style design requires). Do not change Value, Min, Max, SmallChange, snap,
   IsEnabled, or ToolTip.

5. Bump those 10 row Heights to 28 px (allowed range 26–30). Keep the existing column layouts.
   ZOOM stays slider | value with no left label. If a 3-digit value clips (RIT −500, AGC `2000ms`,
   Pls 510), widen only that value column a few pixels; do not widen the 160 px rail. Apply
   `Foreground="{DynamicResource UiSliderValueBrush}"` on those 10 readout TextBlocks (or a new
   `mscc-slider-value` class). Leave other `mscc-readout` uses mint.

6. At 1024×600, the left rail may show its ScrollViewer if the extra row height needs it. That is
   acceptable. Do not shrink buttons or section headers to hide it. Default 1280×720 must not clip
   or overlap on either rail.

7. Do not add a UserControl unless the ControlTheme truly cannot produce the look. Prefer the
   style + the existing row Grid.

## Acceptance

- The 10 left/right rail sliders use the physical look (recessed track, rectangular glossy thumb,
  yellow value). Settings, S/W, Remote Audio, RX/TX, and cal tabs are unchanged.
- Drag and mouse wheel still change the same bindings, ranges, and steps as before. One wheel
  notch = one SmallChange (AGC 10 ms, RIT 10 Hz, ZOOM 1×, others 1).
- Values still go to the radio and still match the readout (AF volume, mic, drive, COMP, AGC-T /
  AGC fast release, NB pulse, RIT, zoom).
- Digital Vol/Mic stay disabled while Remote is on. Rails stay disabled while disconnected.
- No clipped labels or values, no overlap, at 1280×720 and at 1024×600.

## Build and smoke (stew-HP, amd64)

Do not build a `.deb`. Run from source:

```
cd /home/stew/Documents/GitHub/mscc-station/Avalonia
dotnet build src/MSCC.Avalonia/MSCC.Avalonia.csproj -c Release
dotnet run --project src/MSCC.Avalonia/MSCC.Avalonia.csproj -c Release --no-build
```

Checklist (connect to the radio for the value-matches-radio items):

- [ ] Release build runs on stew-HP (Ubuntu amd64) at the normal 1280×720 window
- [ ] Left rail, Phones Vol: drag and wheel (1 per notch, 0–100); value matches phones volume
- [ ] Left rail, Phones Mic: drag and wheel; value matches mic gain
- [ ] Left rail, Digital Vol and Mic: same, and both go disabled (dimmed thumb/track) when Remote is on
- [ ] Left rail, ZOOM: wheel and drag snap 1–4; readout shows ×
- [ ] Left rail, RIT Offset: wheel is 10 Hz per notch, range −500 to 500; −500 does not clip
- [ ] Right rail, POWER (drive): drag and wheel, 0–100; matches the radio drive for the active mode
- [ ] Right rail, COMP: drag and wheel, 0–24
- [ ] Right rail, AGC: wheel is 10 ms per notch, 0–2000; `ms` readout does not clip
- [ ] Right rail, Pls (NB pulse): drag and wheel, 50–510, 1 per notch
- [ ] No overlap or clipped labels on either rail at 1280×720
- [ ] Resize to 1024×600: rails remain usable (a left-rail scrollbar is OK); no overlap
- [ ] Settings RGB, S/W window, Remote Audio, RX/TX power, and cal sliders still look as they did (Fluent)

## Commit (commit only, no installer, no GitHub release)

Stew's rule: Build commits on this host; Stew pushes. Commit, then stop. Do not push.

```
git add Avalonia/src/MSCC.Avalonia/App.axaml Avalonia/src/MSCC.Avalonia/Views/MainWindow.axaml Avalonia/src/MSCC.Avalonia/Controls/SliderWheel.cs Avalonia/src/MSCC.Avalonia/Views/MainWindow.axaml.cs
git status
git commit -m "Avalonia: PowerSDR-style physical sliders on the left and right rails"
```

Add any extra file the attached property or theme needs, and only those. Never add: `.deb` files,
`Avalonia/publish/`, `installers/`, Visual Studio bookkeeping, or other people's dirty files.
