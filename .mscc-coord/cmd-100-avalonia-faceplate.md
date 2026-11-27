# cmd-100 — Avalonia faceplate: one steel front panel behind everything + left-panel cleanup

> **STOP — read first.** Build commits only. **No push, no installer, no GitHub Release, no version bump** (Avalonia stays **1.7.08**). Avalonia only: `Avalonia/src/MSCC.Avalonia` (plus one generator script under `Avalonia/tools/`). **Do not touch** `rpi/`, `Solidus/`, `installers/rpi/`, the WPF tree, **`Controls/UiChromeTheme.cs`**, or `mscc-ui/Avalonia-Migration`. No radio protocol commands; this is visual only. Do not commit bin/obj/publish/.deb/VS files or `.mscc-coord/status/ubuntu-stew.md`. Do not pull, merge or rebase. Stew pushes. Ask Stew before any destructive git step.

Follows cmd-099b (Build `c7be7233`). Stew: "This looks outstanding! … The background is what pulls it all together." This command makes the window read as one radio front panel, PowerSDR-skin style: a steel-blue grey faceplate, lit from above, with the displays set into it.

Line numbers are from `c7be7233`; find anchors by name if they moved.

## A. Left-panel cleanup

1. `Views/MainWindow.axaml` ~124–153: delete the **Step** row (the `Grid` with `TextBlock Text="Step"` and the `StepLabel` button bound to `CycleStepCommand`). Keep `CycleStepCommand` in the VM: the VFO button-row STEP (~360) and the VFO A double-click still use it.
2. Heading `FILTERS / STEP` → **`FILTERS`**; comment `<!-- Filters / Step … -->` → `<!-- Filters (live when connected) -->`.
3. Even spacing: the freed row is 24 px. Before cmd-100 the left rail had ~13 px spare at 1024×600 (582 usable, ~569 content), so after A.1 there is ~**37 px** spare.
   - Mark the left rail `StackPanel` with `Classes="mscc-left-rail"` and add a style in `App.axaml`: `StackPanel.mscc-left-rail > TextBlock.mscc-section` → `Margin="0,10,0,1"` (was `0,4,0,1`). Keep `SERVER` at `Margin="0,0,0,1"` (local value wins).
   - 5 headings × 6 px extra = **30 px** used, ~7 px left.
   - At 1024×600 the left `ScrollViewer` must show **no scroll bar**. If one appears, use `0,9,0,1`, then `0,8,0,1`. Report the value used and the measured spare px.
   - Right rail: do not change; report whether it shows a scroll bar at 1024×600.

## B. Faceplate

### B1. Why the faceplate is a layer, not Window.Background
`Controls/UiChromeTheme.cs` (`ApplyToApplication`, ~498–545) replaces `UiWindowBackgroundBrush` / `UiPanelBackgroundBrush` with solid colors from Settings and **sets `MainWindow.Background` directly**. Do not edit that file. So the faceplate is drawn by layers inside the window's root `<Panel>` (MainWindow.axaml line ~22), **in front of** `Window.Background` and **behind** the main `Grid`. `UiWindowBackgroundBrush` stays as the fallback behind it. The Settings "UI background" color no longer shows on the main window (it still applies to the other windows); report this, do not change Settings.

### B2. Named resources (App.axaml, one group)
Add a group right after the `UiVfo*` brushes:

```xml
<!-- ===== Faceplate (UiFace*) =====
     Skin: Steel (default). Later presets (not in this command): Dark, Olive — swap these keys only.
     Light source: above (matches light-bar buttons, cmd-097b): lighter high-center, darker edges. -->
<RadialGradientBrush x:Key="UiFaceBaseBrush" Center="50%,30%" GradientOrigin="50%,22%" RadiusX="85%" RadiusY="95%">
  <GradientStop Offset="0"    Color="#FF4E5C6D"/>
  <GradientStop Offset="0.55" Color="#FF3E4A58"/>
  <GradientStop Offset="1"    Color="#FF262E38"/>
</RadialGradientBrush>
<LinearGradientBrush x:Key="UiFaceSheenBrush" StartPoint="0%,0%" EndPoint="0%,100%">
  <GradientStop Offset="0"    Color="#1FFFFFFF"/>
  <GradientStop Offset="0.006" Color="#10FFFFFF"/>
  <GradientStop Offset="0.35" Color="#00FFFFFF"/>
</LinearGradientBrush>
<RadialGradientBrush x:Key="UiFaceVignetteBrush" Center="50%,45%" GradientOrigin="50%,45%" RadiusX="75%" RadiusY="80%">
  <GradientStop Offset="0.6" Color="#00000000"/>
  <GradientStop Offset="1"   Color="#59000000"/>
</RadialGradientBrush>
<ImageBrush x:Key="UiFaceTextureBrush" Source="avares://MSCC.Avalonia/Assets/face-brushed.png"
            TileMode="Tile" Stretch="None" DestinationRect="0,0,256,256" Opacity="0.55"/>
<SolidColorBrush x:Key="UiFaceHeadingBrush" Color="#FFEEF2F6"/>
<Color x:Key="UiFaceHeadingShadowColor">#C0000000</Color>
<SolidColorBrush x:Key="UiFaceLabelBrush" Color="#FFFFFFFF"/>
<SolidColorBrush x:Key="UiFaceMutedTextBrush" Color="#FFD4DCE4"/>
<SolidColorBrush x:Key="UiFaceWarnTextBrush" Color="#FFFFD08A"/>
<SolidColorBrush x:Key="UiFaceOkTextBrush" Color="#FFA8EBC8"/>
```

Tune the base stops by eye only toward **darker**, never lighter than `#4E5C6D` at the center (text contrast below depends on it).

### B3. Brushed-metal texture (cheap on Pi arm64)
- New script `Avalonia/tools/gen-face-texture.py`: Python 3, **stdlib only** (`zlib`, `struct`, `random`), fixed seed `100`. Writes `Avalonia/src/MSCC.Avalonia/Assets/face-brushed.png`, **256×256 RGBA**.
- Brushed look: for each row, random values smoothed **horizontally** with a wrap-around box blur (radius 24) so streaks run left-right and the tile is seamless left/right; rows independent but each row also averaged 25 % with the row above (wrap at the bottom) so it is seamless top/bottom.
- Pixel: value `v` in −1…+1 → if `v > 0` white `(255,255,255, a)` else black `(0,0,0, a)`, `a = round(|v| × 22)` (max alpha 22/255). No gray pixels (gray would wash the steel color).
- Commit the PNG and the script. Expected PNG size well under 100 KB; report it. (`Assets\**` is already an `AvaloniaResource`.)
- The brush is static, tiled, no blur effect at run time.

### B4. Layers (MainWindow.axaml root `<Panel>`, before the main `Grid`)
```xml
<Border Background="{DynamicResource UiFaceBaseBrush}" IsHitTestVisible="False"/>
<Border Background="{DynamicResource UiFaceTextureBrush}" IsHitTestVisible="False"/>
<Border Background="{DynamicResource UiFaceSheenBrush}" IsHitTestVisible="False"/>
<Border Background="{DynamicResource UiFaceVignetteBrush}" IsHitTestVisible="False"/>
```
The vignette sits under the content, so it never darkens a display, a button or text.

### B5. Panels become layout only
- `App.axaml` `Border.mscc-panel` (~703): `Background` → `Transparent`, `BorderThickness` → `0`. Keep `CornerRadius` and `Padding` (layout unchanged). This covers the left rail, right rail, top meter/VFO row, band row and the Settings tab.
- Tab content roots → `Background="Transparent"`, no border: CW (~487), RX/TX (~688), FAVORITES (~850), QRP CAL (~924), AMP CAL (~1036), RX IQ (~1147), TX IQ (~1246), FREQ CAL (~1349).
- Group cards inside tabs (Borders that hold labels and controls only, e.g. `#FF222222` at ~692/~790/~1464/~1569 and `#FF252525` with `#444` border at ~931/~957/~1043/~1069/~1253) → `Background="Transparent"`, `BorderThickness="0"`. Rule: **a Border that holds data (a list, a readout value, a text box, a graph) stays dark; a Border that only groups labels and controls becomes transparent.** Report the list of Borders changed and kept.
- `TabControl` / `TabItem`: no background on the strip or content area (`Background="Transparent"` in the existing `TabControl` / `TabItem` styles ~712–721). Tab headers: unselected Foreground `UiFaceMutedTextBrush`, selected `UiFaceLabelBrush`; keep fonts and the existing selected indicator.

### B6. Displays stay recessed
- VFOs and both meters: unchanged (bezel + bevels from cmd-098/099b).
- **Spectrum on MAIN** (~478): replace the `Border Background="#FF0A0A12" BorderBrush="#FF333333" BorderThickness="1" CornerRadius="2"` with the same frame as the meters: outer `Border Background="{DynamicResource UiVfoBezelBrush}" CornerRadius="4" Padding="3"`, inner `Grid` = dark `Border #FF0A0A12 CornerRadius 2`, the control, then the two bevel overlays (`UiVfoBevelDarkBrush` `2,2,0,0`, `UiVfoBevelLightBrush` `0,0,2,2`, `IsHitTestVisible="False"`). Do the same for the RX IQ (~1233) and FREQ CAL (~1442) spectrum hosts.
- Time bar (MAIN ~465, `#FF1A1A1A`): stays a dark display box.
- Light-bar buttons, sliders, checkboxes, text boxes, combo boxes: unchanged.

### B7. Section headings: lettering on the panel
`App.axaml` `TextBlock.mscc-section` (~572): Foreground `UiFaceHeadingBrush` (was `UiAccentBrush` green), FontWeight Bold, FontSize **10** (unchanged), plus a crisp 1 px shadow below (light from above):
`<Setter Property="Effect"><DropShadowEffect OffsetX="0" OffsetY="1" BlurRadius="0" Color="{DynamicResource UiFaceHeadingShadowColor}" Opacity="1"/></Setter>`
Zero blur keeps it cheap. If `DynamicResource` on `Color` does not resolve inside the effect, use `StaticResource`. Apply the same heading look to tab-content headings that use `Foreground="#00FFAA"` **and** act as section titles (e.g. `DEFAULT FILTERS` ~745); data values in `#00FFAA` stay green.

### B8. Text contrast on the face (check every label)
Worst case is the lightest face color, the center `#4E5C6D` (relative luminance ≈ 0.104). Target ≥ **4.5 : 1**.

| Foreground now | Contrast on `#4E5C6D` | Action when the text sits on the face |
|---|---|---|
| White / `UiFaceHeadingBrush` / `#EEE` | 6.8 / 6.0 / 5.9 | keep |
| `#00FFAA` / `#0FA` (accent) | 5.2 | keep for values |
| `#FFE14A` (slider values) | 5.2 | keep |
| `#FFCC66` | 4.6 | keep |
| `#D0D0D0` / `#CCC`, `#CCCCCC` | 4.4 / 4.3 | → `UiFaceMutedTextBrush` (4.9) |
| `#FFC000` | 4.2 | → `UiFaceWarnTextBrush` (4.8) |
| `#AAA` | 2.9 | → `UiFaceMutedTextBrush` |
| `#888`, `#888888` | 1.9 | → `UiFaceMutedTextBrush` |
| `#666` | 1.2 | → `UiFaceMutedTextBrush` |
| `#FFAA66` | 3.6 | → `UiFaceWarnTextBrush` |
| `#88CCAA` | 3.7 | → `UiFaceOkTextBrush` (5.0) |

Counts in MainWindow.axaml today: `#AAA` 22, `#888` 13 (+1 `#888888`), `#666` 12, `#D0D0D0` 11, `#CCC` 9 (+4 `#CCCCCC`), `#FFC000` 2, `#FFAA66` 1, `#88CCAA` 1. Change only the ones that sit on the face; text inside a dark box (time bar, readouts, lists, text boxes) keeps its color. Also check: `mscc-label` (white, OK), `UiMutedTextBrush` (`#D0D0D0`, set at run time by UiChromeTheme.cs: do not bind face text to it; use `UiFaceMutedTextBrush`), `mscc-slider-value`, CheckBox content (Fluent default foreground; set `UiFaceLabelBrush` if it renders below 4.5:1), disabled controls (Fluent disabled opacity; report any that become unreadable). Report the full list of changed lines.

## Fit
- Width/height of every element is unchanged: B is colors, layers and borders only (the Settings/tab panels lose a 1 px border, which only adds room). Top row stays 180 + 320 + 180 = **680 ≤ 683** at 1024; top panel **126** high (cmd-099b).
- Spectrum frame: border 1 → bezel padding 3 → the spectrum loses 4 px width and height. Fine at both sizes.
- Left rail: see A.3.

## Render cost (Pi arm64)
Four static full-window layers: two gradients, one tiled 256² bitmap, one gradient. The spectrum/waterfall redraws its own rectangle each frame; the layers behind it are re-composited only inside that rectangle. On stew-HP with the spectrum running, record `top` CPU % for the MSCC.Avalonia process for 30 s before and after (same window size, same band). If it rises more than 3 points, drop the texture layer first and report.

## Checklist (Build)
- [ ] `git fetch` + `git status -sb`; report if origin moved. Do not pull.
- [ ] A.1–A.2 Step row removed, heading `FILTERS`; `CycleStepCommand` still used by the button row and double-click.
- [ ] A.3 left-rail heading spacing; no scroll bar at 1024×600; value used + spare px reported; right rail scroll state reported.
- [ ] B2 `UiFace*` group in App.axaml with the skin comment.
- [ ] B3 script + `face-brushed.png` committed; PNG size reported; tile seamless (no visible seams at 1280×720).
- [ ] B4 four layers in the root Panel; `UiChromeTheme.cs` untouched.
- [ ] B5 panels/tab roots/group cards transparent; list of Borders changed vs kept.
- [ ] B6 spectrum hosts framed like the meters (MAIN, RX IQ, FREQ CAL).
- [ ] B7 headings lettered with 1 px shadow.
- [ ] B8 contrast fixes; list of changed lines.
- [ ] `dotnet build -c Release --no-incremental`: 0 errors, no new warnings.
- [ ] Screenshots at **1024×600** and **1280×720** of MAIN, CW, RX/TX, SETTINGS, QRP CAL; nothing clipped.
- [ ] CPU before/after (Render cost).
- [ ] One commit: `Avalonia: steel faceplate (UiFace* brushes, brushed texture, vignette); panels transparent; spectrum framed; left Step removed`.
- [ ] Report commit hash, files changed, measurements, the Border list and the contrast list.

## Stew's check (printable)
- [ ] One continuous steel-blue faceplate: lighter top-center, darker edges, faint brushed grain, no seams.
- [ ] No panel boxes or borders around the rails, band row, top row or tabs; headings read as lettering on the panel.
- [ ] VFOs, meters and spectrum look set into the panel (same frame on all).
- [ ] Every label, checkbox, slider value and tab header readable on the faceplate (all tabs).
- [ ] Left panel: no Step button; heading reads FILTERS; sections evenly spaced; no scroll bar at 1024×600.
- [ ] STEP in the VFO button row and VFO A double-click still change the step.
- [ ] Light-bar buttons unchanged.
- [ ] Spectrum/waterfall as smooth as before (also check later on the Pi).
