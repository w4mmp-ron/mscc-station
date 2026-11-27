# cmd-101 — Avalonia tabs: etched border around the tab working area; CW tab to the WPF layout (compact), CQ memory fits, light-bar buttons

> **STOP — read first.** Build commits only. **No push, no installer, no GitHub Release, no version bump** (Avalonia stays **1.7.08**). Avalonia only: `Avalonia/src/MSCC.Avalonia`. **Read the WPF tree for reference only — do not edit it.** **Do not touch** `rpi/`, `Solidus/`, `installers/rpi/`, **`Controls/UiChromeTheme.cs`**, or `mscc-ui/Avalonia-Migration`. No radio protocol commands; layout and styling only, every binding and command stays the same. Do not commit bin/obj/publish/.deb/VS files or `.mscc-coord/status/ubuntu-stew.md`. Do not pull, merge or rebase. Stew pushes. Ask Stew before any destructive git step.

Follows cmd-100 (Build `3f84285c`). Stew approved cmd-100: MAIN tab is done. Plan from here: refine Avalonia **tab by tab** before the Windows port. Skins come later, each marked **Low CPU** or **Flashy** (not in this command).

Line numbers are from `3f84285c` (`Views/MainWindow.axaml` unless noted); find anchors by name if they moved.

## 1. Etched border around the tab working area (all tabs except MAIN)

**MAIN is excluded.** Its spectrum already sits in the recessed bezel frame (cmd-100), which defines the area; a second line around it would double the edge.

Every other tab already has one root `Border` (transparent since cmd-100). Turn each into a "tab well":

| Tab | Root Border (line) | Today |
|---|---|---|
| CW | 491 | `Background="Transparent" BorderThickness="0" Padding="10"` |
| RX/TX | 692 | `… Padding="8"` |
| FAVORITES | 854 | `… CornerRadius="4" Padding="10"` |
| QRP CAL | 927 | `… CornerRadius="4" Padding="10"` |
| AMP CAL | 1039 | `… CornerRadius="4" Padding="10"` |
| RX IQ | 1151 | `… CornerRadius="4" Padding="8"` |
| TX IQ | 1257 | `… CornerRadius="4" Padding="10"` |
| FREQ CAL | 1361 | `… Padding="8"` |
| SETTINGS | 1473 | `Classes="mscc-panel" Margin="8"` |

For each: **remove** the local `Background`, `BorderThickness`, `CornerRadius` and `Margin` (local values beat styles), remove `mscc-panel` from SETTINGS, add `Classes="mscc-tab-well"`, keep the tab's own `Padding` (8 or 10).

`App.axaml`, in the `UiFace*` group:
```xml
<!-- Tab well: etched groove, light from above (dark line, light line under it). -->
<SolidColorBrush x:Key="UiFaceEtchDarkBrush" Color="#B0141A22"/>
<BoxShadows x:Key="UiFaceEtchShadows">inset 0 1 0 0 #26FFFFFF, 0 1 0 0 #40FFFFFF</BoxShadows>
```
Style (after `Border.mscc-panel` so it wins on order too):
```xml
<Style Selector="Border.mscc-tab-well">
  <Setter Property="Background" Value="Transparent"/>
  <Setter Property="BorderBrush" Value="{DynamicResource UiFaceEtchDarkBrush}"/>
  <Setter Property="BorderThickness" Value="1"/>
  <Setter Property="CornerRadius" Value="4"/>
  <Setter Property="Margin" Value="2"/>
  <Setter Property="BoxShadow" Value="{DynamicResource UiFaceEtchShadows}"/>
</Style>
```
- Result: a 1 px dark line all round; a 1 px light line just inside the top edge and just below the bottom edge. Zero blur: cheap on the Pi.
- Inside stays the faceplate color (no fill). If `BoxShadows` does not parse as a resource, put the same string in the setter `Value` and keep the comment. Report which.

## 2. CW tab: match the WPF layout, compact

### Reference (WPF, read-only)
`mscc-ui/windows-work-tree/mscc-mscc/mscc-new/src/MSCC.Wpf/MainWindow.xaml` 741–1012, and Stew's screenshot of it (about **620 px** wide, CW CONTROLS block about **170 px** tall):

```
CW CONTROLS
[ ] Use external electronic keyer (legacy)
 KEYER MODE   SPACING   PADDLE    WEIGHT  PITCH      SPEED       [20 ] - + WPM
 ┌─────────┐ ┌───────┐ ┌───────┐ ┌────┐ ┌──────┐     FARNSWORTH  [Off] - + WPM  [ ] PADDLE
 │STRAIGHT │ │ELEMENT│ │NORMAL │ │ 25 │ │400Hz │     HOLD        [100] - + ms
 │IAMBIC-A │ │LETTER │ │REVERSE│ │ 50 │ │600Hz │     [ ] POTENTIA / QSK
 │IAMBIC-B │ └───────┘ └───────┘ │ 75 │ │800Hz │
 └─────────┘                     └────┘ │1000Hz│
                                        └──────┘
CQ MEMORY
<two-line gray help text>
[ slot 0 text box ]  0/48  [R] [P]
[ slot 1 text box ]  0/48  [R] [P]
[ slot 2 text box ]  0/48  [R] [P]
[ slot 3 text box ]  0/48  [R] [P]
```

### WPF vs Avalonia today
| | WPF | Avalonia (3f84285c) |
|---|---|---|
| Scroll | vertical only (`HorizontalScrollBarVisibility="Disabled"`) | horizontal **Auto** (492) → content runs wider than the tab and pushes R/P off the right edge |
| Spacing | StackPanel, no spacing; margins per row | `StackPanel Spacing="10"` (493) |
| Help line under heading | none | "Keyer / spacing / paddle …" (495) |
| Lists row | 5 lists, `Margin="5,0"`, heights 75/60/60/75/100, item padding `2,1` | 5 lists, fixed widths 100/88/88/72/80, heights 78/56/56/78/100, **Fluent item padding (~12,9)** so items are ~36 px and the lists scroll |
| SPEED / FARNSWORTH / HOLD | **stacked column to the right of the lists** (`Grid` `Auto,Auto`, right column `Margin="8,0,0,0"`), label width 78, value box MinWidth 35, − + 18×18, rows `Margin 0,0,0,8` | **one horizontal row under the lists** (551–599, Spacing 16) ≈ 730 px wide |
| PADDLE checkbox | end of the FARNSWORTH row | same |
| POTENTIA / QSK | under HOLD in the right column | end of the horizontal row |
| CQ MEMORY | heading `Margin 0,14,0,4`, help text, `Grid *,Auto,Auto,Auto`, R/P 32×26 | same grid, but stretched by the over-wide content |
| RIT note | none | "RIT is on the left rail …" (684) |

### Changes (CW tab, 490–689)
1. ScrollViewer (492): `HorizontalScrollBarVisibility="Disabled"`, vertical `Auto`.
2. Root StackPanel (493): `Spacing="0"`; give rows their own margins as below.
3. Heading `CW CONTROLS` (494): `Margin="0,0,0,6"`. **Delete** the help line (495–496).
4. Legacy checkbox (498): `Margin="0,0,0,4"`, `MinWidth="0"`. Warning text (502) unchanged, `Margin="0,0,0,6"`.
5. **Keyer block** = `Grid ColumnDefinitions="Auto,Auto" HorizontalAlignment="Left" Margin="0,0,0,10"`:
   - **Column 0** — the 5 lists in a horizontal StackPanel, each list's StackPanel `Margin="5,0"` (drop the fixed Widths 100/88/88/72/80), caption TextBlock above as now, `IsEnabled` bindings unchanged (PITCH has none, as now).
     - Heights as WPF: KEYER MODE **75**, SPACING **60**, PADDLE **60**, WEIGHT **75**, PITCH **100**.
     - Add `Classes="mscc-cw-list"` to each ListBox and in `App.axaml`:
       `ListBox.mscc-cw-list` → `ScrollViewer.VerticalScrollBarVisibility="Disabled"`, `Padding="0"`;
       `ListBox.mscc-cw-list ListBoxItem` → `Padding="4,1"`, `MinHeight="0"`, `FontSize="11"`.
       Items become ~17 px: 3 rows = 51 ≤ 75, 4 rows (PITCH) = 68 ≤ 100. No list scrolls.
   - **Column 1** — `StackPanel Margin="8,0,0,0" VerticalAlignment="Top"`:
     - SPEED row, FARNSWORTH row (+ PADDLE checkbox, `Margin="6,0,0,0"`, `MinWidth="0"`), HOLD row: each `Orientation="Horizontal" Margin="0,0,0,8"` (HOLD `0,0,0,10`), same `IsEnabled` bindings as now (HOLD has none).
     - In each row: label TextBlock `Width="78"` FontSize 10 white; value Border `#FF444444`/`#FF888888` `Padding="3,1" MinWidth="35"` with the value TextBlock FontSize 11 Bold accent (a readout: stays dark); − and + buttons (see §4); unit TextBlock (`WPM` / `ms`) `Margin="4,0"`.
     - `POTENTIA / QSK` checkbox under HOLD, `MinWidth="0"`.
   - Width: column 0 ≈ 76 + 64 + 64 + 40 + 60 + 5 × 10 margins ≈ **354**; column 1 ≈ 8 + 78 + 35 + 2 + 24 + 2 + 24 + 30 (unit) + 6 + ~66 (PADDLE) ≈ **275** → ≈ **630 px**. Available at 1024×600: center column 693 − TabControl padding 4 − well margin 4 − well border 2 − CW padding 20 ≈ **663 px**. Fits.
   - Fluent `CheckBox` has a default `MinWidth` of 120; set `MinWidth="0"` on all CW-tab checkboxes so they size to their text.
6. **CQ MEMORY** (601–682):
   - Heading `Margin="0,4,0,4"`; help text unchanged, `Margin="0,0,0,6"`.
   - Grid: `ColumnDefinitions="Auto,Auto,Auto,Auto"`, `HorizontalAlignment="Left"`.
   - Slot TextBoxes: `Width="360"`, `Padding="4,3"`, `MaxLength="48"` unchanged, FontSize 12 monospace unchanged. Width math: 48 chars × 0.602 em × 12 pt = **347 px** in DejaVu Sans Mono (stew-HP; Consolas is not installed) + padding 8 + border 2 = 357 → **360**. Consolas on Windows is narrower (0.55 em → 317 px).
   - Row: 360 + 6 + counter 36 + 6 + R 30 + 4 + P 30 = **472 px** ≤ 663. R and P fully visible at 1024×600.
   - `KeyerMemStatus` line unchanged.
7. **Delete** the RIT note (684–685) — not in WPF, and RIT has its own section on the left panel.
8. Height at 1024×600: tab content area ≈ 600 − 8 − 126 (top row) − 2 − ~34 (band row) − ~30 (tab strip) − 4 ≈ **396 px**; well border/margin/padding −26 → ≈ **370 px** for content. CW content ≈ heading 20 + checkbox 26 + keyer block 113 + 10 + CQ heading 22 + help 32 + 4 × 28 slots + status 16 ≈ **351 px**. No vertical scroll bar expected; report the measured value.

## 3. (covered in 2.6) CQ memory slots sized to 48 characters

## 4. CW tab buttons = front-panel light-bar buttons

Convert all 14 `mscc-gold` buttons in the CW tab to `Classes="mscc-bar"` (cmd-097 `MsccLightBarButtonTheme`). Commands, `CommandParameter`s, `IsEnabled` bindings and tooltips stay exactly as they are.

| Buttons | Count | Size | FontSize |
|---|---|---|---|
| − / + for SPEED, FARNSWORTH, HOLD | 6 | Width 24, Height 22, `Margin="2,0"` / `"0,0,2,0"` | 11 Bold |
| R / P for slots 0–3 | 8 | Width 30, Height 24, `Margin="0,2,4,2"` (R) / `"0,2,0,2"` (P) | 11 Bold |

- Use the real minus sign "−" (as now) on the − buttons.
- Check the light-bar slot does not cover the glyph at 22 px height; if it does, use Height 24 for the − / + buttons too and report.
- Other tabs keep `mscc-gold` for now (tab by tab).

## Fit
- 1024×600 and 1280×720: CW tab with no horizontal scroll bar; R/P visible; keyer block on one row (lists + right column) as in the WPF screenshot.
- Every tab: the well border is fully visible on all four sides (no clipping at the bottom at 1024×600). Report any tab whose content now needs a vertical scroll bar that did not before (the well takes 2 px margin + 1 px border each side).

## Checklist (Build)
- [ ] `git fetch` + `git status -sb`; report if origin moved. Do not pull.
- [ ] §1 `UiFaceEtchDarkBrush`, `UiFaceEtchShadows`, `Border.mscc-tab-well`; 9 tab roots converted (local values removed); MAIN untouched. Report resource vs literal BoxShadows.
- [ ] §2 CW tab rebuilt per 2.1–2.8 (no horizontal scroll, help line and RIT note removed, compact lists, right column SPEED/FARNSWORTH/HOLD/QSK, CQ grid Auto columns, slot Width 360).
- [ ] §4 14 CW buttons → `mscc-bar`; bindings unchanged.
- [ ] `dotnet build -c Release --no-incremental`: 0 errors, no new warnings.
- [ ] Screenshots at **1024×600** and **1280×720**: CW, RX/TX, FAVORITES, SETTINGS, QRP CAL. Report CW keyer-block width × height, CQ row width, CW content height vs available, any new vertical scroll bars.
- [ ] One commit: `Avalonia: etched tab wells; CW tab to WPF layout (compact), CQ slots fit, light-bar buttons`.
- [ ] Report commit hash, files changed, measurements.

## Stew's check (printable)
- [ ] Every tab except MAIN has a thin etched border around its working area; MAIN unchanged.
- [ ] CW tab looks like the WPF screenshot: lists in one row, SPEED / FARNSWORTH / HOLD stacked to the right, PADDLE at the end of FARNSWORTH, POTENTIA / QSK under HOLD.
- [ ] All list items visible without scrolling; selecting KEYER MODE / SPACING / PADDLE / WEIGHT / PITCH still changes the radio.
- [ ] SPEED, FARNSWORTH, HOLD − / + work; buttons look like the front-panel light-bar buttons.
- [ ] CQ MEMORY at 1024×600: all four R and P buttons visible; 48 characters fit in a slot; R stores, P plays, paddle aborts.
- [ ] Legacy keyer checkbox still grays the keyer controls (HOLD stays live).
- [ ] No horizontal scroll bar on the CW tab at either size.
