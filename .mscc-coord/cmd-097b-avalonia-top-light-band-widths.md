# cmd-097b: Avalonia light-bar buttons lit from above; equal-width band buttons

Host: stew-HP (Ubuntu amd64, host id ubuntu-stew). Maintained client: `Avalonia/` (`MSCC.Avalonia.sln`,
package mscc-avalonia, 1.7.08). Follows cmd-097 (`d6ec0995`). Do NOT edit `mscc-ui/Avalonia-Migration`
(frozen), `rpi/`, `Solidus/`, `installers/rpi/`, or the WPF tree.

## STOP

- Commit only. Do not push. Stew pushes.
- No installer, no `.deb`, no GitHub Release.
- No version bump. Leave `MSCC.Avalonia.csproj` at 1.7.08. Do not edit
  `packaging/mscc-avalonia/DEBIAN/control`, the Addendum, or `installers/windows/CHANGELOG.md`.
- Do not touch `rpi/`, `Solidus/`, `installers/rpi/`, WPF, or `Controls/UiChromeTheme.cs`.
- Do not pull, merge, or rebase. When this brief was written stew-HP was 10 ahead and 1 behind:
  Ron's `c9906fa7` "Rules: release keyword finished -> cut release" (`AGENTS.md`, `CLAUDE.md`,
  `rpi/CLAUDE.md`; no `Avalonia/` change). Commit on top of the local tree. Stew pulls before he
  pushes.
- Do not change any binding, Command, CommandParameter, IsEnabled, ToolTip, or the
  `mscc-selected` / `mscc-tx-on` class bindings. Look and layout only.
- Never commit `bin/`, `obj/`, `Avalonia/publish/`, `.deb`, or Visual Studio files.

## Goal (Stew, 2026-11-27)

1. The light-bar buttons look like they lean to the right, most of all on the band row. The
   raised edge is lit from the upper left (light top/left, dark bottom/right), so each button's
   dark right edge sits next to the light left edge of the next button. Light every light-bar
   button **from directly above**: light top edge, dark bottom edge, left and right edges the same.
2. All band buttons the **same width**. Today 630 is the narrowest and 2200 is narrower than the
   rest. The row must fit at the minimum window (1024×600) with no clipping.

Decision on item 1: change it in `MsccLightBarButtonTheme`, so **all** light-bar buttons (band,
mode, filter/step, left and right panels) get it. One look everywhere, and one place for skins.

## Findings (tree at d6ec0995)

All paths are under `Avalonia/src/MSCC.Avalonia/`.

### Edge shading

- Resources: `App.axaml` 65 `UiBarEdgeLightBrush` (`#66707A86`), 66 `UiBarEdgeDarkBrush`
  (`#CC000000`).
- Template (`MsccLightBarButtonTheme`): `Border#RaisedLight` at 320–325
  (`BorderThickness="1,1,0,0"`, light) and `Border#RaisedDark` at 326–331
  (`BorderThickness="0,0,1,1"`, dark). That is the upper-left light.
- Pressed: 368–370 sets RaisedLight to dark, 371–373 sets RaisedDark to light.
- `UiBarEdgeLightBrush` / `UiBarEdgeDarkBrush` are used only in `App.axaml` (searched `*.axaml`
  and `*.cs`).

### Band button widths

`Views/MainWindow.axaml` 288–289: band Grid
`ColumnDefinitions="Auto,Auto,*,*,*,*,*,*,*,*,*,*,Auto"`, `ColumnSpacing="2"`, 13 buttons:

| Col | Button | Line | Width today |
|-----|--------|------|-------------|
| 0 | 2200 | 291 | `Auto` = text width (FontSize 10) + padding, about 35 px |
| 1 | 630 | 298 | `Auto`, about 29 px (narrowest) |
| 2–11 | 160, 80, 60, 40, 30, 20, 17, 15, 12, 10 | 306–351 | `*`, share what is left |
| 12 | GEN (`GenButtonText`) | 356 | `Auto` with `MinWidth="52"` (363) |

So 2200 and 630 size to their text, GEN is held at 52 px or more, and the 10 HF buttons share the
rest. GEN's text changes: Proficio WWV1–4, CHU1, CHU2, RWM, USER; Geminus 198, 660, 880
(`ViewModels/MainViewModel.cs` 137–155). The longest is 4 characters.

### Width math (minimum window 1024×600)

| Step | px |
|------|----|
| Window MinWidth (`MainWindow.axaml` 14) | 1024 |
| Outer Grid `Margin="4"` (23) | −8 → 1016 |
| Left and right panels `160,*,160` (23) | −320 → 696 |
| Center Grid `Margin="0,0,3,0"` (183) | −3 → 693 |
| Band Border `mscc-panel`: border 1+1, `Padding="3,3"` (287) | −8 → 685 |
| 12 gaps × `ColumnSpacing` 2 | −24 → 661 |
| **13 equal columns** | **661 ÷ 13 = 50.8 px each** |

Today at 1024: 2200 ≈ 35, 630 ≈ 29, GEN 52, so each HF button gets (661 − 116) ÷ 10 ≈ **54.5 px**.
Equal columns take about **4 px off each HF button** and give it to 2200, 630 and GEN. At 1280×720
the row is 940 − 24 = 916 px → **70.5 px each** (HF today ≈ 80).

Room in a 50.8 px button: 2 px outline + 8 px padding (`4,0`) → about 41 px for text. "2200" at
FontSize 10 Bold ≈ 26 px, "WWV4"/"CHU1" ≈ 30 px, "160" at FontSize 11 ≈ 21 px. Everything fits.

## Change plan

### 1. Light from above (`App.axaml`)

Resources (replace lines 65–66; keep the same light and dark values):

| Key | Use | Start value |
|-----|-----|-------------|
| `UiBarEdgeTopBrush` | 1 px top edge (light) | `#66707A86` (was EdgeLight) |
| `UiBarEdgeBottomBrush` | 1 px bottom edge (dark) | `#CC000000` (was EdgeDark) |
| `UiBarEdgeSideBrush` | 1 px left **and** right edges, the same on both | `#59000000` (a mid tone; tune by eye so neighbours look alike) |

Delete `UiBarEdgeLightBrush` and `UiBarEdgeDarkBrush`. After the change, a search for
`UiBarEdgeLight` / `UiBarEdgeDark` in `*.axaml` and `*.cs` must find nothing.

Template (replace `RaisedLight` and `RaisedDark` at 320–331 with three Borders, same place in the
Grid, before `Shine`; all `Grid.RowSpan="2"`, `CornerRadius="2"`, `IsHitTestVisible="False"`):

- `Border#EdgeSides`: `BorderThickness="1,0,1,0"`, BorderBrush `UiBarEdgeSideBrush`.
- `Border#EdgeTop`: `BorderThickness="0,1,0,0"`, BorderBrush `UiBarEdgeTopBrush`.
- `Border#EdgeBottom`: `BorderThickness="0,0,0,1"`, BorderBrush `UiBarEdgeBottomBrush`.

Pressed (replace 368–373): `^:pressed /template/ Border#EdgeTop` → `UiBarEdgeBottomBrush`;
`^:pressed /template/ Border#EdgeBottom` → `UiBarEdgeTopBrush`. Sides do not change on press.
Keep the pressed face (`UiBarFacePressedBrush`).

Shine (`Border#Shine`, top 45%) stays as it is. Nothing else in the theme changes: slot, text
margin, selected/blue, transmit/red, hover, disabled.

### 2. Equal band buttons (`Views/MainWindow.axaml`)

- 288: `ColumnDefinitions="Auto,Auto,*,*,*,*,*,*,*,*,*,*,Auto"` →
  **`ColumnDefinitions="*,*,*,*,*,*,*,*,*,*,*,*,*"`** (13 equal columns). Keep `ColumnSpacing="2"`
  and every `Grid.Column` index.
- GEN (356–363): remove `MinWidth="52"`; add `HorizontalAlignment="Stretch"`. Keep its other
  attributes.
- 2200 and 630 already have `HorizontalAlignment="Stretch"`; leave them.
- Do not set a fixed Width on any band button. Equal `*` columns keep them equal at every window
  size and fill the row (a fixed width would leave an empty gap on the right at 1280 and wider).
- Leave font sizes as they are (2200/630/GEN 10, HF 11).

## Acceptance

- Every light-bar button looks lit from above: light top, dark bottom, both sides the same. The
  band row no longer looks like it leans. Pressed swaps only top and bottom.
- The 13 band buttons are the same width at 1024×600 (about 51 px) and at 1280×720 (about 70 px).
  No text clips, including GEN on WWV4 / CHU1 / USER.
- The band row fits at 1024×600 with no clipping. Bands, GEN rotation and selection still work.
- No `UiBarEdgeLight` / `UiBarEdgeDark` left anywhere.

## Build and smoke (stew-HP, amd64)

Do not build a `.deb`. Run from source:

```
cd /home/stew/Documents/GitHub/mscc-station/Avalonia
dotnet build src/MSCC.Avalonia/MSCC.Avalonia.csproj -c Release
dotnet run --project src/MSCC.Avalonia/MSCC.Avalonia.csproj -c Release --no-build
```

Checklist:

- [ ] Release build runs on stew-HP with no new warnings in the XAML
- [ ] Band row: no "leaning right" look; each button lit from the top, sides alike
- [ ] Band row: all 13 buttons the same width at 1280×720
- [ ] Band row: all 13 buttons the same width at 1024×600, nothing clipped, no scroll
- [ ] 2200 and 630 text fits; HF numbers fit
- [ ] GEN: press it to rotate WWV1–4, CHU1, CHU2, RWM, USER; text always fits
- [ ] Band selection: blue bar on the current band; each band still changes band
- [ ] Mode, filter/step, left and right panel buttons: lit from the top, sides alike
- [ ] Pressing any button: top and bottom edges swap (looks pushed in); sides unchanged
- [ ] Shine at the top still shows; slot, blue selected and red PTT/TUN/AMP unchanged
- [ ] 1024×600: left panel still has no scroll bar and Clear RIT shows in full

## Commit (commit only, no push, no installer, no GitHub release, no version bump)

Stew's rule: Build commits on this host; Stew pushes. Commit, then stop. Do not push.

```
git add Avalonia/src/MSCC.Avalonia/App.axaml Avalonia/src/MSCC.Avalonia/Views/MainWindow.axaml
git status
git commit -m "Avalonia: light-bar buttons lit from above; equal-width band buttons"
```

Never add: `.deb` files, `Avalonia/publish/`, `bin/`, `obj/`, `installers/`, Visual Studio files, or
other people's dirty files.
