# cmd-097: Avalonia light-bar buttons (band, mode, filter/step, left/right panels) + filter spacing

Host: stew-HP (Ubuntu amd64, host id ubuntu-stew). Maintained client: `Avalonia/` (`MSCC.Avalonia.sln`,
package mscc-avalonia, 1.7.08). Builds on cmd-096 (`a7dbe480`). Do NOT edit `mscc-ui/Avalonia-Migration`
(frozen), `rpi/`, `Solidus/`, `installers/rpi/`, or the WPF tree.

## STOP

- Commit only. Do not push. Stew pushes.
- No installer, no `.deb`, no GitHub Release.
- No version bump. Leave `MSCC.Avalonia.csproj` at 1.7.08. Do not edit
  `packaging/mscc-avalonia/DEBIAN/control`, the Addendum, or `installers/windows/CHANGELOG.md`.
- Do not touch `rpi/`, `Solidus/`, `installers/rpi/`, or WPF.
- Do not pull, merge, or rebase. If `origin/main` has commits you do not have, keep going on the
  local tree; Stew pulls before he pushes. (When this brief was written stew-HP was 7 ahead, 0
  behind; cmd-094/095/096 not yet pushed.)
- Do not change any binding, Command, CommandParameter, Click handler, IsEnabled, ToolTip, or the
  `mscc-selected` / `mscc-tx-on` class bindings. This cmd is look and layout only.
- Do not edit `Controls/UiChromeTheme.cs` (Settings → UI Appearance). See "Settings button color".
- Scope is the band bar, the left panel and the right panel of the main window. Leave the tab pages
  (RX/TX, FAVORITES, QRP CAL, AMP CAL, RX IQ, TX IQ, FREQ CAL, SETTINGS), the S/W window, Remote
  Audio, and Debug Log on the gold buttons. They are a follow-up cmd.
- Never commit `bin/`, `obj/`, `Avalonia/publish/`, `.deb`, or Visual Studio files.

## Goal (Stew, 2026-11-27)

Stew's references: a PowerSDR-style band grid and a black-button panel. What he wants:

- Every button has a thin **indicator slot** near the bottom. Off = dark grey slot. Selected = the
  slot lights **blue** (blue is the standard).
- The **button face does not change** when selected. Today the whole button turns a darker gold;
  that goes away.
- Face: **dark glossy gradient with a shiny upper edge**, like the slider knob. A **subtle raised
  edge** (1 px lighter top/left, 1 px darker bottom/right) is included, kept subtle.
- Button text moves **up about 3 px** to make room for the slot.
- **Every color is a named resource**, so a later skins system only swaps values. The face does
  not need to be black.
- Left panel FILTERS / STEP buttons get the **same spacing as the right panel buttons**.
- cmd-096 fit rule still holds: at the minimum window (1024×600, `MainWindow.axaml` 14) the left
  panel has no scroll bar and Clear RIT shows in full.

Stew's band picture also shows 6 / VHF+ / WWV. MSCC's band list stays as it is (2200, 630, 160–10,
GEN). This cmd changes the look only.

## Findings (tree at a7dbe480)

All paths are under `Avalonia/src/MSCC.Avalonia/`.

**How buttons and "selected" work now.** All main-window buttons are plain `Button`s with
`Classes="mscc-gold"` (114 uses in `Views/MainWindow.axaml`). There are no ToggleButtons. State is
two classes bound to the view model:

- `Classes.mscc-selected="{Binding …}"`: latched / chosen (bands, modes, CMP, NB/NR/AN/MON, NB and
  NR levels, Phones/Digital, Remote). Styles at `App.axaml` 362–384 turn the whole face
  `UiButtonSelectedBrush` (dark gold) and the text white.
- `Classes.mscc-tx-on="{Binding …}"`: transmitting (PTT, TUN, AMP). Styles at `App.axaml` 324–360
  turn the whole face red.
- Base gold look: `App.axaml` 277–322 (Button + `/template/ ContentPresenter#PART_ContentPresenter`,
  because Fluent paints the face on the presenter). `Button.mscc-stub` at 397 is not used in scope.

No code-behind or view-model code adds or removes these classes (searched `*.cs` for `"mscc-`).
`MainWindow.axaml.cs` only has Click handlers (`LogButton_OnClick` 443, `SwButton_OnClick` 462).

**Settings button color.** `Controls/UiChromeTheme.cs` 498–533 (`ApplyToApplication`) rewrites
`UiButtonFaceBrush`, `UiButtonBorderBrush`, `UiButtonHoverBrush`, `UiButtonTextBrush`,
`UiButtonSelectedBrush`, `UiButtonSelectedBorderBrush` from Settings → UI Appearance → Button. The new
light-bar buttons use their own `UiBar*` resources, so the Settings Button color will no longer
change them; it still changes the gold buttons left on the tabs and other windows. Wiring `UiBar*`
into Settings is the skins cmd, not this one.

**Buttons in scope (`Views/MainWindow.axaml`):**

| Pass | Buttons | Lines | State class today |
|------|---------|-------|-------------------|
| 1 Band bar | 2200, 630, 160, 80, 60, 40, 30, 20, 17, 15, 12, 10, GEN (13) | 291, 298, 306, 311, 316, 321, 326, 331, 336, 341, 346, 351, 356 (Grid 288, `ColumnSpacing="2"`, MinHeight 26) | `mscc-selected` = `BandIs…` |
| 2 Mode | LSB, USB, DIG-U, CW, AM, FM (6) | 1639, 1642, 1645, 1649, 1652, 1655 (UniformGrid 1638, Margin 2, MinHeight 28) | `mscc-selected` = `ModeIs…` |
| 3 Filter/step | Lo, Hi, CW, Step (4) | 128, 134, 140, 146 (Grids 126, 132, 138, 144: `24,*`, Height 22, Margin `0,0,0,1`, button Width 64) | none (cycle buttons) |
| 4 Left panel | Connect/Disconnect 32; Phones 106; Digital 111; Remote 117; Clear RIT 171 | | Phones/Digital/Remote `mscc-selected`; Connect, Clear RIT none |
| 4 Right panel | PTT 1581, TUN 1586, AGC 1591, AMP 1595, CMP 1600, S/W 1605; NB 1668, NR 1672, AN 1676, MON 1680; NB Thr LOW/MED/HIGH 1695/1699/1703; NR LOW/MED/HIGH 1710/1714/1718; CW − 1726, + 1731, pitch 1734; LOG 1739; Save settings 1742 | | PTT/TUN/AMP `mscc-tx-on`; CMP, NB/NR/AN/MON, levels `mscc-selected`; rest none |

RIT (left panel, line 160) and FM Simplex (right, 1663) are CheckBoxes, not buttons. They keep the
cmd-096 checkbox.

**Right panel button spacing (the reference).** OPERATE (1580) and MODE (1638) buttons each have
`Margin="2"` in a UniformGrid, so the gap between two buttons is **4 px**, sideways and up/down.
The left panel FILTERS / STEP rows have a **1 px** gap (`Margin="0,0,0,1"`, buttons fill the 22 px
row). That is the mismatch Stew sees. The AUDIO row (105) has 4 px sideways (Spacing 4) and 3 px
below.

## Change plan

### 1. Resources (`App.axaml`, `Application.Resources`, after the slider brushes at 27–42)

Add these brushes. Names are fixed; the values are a starting point Build may tune by eye. Use
plain `SolidColorBrush` / `LinearGradientBrush` with literal colors (no DynamicResource inside a
brush), so a skins system later swaps the whole brush by key, the same way `UiChromeTheme.SetBrush`
does today.

| Key | Use | Start value |
|-----|-----|-------------|
| `UiBarFaceBrush` | face, vertical gradient | `#3C424C` → `#1C1F24` (0.5) → `#0B0C0E` |
| `UiBarFaceHoverBrush` | face under pointer | `#4A515C` → `#262A31` (0.5) → `#15171B` |
| `UiBarFacePressedBrush` | face while pressed | `#101114` → `#1A1D22` (0.5) → `#24282E` |
| `UiBarShineBrush` | shiny upper edge, top ~45% of the face | `#55FFFFFF` → `#00FFFFFF` |
| `UiBarOutlineBrush` | 1 px outer outline | `#FF000000` |
| `UiBarEdgeLightBrush` | raised edge, top/left | `#66707A86` |
| `UiBarEdgeDarkBrush` | raised edge, bottom/right | `#CC000000` |
| `UiBarSlotOffBrush` | indicator slot, off | `#3A3D42` |
| `UiBarSlotOnBrush` | indicator lit (blue standard), gradient | `#8CC8FF` → `#2E8BFF` (0.5) → `#1A5FD0` |
| `UiBarSlotTxBrush` | indicator lit while transmitting | `#FF6B60` → `#E53935` |
| `UiBarTextBrush` | text | `#F0F0F0` |
| `UiBarTextTxBrush` | text while transmitting | `#FF8A80` |
| `UiBarTextDisabledBrush` | text when disabled | `#8A8A8A` |

### 2. One ControlTheme (`App.axaml`, `Application.Resources`, after `MsccHostPickTheme`)

`<ControlTheme x:Key="MsccLightBarButtonTheme" TargetType="Button">`, a full template (not based on
Fluent, so Fluent's grey hover cannot leak in). Applied by a style `Button.mscc-bar` that sets
`Theme` to it (put the style in `Application.Styles` next to the `mscc-gold` styles).

Setters: Background `UiBarFaceBrush`, Foreground `UiBarTextBrush`, FontWeight Bold, FontSize 10,
Padding `4,0`, MinHeight 18, HorizontalContentAlignment Center, VerticalContentAlignment Center,
CornerRadius 3.

Template, outside in:

1. Outer `Border` (CornerRadius 3, BorderThickness 1, BorderBrush `UiBarOutlineBrush`,
   Background `{TemplateBinding Background}`, ClipToBounds True) holding a `Grid` with
   `RowDefinitions="0.45*,0.55*"`.
2. `Border#RaisedLight` (RowSpan 2, BorderThickness `1,1,0,0`, BorderBrush `UiBarEdgeLightBrush`,
   CornerRadius 2) and `Border#RaisedDark` (RowSpan 2, BorderThickness `0,0,1,1`, BorderBrush
   `UiBarEdgeDarkBrush`, CornerRadius 2). Subtle: 1 px only.
3. `Border#Shine` in row 0, Margin `1,1,1,0`, Background `UiBarShineBrush`, CornerRadius `2,2,0,0`,
   IsHitTestVisible False.
4. `ContentPresenter#PART_ContentPresenter` (RowSpan 2) with Content / ContentTemplate / Padding /
   Foreground / FontSize / FontWeight / HorizontalContentAlignment / VerticalContentAlignment
   TemplateBindings, RecognizesAccessKey True, **`Margin="0,0,0,6"`**. That moves the text center
   up 3 px, clear of the slot.
5. `Border#LightBar` (RowSpan 2): **Height 3**, VerticalAlignment Bottom, **Margin `5,0,5,2`**,
   CornerRadius 1, Background `UiBarSlotOffBrush`, IsHitTestVisible False.

State styles inside the ControlTheme (all with `^`):

- `:pointerover` → Background `UiBarFaceHoverBrush`.
- `:pressed` → Background `UiBarFacePressedBrush`; swap the edges (RaisedLight BorderBrush
  `UiBarEdgeDarkBrush`, RaisedDark BorderBrush `UiBarEdgeLightBrush`) so it looks pushed in.
- `.mscc-selected /template/ Border#LightBar` → Background `UiBarSlotOnBrush`. Face and text do not
  change.
- `.mscc-tx-on /template/ Border#LightBar` → Background `UiBarSlotTxBrush`; `.mscc-tx-on` →
  Foreground `UiBarTextTxBrush`. Declare after `.mscc-selected` so transmit wins when both apply.
  Face does not change.
- `:disabled` → Opacity 0.45 and Foreground `UiBarTextDisabledBrush`. The slot keeps its state
  (dimmed by the opacity).

Explicit attributes on the buttons (FontSize 9/8/11, Padding `2,1` / `1,0`, Margin, MinHeight,
Width, Height) keep working; the template must use the TemplateBindings above.

### 3. Apply in passes (`Views/MainWindow.axaml`)

In every button listed in the table in Findings, replace `Classes="mscc-gold"` with
`Classes="mscc-bar"`. Keep every `Classes.mscc-selected` / `Classes.mscc-tx-on` binding as it is.
Do it in this order, building and looking after each pass:

1. Band bar (13).
2. Mode (6).
3. Filter / step (4), with the spacing change in item 4.
4. Left panel (Connect, Phones, Digital, Remote, Clear RIT) and right panel (OPERATE 6, NB/NR/AN/MON,
   NB and NR level rows, CW −/+/pitch, LOG, Save settings).

**Action buttons, decided:** buttons with no selected or transmit state (Connect/Disconnect, Clear
RIT, Lo/Hi/CW/Step, AGC cycle, S/W, CW −/+/pitch, LOG, Save settings) get the same face **and show
the slot, always off (dark grey)**. Every button in a panel then has the same shape and text height.
No new bindings are added in this cmd (Connect does not light when connected).

Remote (117) has `Classes.mscc-selected="{Binding RemoteAudio}"`, so it is a toggle: its slot
lights blue while Remote is on. PTT/TUN/AMP light red.

Leave `Button.mscc-gold` and its styles in `App.axaml` for the tab pages and other windows.

### 4. FILTERS / STEP spacing = right panel (`Views/MainWindow.axaml` 126–149)

Match the right panel's 4 px gap. Stew allowed 1–2 px off each button height to make room.

- Each of the 4 row Grids (126, 132, 138, 144): `Height="22" Margin="0,0,0,1"` →
  **`Height="24" Margin="0"`**.
- Each button (128, 134, 140, 146): add **`Height="20" Margin="0,2" VerticalAlignment="Center"`**.
  Keep `Width="64"` and `HorizontalAlignment="Left"`.
- Result: 20 px buttons, 4 px between them (2 + 2), same as OPERATE / MODE.
- AUDIO row (105): `Margin="0,0,0,3"` → **`Margin="0,0,0,4"`** so Phones/Digital sit 4 px above
  Remote, like the right panel.

### 5. Fit rule and pixel budget (left panel, 1024×600)

Usable height 582 px (600 − 8 grid margin − 10 border/padding). Estimate after cmd-096: **~564 px**
to the bottom of Clear RIT (Stew sees room under Clear RIT).

| Change | px |
|--------|----|
| FILTERS / STEP: 4 rows 23 → 24 | +4 |
| AUDIO row bottom margin 3 → 4 | +1 |
| Light-bar template (heights unchanged: Connect 24, Phones/Digital/Remote 24, Clear RIT 22) | 0 |
| **New total** | **~569 px, ~13 px spare** (~2 px if the status line wraps to 2 lines) |

If Clear RIT is not fully visible at 1024×600, in this order: (a) trim `TextBlock.mscc-section`
top margin (`App.axaml`, `0,4,0,1`) to `0,3,0,1` (6 headers on the left panel, 6 px); (b) filter
buttons 20 → 19 and rows 24 → 23 (4 px). Do not shrink slider rows, the slot, or the gap below 4 px.

Right panel: heights do not change, so its layout is unchanged at 1024×600 and 1280×720.

## Acceptance

- Band, mode, filter/step and left/right panel buttons have the dark glossy face, a shiny top, a
  subtle raised edge, and a 3 px slot near the bottom. Text sits above the slot, not touching it.
- Selected = blue slot, face unchanged. Transmit (PTT/TUN/AMP) = red slot and red text, face
  unchanged. Action buttons = grey slot always.
- All 13 `UiBar*` brushes exist in `App.axaml` and the template uses only them for color.
- Every button still does exactly what it did; band/mode/NB/NR/AN/MON/CMP/levels/Phones/Digital/
  Remote still show the right selection.
- FILTERS / STEP buttons have a 4 px gap, like the right panel.
- 1024×600: no left-panel scroll bar, Clear RIT in full. 1280×720: nothing clips on either panel or
  the band bar.
- Tab pages, S/W, Remote Audio and Debug Log buttons are still gold.

## Build and smoke (stew-HP, amd64)

Do not build a `.deb`. Run from source:

```
cd /home/stew/Documents/GitHub/mscc-station/Avalonia
dotnet build src/MSCC.Avalonia/MSCC.Avalonia.csproj -c Release
dotnet run --project src/MSCC.Avalonia/MSCC.Avalonia.csproj -c Release --no-build
```

Checklist (connect to the radio for the "still works" items):

- [ ] Release build runs on stew-HP with no new warnings in the XAML
- [ ] Band bar: every band button has the dark glossy face and a grey slot
- [ ] Band bar: the current band's slot is lit blue; the face did not change color
- [ ] Band bar: clicking each band moves the blue bar and changes band; GEN too
- [ ] Band bar: LF (2200/630) and HF buttons dim when disabled for the radio type
- [ ] Mode: LSB, USB, DIG-U, CW, AM, FM each light blue when chosen and set the mode
- [ ] FILTERS / STEP: Lo, Hi, CW, Step still cycle; slot stays grey
- [ ] FILTERS / STEP: gap between buttons looks the same as OPERATE / MODE on the right
- [ ] Button text sits above the slot (not touching it) on 20, 22, 24, 26 and 28 px buttons
- [ ] NB / NR level rows (LOW / MED / HIGH, small 18 px buttons): text readable, blue bar on the chosen level
- [ ] Phones / Digital: the chosen path lights blue
- [ ] Remote: lights blue while Remote is on, grey when off
- [ ] Connect / Disconnect and Clear RIT: same face, grey slot, still work
- [ ] PTT, TUN, AMP: red slot and red text while on, grey when off; face unchanged
- [ ] CMP, NB, NR, AN, MON: blue when on, grey when off
- [ ] AGC cycle, S/W, CW −/+/pitch, LOG, Save settings: grey slot, still work
- [ ] Hover makes the face a little lighter; pressing looks pushed in
- [ ] Shiny upper edge and raised edge look subtle, not cartoonish
- [ ] Window at minimum size (1024×600): left panel has no scroll bar
- [ ] Window at minimum size: Clear RIT button shows in full
- [ ] Default 1280×720: nothing clips on the band bar or either panel
- [ ] Tab pages (RX/TX, FAVORITES, cal tabs, SETTINGS), S/W, Remote Audio, Debug Log: buttons still gold and working
- [ ] Settings → UI Appearance → Button color still changes the gold buttons (it does not change the new ones; expected)
- [ ] Sliders, checkboxes and wheel (1 step per notch) still as after cmd-096

## Commit (commit only, no push, no installer, no GitHub release, no version bump)

Stew's rule: Build commits on this host; Stew pushes. Commit, then stop. Do not push.

```
git add Avalonia/src/MSCC.Avalonia/App.axaml Avalonia/src/MSCC.Avalonia/Views/MainWindow.axaml
git status
git commit -m "Avalonia: light-bar buttons on band bar and panels; filter buttons spaced like the right panel"
```

Never add: `.deb` files, `Avalonia/publish/`, `bin/`, `obj/`, `installers/`, Visual Studio files, or
other people's dirty files.
