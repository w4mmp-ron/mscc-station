# cmd-098: Avalonia VFO faces — recessed backlit LCD with band-plan strip

Host: stew-HP (Ubuntu amd64, host id ubuntu-stew). Maintained client: `Avalonia/` (`MSCC.Avalonia.sln`,
package mscc-avalonia, 1.7.08). Follows cmd-097b (pushed). Do NOT edit `mscc-ui/Avalonia-Migration`
(frozen), `rpi/`, `Solidus/`, `installers/rpi/`, or the WPF tree.

## STOP

- Commit only. Do not push. Stew pushes.
- No installer, no `.deb`, no GitHub Release.
- No version bump. Leave `MSCC.Avalonia.csproj` at 1.7.08. Do not edit
  `packaging/mscc-avalonia/DEBIAN/control`, the Addendum, or `installers/windows/CHANGELOG.md`.
- Do not touch `rpi/`, `Solidus/`, `installers/rpi/`, WPF, or `Controls/UiChromeTheme.cs`.
- Do not pull, merge, or rebase. If `origin/main` moved, keep going on the local tree; Stew pulls
  before he pushes.
- **Keep the VFO sizes exactly:** VFO A `Width="176" Height="92"`, VFO B `Width="156" Height="92"`.
- **No TX button or TX indicator** in either VFO face.
- Do not change tuning: click-to-select, digit-hover wheel (step = digit place), VFO A
  double-click cycles Step, and the left Step fallback must all work as now.
- Do not edit `Assets/bandplan-us.csv` or `Assets/bandplan-us.SOURCE.md` (committed with this
  brief). Read them only.
- `.mscc-coord/status/ubuntu-stew.md` is a local status file. Do not commit it.
- Never commit `bin/`, `obj/`, `Avalonia/publish/`, `.deb`, or Visual Studio files.

## Goal (Stew, 2026-11-27)

Turn both VFO faces into a PowerSDR-style **backlit LCD**: light cream/yellow face with a soft glow,
**large dark digits right-aligned** like `14.074 000` with the **last three (Hz) digits smaller**,
and a **lower strip** naming the band and the band section you are tuned to, from PowerSDR's band
text table (for example `20M FT8 DIGU`, `20M Extra Phone`, `Broadcast AM Med Wave`,
`60M Channel 1`, `WWV Day Time`). The LCD sits **recessed** in the panel: dark top/left inner
edge, light bottom/right inner edge, like a display sunk into the front panel. No TX button.
Colors are named `UiVfo*` resources so a later skins system only swaps values.

## Band-plan data (committed with this brief)

- `Avalonia/src/MSCC.Avalonia/Assets/bandplan-us.csv` — 431 rows + header
  `low_hz,high_hz,name,tx`, sorted by `low_hz`, whole Hz. Covers 60 kHz (WWVB) to 54 MHz.
  95 rows are spot frequencies (`low_hz == high_hz`). Already picked up by
  `<AvaloniaResource Include="Assets\**" />` (`MSCC.Avalonia.csproj` 17).
- `Avalonia/src/MSCC.Avalonia/Assets/bandplan-us.SOURCE.md` — source and license note.
- Source: ke9ns PowerSDR v2.8.0 `Console/database.cs`, US BandText (`AddBandTextTable()` +
  `AddBandTextSWB()`), GPL v2 or later (FlexRadio Systems 2003-2013). Keep the SOURCE note with
  the CSV.
- CSV format: RFC 4180. Seven names contain commas and are in double quotes (for example
  `"6M CW, SSB & Digital"`). No name contains a double quote. The parser must handle quoted
  fields.

Samples:

```
135700,137799,2.2kM CW & Narrow Band,1
472000,478999,630M CW/JT9/WSPR/Narrow,1
530001,1609999,Broadcast AM Med Wave,0
1810001,1837999,160M CW,1
5332000,5332000,60M Channel 1,1
10000000,10000000,WWV Day-Evening Time,0
14074000,14074000,20M FT8 DIGU,1
14150000,14174999,20M Extra Phone,1
```

## Findings (tree at bbb7e5b6, synced with origin)

All paths are under `Avalonia/src/MSCC.Avalonia/`.

**VFO A** (`Views/MainWindow.axaml` 211–244): `Border x:Name="VfoAPanel"` 176×92,
`Background="#FF0A1A0A"`, CornerRadius 4, Padding `4,3`, classes `mscc-vfo-active` /
`mscc-vfo-idle` bound to `UseVfoA`, events `VfoAPanel_OnPointerPressed`,
`VfoAPanel_OnPointerWheelChanged`, `VfoAPanel_OnDoubleTapped`. Inside a StackPanel:
- header TextBlock "VFO A · digit + wheel" (FontSize 8),
- `TextBlock x:Name="VfoAFreqText"` `Text="{Binding FrequencyDisplayMhz}"` FontSize 26 Bold
  Consolas, centered, events `VfoAFreqText_OnPointerMoved`, `VfoFreqText_OnPointerExited`,
  `VfoAFreqText_OnPointerWheelChanged`,
- row: `{Binding ModeText}` (11 Bold, accent) and `{Binding VfoTuneStepDisplay}` (9).

**VFO B** (247–276): `Border x:Name="VfoBPanel"` 156×92, `Background="#FF0A0A12"`, classes bound
to `UseVfoB` / `UseVfoA`, events `VfoBPanel_OnPointerPressed`, `VfoBPanel_OnPointerWheelChanged`.
Header "VFO B · digit + wheel", `TextBlock x:Name="VfoBFreqText"`
`Text="{Binding VfoBDisplayMhz}"` FontSize 22, same pointer events (B versions), then
`{Binding VfoBModeText}` (10 Bold).

**Active/idle frame:** `App.axaml` 606–615: active = `UiAccentBrush` border 2, Opacity 1; idle =
`UiAccentMutedBrush` border 1, Opacity 0.78. Keep these as the "which VFO is active" cue.

**View model** (`ViewModels/MainViewModel.cs`): `[ObservableProperty]` `_frequencyDisplayMhz`
(330), `_modeText` (331), `_vfoBDisplayMhz` (536), `_vfoBModeText` (537). Display strings come
from `FormatMhz(hz)` (8971: `"0.000000"`, invariant). No `OnFrequencyDisplayMhzChanged` /
`OnVfoBDisplayMhzChanged` partials exist yet. `VfoTuneStepDisplay` at 2496.

**Digit hit-test** (`Views/MainWindow.axaml.cs`): `HandleDigitWheel` 273, `UpdateHoverStep` ~297,
`TryGetDigitStepHz` 337–420. It measures `freqTb.Text` with the TextBlock's font, assumes the text
is **centered** in the TextBlock, finds the digit under the pointer, and returns
`10^(digits to the right)`. Splitting the frequency into a big part and a small Hz part breaks
this unless the plan below is followed.

**Top row width at 1024×600 (already tight, not changed by this cmd):** center column 693 px −
panel border 2 − Padding `4,2` (188) 8 = **683 px**; fixed content S-meter 180 + VFO A 176 + VFO B
156 + ALC 180 = **692 px**. It overflows by about 9 px today. This cmd keeps every size, so it
does not make it worse. Build: look at 1024×600 and **report** what clips (likely the ALC
meter's right edge). Do not fix it in this cmd.

## Change plan

### 1. Resources (`App.axaml`, `Application.Resources`, after the `UiBar*` brushes)

Brushes with literal colors (skins swap the whole brush by key). Start values; tune by eye.

| Key | Use | Start value |
|-----|-----|-------------|
| `UiVfoBezelBrush` | VFO frame background around the LCD | `#FF101214` |
| `UiVfoLcdBrush` | backlit face, `RadialGradientBrush` (center 50%,40%, radius ~70%) | center `#FFFFF4C4` → edge `#FFE3CF7C` |
| `UiVfoLcdShadeBrush` | inner shadow at the top of the LCD (6 px) | `#59000000` → `#00000000` |
| `UiVfoBevelDarkBrush` | recess, top and left inner edge (2 px) | `#E0000000` |
| `UiVfoBevelLightBrush` | recess, bottom and right inner edge (2 px) | `#80FFFFFF` |
| `UiVfoDigitBrush` | big digits | `#FF1E1C12` |
| `UiVfoDigitHzBrush` | small Hz digits | `#FF3C3824` |
| `UiVfoLabelBrush` | "VFO A" / "VFO B" label, step text | `#FF5A5438` |
| `UiVfoModeBrush` | mode text | `#FF1E1C12` |
| `UiVfoStripBrush` | lower strip background | `#26000000` |
| `UiVfoStripTextBrush` | lower strip text | `#FF2A281A` |

### 2. Band-plan service (`Services/BandPlanService.cs`, new)

- Static class `BandPlanService`. Loads `avares://MSCC.Avalonia/Assets/bandplan-us.csv` once
  (lazy, `AssetLoader.Open`), parses RFC 4180 (quoted fields), skips the header.
- `public static string Describe(long hz)`:
  1. A spot row with `low_hz == high_hz == hz` wins.
  2. Else the **narrowest** range with `low_hz <= hz <= high_hz` (ties: first in file).
  3. Else return **`"Out of Band"`** (PowerSDR's own text, `DB.BandText`).
  4. `hz <= 0` → `""`.
- No exceptions out of it: if the asset fails to load, log once (existing debug log) and return
  `""` for every lookup.
- Keep the `tx` column parsed but unused.

### 3. View-model properties (`ViewModels/MainViewModel.cs`)

Add `[ObservableProperty]` strings: `_vfoAMhzKhz`, `_vfoAHz`, `_vfoABandText`, `_vfoBMhzKhz`,
`_vfoBHz`, `_vfoBBandText`.

Add partials:

- `partial void OnFrequencyDisplayMhzChanged(string value)` → split and look up for VFO A.
- `partial void OnVfoBDisplayMhzChanged(string value)` → same for VFO B.

Split rule: `value` is `"0.000000"` format. `MhzKhz` = everything up to and including the third
decimal (`"14.074"`); `Hz` = the last three digits (`"000"`). Band text: parse `value` invariant to
Hz (`decimal × 1_000_000`, rounded) → `BandPlanService.Describe(hz)`. Do not change
`FrequencyDisplayMhz`, `VfoBDisplayMhz` or `FormatMhz`; everything else that uses them stays.

### 4. VFO face layout (`Views/MainWindow.axaml` 211–276)

For **both** VFOs, keep the outer `Border` (name, Grid.Column, Width, Height, CornerRadius,
Cursor, classes, ToolTip, Focusable, all events) and change only:

- `Background` → `{DynamicResource UiVfoBezelBrush}`; Padding → `3`.
- Content → a `Grid` that is the LCD:
  1. `Border` (the face): Background `UiVfoLcdBrush`, CornerRadius 2, ClipToBounds True.
  2. Recess: `Border` BorderThickness `2,2,0,0` BorderBrush `UiVfoBevelDarkBrush` and `Border`
     BorderThickness `0,0,2,2` BorderBrush `UiVfoBevelLightBrush`, both CornerRadius 2,
     IsHitTestVisible False, drawn over the face.
  3. Top inner shadow: `Border` Height 6, VerticalAlignment Top, Margin `2,2,2,0`, Background
     `UiVfoLcdShadeBrush`, IsHitTestVisible False.
  4. Content Grid, Margin `5,2,5,3`, `RowDefinitions="13,*,15"`:
     - **Row 0:** left `TextBlock` "VFO A" / "VFO B" (FontSize 8, Bold, `UiVfoLabelBrush`); right a
       horizontal StackPanel (Spacing 5): mode (`ModeText` / `VfoBModeText`, FontSize 10 Bold,
       `UiVfoModeBrush`) and, on VFO A only, `VfoTuneStepDisplay` (FontSize 8, `UiVfoLabelBrush`).
       The old "· digit + wheel" header text goes (the tooltip already explains it).
     - **Row 1:** horizontal StackPanel, `HorizontalAlignment="Right"`,
       `VerticalAlignment="Center"`, Spacing 0:
       - `TextBlock x:Name="VfoAFreqText"` (B: `VfoBFreqText`) `Text="{Binding VfoAMhzKhz}"`
         (B: `VfoBMhzKhz`), FontSize **26** (B: **22**), Bold, `Consolas, Courier New, monospace`,
         `UiVfoDigitBrush`, `VerticalAlignment="Bottom"`, **`Tag="3"`**, same three pointer
         events as now.
       - `TextBlock x:Name="VfoAFreqHzText"` (B: `VfoBFreqHzText`) `Text="{Binding VfoAHz}"`
         (B: `VfoBHz`), FontSize **18** (B: **15**), Bold, same font, `UiVfoDigitHzBrush`,
         `VerticalAlignment="Bottom"`, Margin `3,0,0,2`, **`Tag="0"`**, the same three pointer
         events as the big digits.
       Do not give either TextBlock a Width; each must size to its text (the hit-test then sees
       `textLeft = 0`).
     - **Row 2:** strip `Border` Background `UiVfoStripBrush`, CornerRadius 1, containing a
       `TextBlock` `Text="{Binding VfoABandText}"` (B: `VfoBBandText`), FontSize 9, SemiBold,
       `UiVfoStripTextBrush`, HorizontalAlignment Center, VerticalAlignment Center,
       `TextTrimming="CharacterEllipsis"`, `ToolTip.Tip` bound to the same text.
- Nothing about TX anywhere in the face.

### 5. Digit hit-test offset (`Views/MainWindow.axaml.cs` `TryGetDigitStepHz` 337–420)

After the step is computed, multiply it by `10^offset`, where `offset` is
`int.TryParse(freqTb.Tag?.ToString(), out var o) ? o : 0`. Big digits (`Tag="3"`): the last digit
(`14.07[4]`) gives 1000 Hz, the MHz digit gives 1 000 000 Hz — same as today. Hz digits
(`Tag="0"`): 100 / 10 / 1 Hz. Over the `.` keep the existing "digit to the left" rule. Wheel and
hover use the same handlers, so `HandleDigitWheel` and `UpdateHoverStep` need no other change.
Not over a digit on VFO A still falls back to the left Step.

### 6. Fit

- VFO A and B outer sizes unchanged (176×92, 156×92). Rows inside: 13 + ~38 + 15 + margins in the
  86 px inside the 3 px padding: fits.
- Width check, VFO A inside ≈ 176 − 6 − 10 = 160 px: `14.074` at 26 px monospace ≈ 86–94 px +
  `000` at 18 px ≈ 30–33 px + 3 = ~130 px. VFO B inside ≈ 140 px: 22/15 px ≈ 110 px. `54.000 000`
  is the widest real value; it fits both.
- Band text longer than the strip (for example `160M Phone/Data (Japan Data)` on VFO B) trims
  with an ellipsis and shows in full in the tooltip.
- 1024×600: see Findings; report the existing top-row overflow, do not fix it here.

## Acceptance

- Both VFOs show a cream backlit LCD sunk into a dark bezel (dark top/left inner edge, light
  bottom/right), with large dark right-aligned digits and smaller Hz digits.
- The strip shows the PowerSDR band text for the current frequency on each VFO and updates as you
  tune; `Out of Band` when no row matches.
- Active VFO frame (accent border) and idle dimming still work. No TX element.
- Digit wheel on every digit (MHz down to 1 Hz) steps by that digit's place on both VFOs; hover
  shows the step; VFO A off-digit wheel uses the left Step; double-click cycles Step.
- VFO sizes unchanged. All `UiVfo*` brushes exist and the faces use only them for color.

## Build and smoke (stew-HP, amd64)

Do not build a `.deb`. Run from source:

```
cd /home/stew/Documents/GitHub/mscc-station/Avalonia
dotnet build src/MSCC.Avalonia/MSCC.Avalonia.csproj -c Release
dotnet run --project src/MSCC.Avalonia/MSCC.Avalonia.csproj -c Release --no-build
```

Checklist (connect to the radio for tuning items):

- [ ] Release build runs on stew-HP with no new warnings
- [ ] VFO A and VFO B: cream backlit LCD, recessed (dark top/left, light bottom/right)
- [ ] Digits dark, right-aligned; last three Hz digits smaller, like `14.074 000`
- [ ] No TX button or TX light in either VFO
- [ ] VFO sizes look the same as before (176×92 and 156×92)
- [ ] 14.074000 → strip `20M FT8 DIGU`; 14.160000 → `20M Extra Phone`; 14.030000 → `20M CW`
- [ ] 1.000000 (GEN or tune) → `Broadcast AM Med Wave`; 10.000000 → `WWV Day-Evening Time`
- [ ] 5.332000 → `60M Channel 1`; 5.340000 → `60M General`
- [ ] Geminus or LF tune: 0.136000 → `2.2kM CW & Narrow Band`; 0.475000 → `630M CW/JT9/WSPR/Narrow`
- [ ] A frequency with no row (for example 4.747000) → `Out of Band`
- [ ] VFO B strip follows VFO B's frequency (split / FM offset too)
- [ ] Wheel over each big digit: MHz, 100 kHz, 10 kHz, 1 kHz steps; over each small digit: 100, 10, 1 Hz (both VFOs)
- [ ] Hover over digits updates the step readout; leaving the digits clears it
- [ ] VFO A wheel off the digits uses the left Step; double-click VFO A cycles Step
- [ ] Click VFO A / VFO B selects it; active one has the accent frame, idle one dims
- [ ] Mode text in each VFO matches the mode buttons
- [ ] Long band text trims with … and shows in full on hover
- [ ] 1024×600: VFOs unchanged; report what (if anything) clips in the top row
- [ ] 1280×720: top row looks right, nothing overlaps

## Commit (commit only, no push, no installer, no GitHub release, no version bump)

Stew's rule: Build commits on this host; Stew pushes. Commit, then stop. Do not push.

```
git add Avalonia/src/MSCC.Avalonia/App.axaml Avalonia/src/MSCC.Avalonia/Views/MainWindow.axaml Avalonia/src/MSCC.Avalonia/Views/MainWindow.axaml.cs Avalonia/src/MSCC.Avalonia/ViewModels/MainViewModel.cs Avalonia/src/MSCC.Avalonia/Services/BandPlanService.cs
git status
git commit -m "Avalonia: recessed backlit LCD VFO faces with PowerSDR band-plan strip"
```

Never add: `.deb` files, `Avalonia/publish/`, `bin/`, `obj/`, `installers/`, Visual Studio files,
`.mscc-coord/status/ubuntu-stew.md`, or other people's dirty files.
