# cmd-096: Avalonia left panel fit at minimum window size (checkboxes, widths, slider values, track, thumb line, pkts line)

Host: stew-HP (Ubuntu amd64, host id ubuntu-stew). Maintained client: `Avalonia/` (`MSCC.Avalonia.sln`,
package mscc-avalonia, 1.7.08). Builds on cmd-095 (`e4ca55dd`). Do NOT edit `mscc-ui/Avalonia-Migration`
(frozen), `rpi/`, `Solidus/`, `installers/rpi/`, or the WPF tree. WPF is the size reference only.

## STOP

- Commit only. Do not push. Stew pushes.
- No installer, no `.deb`, no GitHub Release.
- No version bump. Leave `MSCC.Avalonia.csproj` at 1.7.08. Do not edit
  `packaging/mscc-avalonia/DEBIAN/control`, the Addendum, or `installers/windows/CHANGELOG.md`.
- Do not touch `rpi/`, `Solidus/`, `installers/rpi/`, or WPF.
- Do not pull, merge, or rebase. When this brief was written, stew-HP was 4 commits ahead of
  origin (cmd-094/095) and 1 behind: Ron's `419408e3` R11-27-0 (Pi trans 3.165 remote mic buffer;
  `rpi/`, `installers/rpi/`, and docs only, no `Avalonia/` or `AGENTS.md` overlap). Commit on top of
  the local tree. Stew pulls (`git pull --rebase`) before he pushes.
- Do not change bindings, min/max, SmallChange, tick snap, IsEnabled, commands, or tooltips. This cmd
  is look and layout only, except item 5 (one TextBlock removed).
- Never commit `bin/`, `obj/`, `Avalonia/publish/`, `.deb`, or Visual Studio files.

## Goal (Stew, 2026-11-27)

At the minimum window size (`MainWindow.axaml` 14: `MinWidth="1024" MinHeight="600"`), the left
panel has **no scroll bar** and the **Clear RIT button shows in full**. Nothing clips on the right
edge of the left panel.

## Findings (tree at e4ca55dd)

All paths are under `Avalonia/src/MSCC.Avalonia/`.

**Left panel size.** `Views/MainWindow.axaml` 23: `ColumnDefinitions="160,*,160"`. The left Border
(26) has Margin `0,0,3,0`, and `Border.mscc-panel` (`App.axaml` 387–393) adds 1 px border + 4 px
padding. Usable content: **147 px wide**. Height at 1024×600: 600 − 8 (grid margin 4) − 10 (border +
padding) = **582 px**. WPF's left column is 180 px (`MSCC.Wpf/MainWindow.xaml` 208); Avalonia's is
160, so controls copied at WPF widths overflow.

**Height used now (estimate, top to bottom):** SERVER 14, Connect 24, status line 14, Auto/Launch
row 33, Host 23, Port 20, Phones header 18 + 2 rows 58, Digital header 18 + 2 rows 58, AUDIO header
18 + buttons 27 + Remote 28, FILTERS/STEP header 18 + 4 rows 92, ZOOM header 18 + row 29, RIT
checkbox 33, Offset 29, Clear RIT 24 → **about 596 px to the bottom of Clear RIT**, then the pkts
line (~15–24 px). Over by ~14 px at Clear RIT, ~38 px with the pkts line. That matches Stew's
screenshot (Clear RIT half cut, scroll bar).

**Saved by this cmd:** checkboxes 32 → 16 px on the Auto/Launch row and the RIT row = **32 px**;
pkts line removed = **15–24 px**. Result: ~564 px used of 582, about **18 px spare** (about 7 px if
the status line wraps to 2 lines). Enough without touching slider rows.

### 1. Checkboxes are Fluent size

Avalonia's Fluent CheckBox has MinHeight 32 and a 20×20 box (`NormalRectangle`) in a 20 px column.
No CheckBox style exists in `App.axaml`. WPF uses the stock Windows CheckBox (13×13 box, about
16 px row) with FontSize 9 Bold (`MSCC.Wpf/MainWindow.xaml` 252–262). Avalonia uses FontSize 8 on
Auto/Launch (`MainWindow.axaml` 39–44) and FontSize 9 on RIT (160–162).

App-wide CheckBoxes (25): `MainWindow.axaml` 39, 42, 160, 402, 481, 499, 889, 1077, 1519, 1663;
`SpectrumWaterfallWindow.axaml` 161, 166, 181, 184, 187, 267, 269; `RemoteAfWindow.axaml` 45, 47, 56;
`DebugLogWindow.axaml` 24; `Controls/AnalogSMeterControl.axaml` 24, 47 and
`Controls/AnalogAlcMeterControl.axaml` 26, 46 (HOLD / Peak). The meter ones are wired by `x:Name`
in code-behind; keep their names.

### 2. Right-edge clipping in the left panel

- **Host dropdown arrow:** `MainWindow.axaml` 47 `ColumnDefinitions="28,*,22"`, ComboBox at 52 with
  `Classes="mscc-host-pick"`. The style (`App.axaml` 348–357) sets Width 22 but not MinWidth, so
  Fluent's `ComboBoxThemeMinWidth` (64) wins and the combo spills ~42 px past the panel edge. Fluent's
  ComboBox template also puts the arrow in a 32 px column.
- **Phones / Digital:** `MainWindow.axaml` 105–116, both `Width="80"` with Spacing 4 = 164 px in a
  147 px panel. Digital clips by 17 px. (Copied from WPF, which has 180 px.)
- **Remote:** `MainWindow.axaml` 117 `Width="164"`: same 17 px clip.

### 3. Slider values touch the thumb

`TextBlock.mscc-readout` (`App.axaml` 294–301) sets `HorizontalAlignment Right` **and**
`MinWidth 28`. In a 28 px column the TextBlock is 28 px wide with left-aligned text, so the number
starts at the slider's right edge. That is why 76 and 98 sit against the thumb.

### 4. Track thickness

`MsccPhysicalSliderTheme` (`App.axaml` 77–121): the track Border at 85 is `Height="6"` with a 1 px
border and a 1 px top highlight (91–94), so only ~3 px of dark trough shows. Slider Height is 22
(78–79), thumb 12×18 (`MsccPhysicalThumbTheme` 55–59). Rows are 28.

### 5. The bar at the bottom-left is the packet counter line

`MainWindow.axaml` 175–176:

```
<!-- Temps moved to SET tab (frees left rail). Optional session counters: -->
<TextBlock Text="{Binding PacketStatsText}" FontSize="8" Foreground="#888888" TextWrapping="Wrap" Margin="0,4,0,0"/>
```

It is a grey debug counter: `PacketStatsText` is set in `ViewModels/MainViewModel.cs` 8939
(`UpdatePacketStats`): `"Pkts {received} | KA {keep-alives} | D5 {pan packets} | Spec {spectrum frames}"`.
No code-behind touches this TextBlock (it has no `x:Name`). The same text also shows on the
**SETTINGS** tab under TEMPS / STATUS (`MainWindow.axaml` 1556, FontSize 11). Stew does not need it
on the left panel; it stays on SETTINGS.

### 7. White line on the slider thumb (Stew approved)

`MsccPhysicalThumbTheme` (`App.axaml` 55–75): inside the outer Border, an inner Border at 67–71
(`Height="2"`, `VerticalAlignment="Top"`, `Margin="1,2,1,0"`,
`Background="{DynamicResource UiSliderThumbHighlightBrush}"`, `Opacity="0.95"`) draws a white line
across the top of every rail thumb. `UiSliderThumbHighlightBrush` (`App.axaml` 41) is used only there.

## Change plan

### 1. Smaller checkboxes, app-wide (`App.axaml`)

- Add one implicit CheckBox theme in `Application.Resources`:
  `<ControlTheme x:Key="{x:Type CheckBox}" TargetType="CheckBox">` (a full small template; do not
  base it on Fluent's, whose 20 px column and 32 px MinHeight cannot be restyled from outside).
  This one place sets the size for every CheckBox in the app.
- Box **14×14 px**, 1 px border, corner radius 2. Check glyph fills ~10 px. MinHeight **16**.
  Content gap 4 px (`Padding="4,0,0,0"`), content VerticalAlignment Center.
- Colors to match the dark UI: unchecked box Background `#FF1A1A1A`, border `#FF888888`; checked
  box Background `{DynamicResource UiAccentBrush}` with a black check; pointer-over border white;
  disabled Opacity 0.45. Support `IsThreeState` (indeterminate = a small filled square) even though
  no MSCC CheckBox uses it now.
- Keep the whole row clickable (box and text), keep keyboard Space toggle, and keep `Foreground`,
  `FontSize`, `FontWeight` from each CheckBox (the template must use `TemplateBinding` for them).
- Do not change any CheckBox in the views except as below.
- Left panel only: Auto and Launch Servers (`MainWindow.axaml` 39–44) go to `FontSize="9"
  FontWeight="Bold"` to match WPF. Keep Spacing; set it to 6 if the row has room after the shrink
  (WPF uses a 6 px margin between them).

### 2. Right-edge fits (`MainWindow.axaml`, `App.axaml`)

- Host combo: in `ComboBox.mscc-host-pick` (`App.axaml` 348–357) add `MinWidth 0`. Change the
  grid at `MainWindow.axaml` 47 to `ColumnDefinitions="28,*,24"` and the style Width to 24; the Host
  TextBox (star column) gets narrower by itself. The drop-down arrow must show in full inside the
  24 px box. If Fluent's 32 px arrow column still clips it, add a small `ControlTheme` for
  `ComboBox.mscc-host-pick` only (a 24 px box holding just the arrow glyph, the same Popup and
  `ItemsSource`/`SelectedItem`/`SelectionChanged` wiring). Do not restyle other ComboBoxes.
- Phones / Digital (105–116): both `Width="70"`, Spacing 4 (144 px total). Same Height 24 and
  FontSize 10.
- Remote (117): drop `Width="164"`, use `HorizontalAlignment="Stretch"` (fills 147 px).
- Check every other left-panel row for anything wider than 147 px and fix it the same way (no
  fixed widths over 147).

### 3. Slider value spacing (`App.axaml`, `MainWindow.axaml`)

- Add `TextBlock.mscc-slider-value` in `App.axaml` next to `mscc-readout`: Foreground
  `{DynamicResource UiSliderValueBrush}`, FontSize 10, Bold, VerticalAlignment Center,
  `HorizontalAlignment Left`, `TextAlignment Left`, `MinWidth 0`, **`Margin="6,0,0,0"`**.
- On the 10 rail readouts (`MainWindow.axaml` 74, 82, 93, 101, 157, 169, 1621, 1629, 1637, 1694)
  replace `Classes="mscc-readout" Foreground="{DynamicResource UiSliderValueBrush}"` with
  `Classes="mscc-slider-value"`. AGC (1637) keeps `FontSize="9"`. Leave other `mscc-readout` uses alone.
- Value columns gain the 6 px from the slider column: left panel `40,*,28` → `40,*,32` (68, 76,
  87, 95), ZOOM `*,28` → `*,32` (152), RIT `40,*,32` → `40,*,36` (163); right panel POWER and COMP
  `40,*,28` → `40,*,32` (1615, 1623), AGC `40,*,36` → `40,*,40` (1631), Pls `32,*,26` → `32,*,30`
  (1688). Panel widths stay 160.
- Check: 100, −500, `2000ms`, 510 and `4×` show in full with a visible gap from the thumb at max.

### 4. Thicker track (`App.axaml` 85)

- Track Border `Height="6"` → **`Height="9"`**. Keep the 1 px border and the 1 px top lip (91–94).
- Do not change the slider Height (22), the thumb (12×18), or the row Height (28).

### 5. Remove the pkts line (`MainWindow.axaml` 175–176)

- Delete the comment line 175 and the `PacketStatsText` TextBlock at 176 from the left panel.
- Do not change `PacketStatsText` in `MainViewModel.cs`; SETTINGS (1556) still shows it.

### 6. Fit rule

- After 1–5, run at 1024×600. The left panel must show no scroll bar and Clear RIT in full, with
  its bottom border visible.
- If it is still short, trim `TextBlock.mscc-section` top margin (`App.axaml` 270, `0,4,0,1`) to
  `0,3,0,1` or `0,2,0,1` (6 headers, 6–12 px). Do not shrink slider rows, buttons, or the track.
- Keep the `ScrollViewer` (27) as it is (Auto); it is the safety net for smaller screens / large
  fonts.
- Default 1280×720 must look the same as before apart from these changes.

### 7. Remove the white line on the thumb (`App.axaml` 55–75, 41)

- Delete the inner Border at 67–71 (Height 2, Top, Margin `1,2,1,0`,
  `UiSliderThumbHighlightBrush`, Opacity 0.95).
- Keep the outer Border, `UiSliderThumbFillBrush` (gradient shading) and `UiSliderThumbBorderBrush`.
- Delete the `UiSliderThumbHighlightBrush` resource at 41 (grep first; at `e4ca55dd` nothing else
  uses it).

## Acceptance

- 1024×600: left panel has no scroll bar; Clear RIT shows in full; nothing clips at the right edge
  (Host arrow, Digital, Remote all inside).
- All CheckBoxes in the app use the small box; all still toggle their bindings.
- Rail values have a gap from the thumb; the track is thicker; the thumb has no white line.
- Wheel is still one SmallChange per notch on rail sliders (AGC 10 ms, RIT 10 Hz, ZOOM 1×, others 1).

## Build and smoke (stew-HP, amd64)

Do not build a `.deb`. Run from source:

```
cd /home/stew/Documents/GitHub/mscc-station/Avalonia
dotnet build src/MSCC.Avalonia/MSCC.Avalonia.csproj -c Release
dotnet run --project src/MSCC.Avalonia/MSCC.Avalonia.csproj -c Release --no-build
```

Checklist (connect to the radio for the binding items):

- [ ] Release build runs on stew-HP with no new warnings in the XAML
- [ ] Window at minimum size (1024×600): left panel has no scroll bar
- [ ] Window at minimum size: Clear RIT button shows in full
- [ ] Auto and Launch Servers boxes are about WPF size (14 px), text readable, row shorter
- [ ] RIT checkbox is the same small size and still turns RIT on/off
- [ ] Host box: drop-down arrow shows in full inside the panel; picking a recent host fills Host
- [ ] Host and Port still edit; Connect / Disconnect works
- [ ] Phones and Digital buttons are the same width and both fit inside the panel
- [ ] Remote button fits inside the panel; Remote on/off still works
- [ ] Slider values at 100 (and RIT −500, AGC 2000ms, Pls 510, ZOOM 4×) have a gap from the thumb
- [ ] Slider track is visibly thicker; rows are not taller
- [ ] Thumb shows gradient shading, no white line at top (left and right panels)
- [ ] Pkts / KA / D5 / Spec line is gone from the bottom of the left panel
- [ ] Pkts line still shows on SETTINGS under TEMPS / STATUS
- [ ] Mouse wheel on every rail slider is still 1 step per notch
- [ ] Phones Vol/Mic, Digital Vol/Mic, ZOOM, RIT Offset, POWER, COMP, AGC, Pls still change the radio and match the readout
- [ ] Digital Vol/Mic still dim and lock while Remote is on
- [ ] Right panel: POWER / COMP / AGC / Pls rows look right, nothing clips
- [ ] S-meter and ALC meter HOLD / Peak checkboxes small, still work
- [ ] CW tab (external keyer, PADDLE, POTENTIA / QSK), QRP cal dummy load, RX IQ UP 24 kHz, SETTINGS SWR, FM Simplex checkboxes look right and toggle
- [ ] S/W window checkboxes (AUTO SNAP, TIME DISPLAY, VIEW GRID, dB LABELS, PEAK MARKER, Show waterfall, Climbing) look right and toggle
- [ ] Remote Audio window (Mute phones, Monitor at radio, Playback EQ) and Debug Log (Pause) checkboxes look right and toggle
- [ ] Default 1280×720: nothing clips or overlaps on either panel

## Commit (commit only, no push, no installer, no GitHub release, no version bump)

Stew's rule: Build commits on this host; Stew pushes. Commit, then stop. Do not push.

```
git add Avalonia/src/MSCC.Avalonia/App.axaml Avalonia/src/MSCC.Avalonia/Views/MainWindow.axaml
git status
git commit -m "Avalonia: left panel fits at minimum size; small checkboxes; slider value gap, thicker track, no thumb line"
```

Add any other file the CheckBox or ComboBox theme truly needs, and only that file. Never add:
`.deb` files, `Avalonia/publish/`, `bin/`, `obj/`, `installers/`, Visual Studio files, or other
people's dirty files.
