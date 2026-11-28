# cmd-102 — Avalonia RX/TX tab: compact lists, physical sliders, Tune button, fit at 1024×600

> **STOP — read first.** Build commits only. **No push, no installer, no GitHub Release, no version bump** (Avalonia stays **1.7.08**). Avalonia only: `Avalonia/src/MSCC.Avalonia`. **Read the WPF tree for reference only — do not edit it.** **Do not touch** `rpi/`, `Solidus/`, `installers/rpi/`, **`Controls/UiChromeTheme.cs`**, or `mscc-ui/Avalonia-Migration`. No new radio protocol commands; bindings stay on existing properties and commands. Do not commit bin/obj/publish/.deb/VS files or `.mscc-coord/status/ubuntu-stew.md`. Do not pull, merge or rebase. Stew pushes. Ask Stew before any destructive git step.

Follows cmd-101c (Build `7587b919`, on origin). Stew: CW tab is done (101c perfect), the other tabs look good, and he was told to push. This command is the RX/TX tab. Stew: layout and text already look good; update the selection lists like the CW tab, update the sliders, and add the Tune button. It must fit at 1024×600 with **no scroll bar**.

Stew: the extra WPF help text is not needed. Do not add it. Remove the Avalonia help lines listed in §5.

Line numbers are from `7587b919` (`Views/MainWindow.axaml` unless noted). RX/TX tab: 700–860.

## WPF vs Avalonia (reference only)

WPF: `mscc-ui/windows-work-tree/mscc-mscc/mscc-new/src/MSCC.Wpf/MainWindow.xaml` 1015–1210, and Stew's screenshot (~680×375).

| | WPF | Avalonia today |
|---|---|---|
| Two columns | Power + default filters \| TX options + buttons | same |
| POWER LEVELS | 5 rows: label 90 \| thin slider 140 \| boxed `100%` green | 5 rows: label 88 \| Fluent `mscc-slider` (round orange thumb + orange fill) \| unboxed `mscc-readout` |
| Power help | none | "Used by TUN / mode (wired when connected)." |
| DEFAULT FILTERS | LOW (5), HIGH (5), CW (3), TX (4); heights 115/115/80/115; items ~20 px, all visible, no list scroll | same bindings; heights 118/118/80/100; Fluent items ~35 px, lists scroll, last items cut off, selected = orange fill |
| Filters help | none | "Radio defaults (saved on server). Active Lo/Hi/CW stay on the left rail." |
| Right headings | **TX OPTIONS** over TX BW, **PROCESSING** over AGC, one row | one heading `TX OPTIONS / PROCESSING` |
| TX BW | 4 items, height 115, no scroll | height 100, items cut off |
| AGC | list SLOW/MED/FAST, `ItemsSource=AgcOptions`, `SelectedIndex=AgcLevel` (sends the existing AGC command) | dark box showing `AgcButtonText` + "Use right rail / AGC button" |
| Help to the right of AGC | three WPF lines about NB/NR/AN, AGC Fast Release, and the right-panel AGC button | one line: "NB, NR, AN and AGC Fast Release stay on the right operate rail." |
| Buttons | 2×2: QRP, AMP, **Tune**, 2 TONE. Tune = `ToggleTuneCommand`, red when `TuneMode` | 2×2: QRP, AMP, 2 TONE, **(reserved)** — all `mscc-gold` |
| Footer lines | none | "TUN uses Tune Power only…", `Current mode: {ModeText}`, `RF mirror: {RfPower}%` |
| Fit at 1024×600 | fits | ~44 px too tall (vertical scroll bar, noted in cmd-101) |

## 1. Compact lists (same style as CW, cmd-101)

In `App.axaml`, extend the existing compact-list style so RX/TX can use it without renaming the CW class:

```xml
<Style Selector="ListBox.mscc-cw-list, ListBox.mscc-compact-list">
```
and the same for `ListBoxItem`. No visual change to the CW lists. Do not add a new selected-item theme; selected look stays what the CW lists already show (Stew signed those off).

Apply `Classes="mscc-compact-list"` to all six RX/TX ListBoxes (LOW, HIGH, CW, TX default, TX BW, and the new AGC list). Heights, with ~17 px items:

| List | Items | Height |
|---|---|---|
| LOW, HIGH, TX (defaults) | 5, 5, 4 | **100** (aligned; 5×17=85 plus padding, all rows visible, no scroll) |
| CW | 3 | **60** |
| TX BW, AGC | 4, 3 | **100** (tops and bottoms aligned) |

Drop the fixed `Width="72"` / `Width="80"` if the caption and items fit tighter (`Margin="0,0,8,0"` like WPF, Auto width). Bindings, `IsEnabled`, and tooltips stay as they are.

## 2. AGC list (existing command, no new protocol)

Avalonia already has `AgcLevel` (0=SLOW, 1=MED, 2=FAST). `OnAgcLevelChanged` (~2808) updates `AgcButtonText` (SLO/MED/FST on the right-rail button) and calls `SendAgcLevelAsync` → `SetAgcLevelAsync`. That is the same path as the right-rail AGC button (`CycleAgcCommand`).

Add one collection next to the other option lists in `ViewModels/MainViewModel.cs` (~949):

```csharp
public ObservableCollection<string> AgcOptions { get; } = new()
    { "SLOW", "MED", "FAST" };
```

Replace the AGC info box (~820–828) with a ListBox: `ItemsSource="{Binding AgcOptions}"`, `SelectedIndex="{Binding AgcLevel}"`, `Classes="mscc-compact-list"`, same `IsEnabled="{Binding IsConnected}"` as TX BW. Selecting an item sets `AgcLevel` and the existing handler sends it; the right-rail button text stays in sync. No new opcode.

## 3. Power sliders = left-rail physical style + boxed readout

The five power rows (~710–756) today use Fluent `Classes="mscc-slider"` (round orange thumb). Change each slider to `Classes="mscc-physical"` (cmd-096 `MsccPhysicalSliderTheme`: 9 px track, no white highlight, `SliderWheel.OneStepPerNotch` already on). **Remove** `PointerWheelChanged="Slider_OnPointerWheelChanged"` from these five sliders so the wheel does not step twice.

Value display: not the rail's unboxed `mscc-slider-value`, and not `mscc-readout`. Match the WPF boxed readout (and the CW SPEED/HOLD boxes):

```xml
<Border Grid.Column="2" Background="#FF444444" BorderBrush="#FF888888" BorderThickness="1"
        Padding="3,1" MinWidth="40" Margin="5,0,0,0" VerticalAlignment="Center">
  <TextBlock Text="{Binding TunePowerPercent, StringFormat={}{0}%}" FontSize="11" FontWeight="Bold"
             Foreground="{DynamicResource UiAccentBrush}" HorizontalAlignment="Center"/>
</Border>
```

Row: `ColumnDefinitions="90,*,Auto"`, Height 24, `Margin="0,0,0,4"` (last row `0,0,0,10`). Label width 90, FontSize 11, existing `mscc-label`. Bindings, min/max, `IsEnabled`, and the FM tooltip stay the same.

Do not put `mscc-physical` on Settings, S/W, or cal sliders. Right-rail sliders are already physical; leave them alone.

## 4. TX buttons: add Tune, all `mscc-bar`

Replace the UniformGrid (~833–847) with a 2×2 like WPF: **QRP | AMP** / **Tune | 2 TONE**. Delete the `(reserved)` stub.

| Button | Command | Lit | Enabled |
|---|---|---|---|
| QRP | existing `SelectQrpCommand` | `Classes.mscc-selected="{Binding QrpMode}"` | as now |
| AMP | existing `SelectFullPowerCommand` | `Classes.mscc-selected="{Binding FullPower}"` | as now |
| Tune | existing `ToggleTuneCommand` (same as the right-rail **TUN**) | `Classes.mscc-tx-on="{Binding TuneMode}"` (red when on, same as TUN) | `CanUserControlTransmit` |
| 2 TONE | existing `ToggleTwoToneCommand` | `Classes.mscc-selected="{Binding TwoToneArmed}"` | `CanUserControlTransmit` (WPF does this; Avalonia had no IsEnabled) |

All four: `Classes="mscc-bar"`, Width 100, Height 28, QRP/Tune `Margin="0,0,6,6"` / `"0,0,6,0"`, AMP/2 TONE `Margin="0,0,0,6"` / `"0,0,0,0"`. Keep the existing QRP, AMP, and 2 TONE tooltips. Tune tooltip = the right-rail TUN tip: `"Antenna tune: TUNE mode + carrier (Tune Power). Red when on."`

Drop the "TX BUTTONS" heading — WPF has none; the 2×2 sits under the lists.

## 5. Remove help and footer text (list)

Stew: the extra WPF help is not necessary, and the Avalonia help lines on this tab can go. **Delete** all of the following (none is a live control Stew needs; mode is on the VFO, RF power is on the right-rail POWER slider, AGC is the new list plus the right-rail button):

| What | Line (today) |
|---|---|
| "Used by TUN / mode (wired when connected)." | 708 |
| "Radio defaults (saved on server). Active Lo/Hi/CW stay on the left rail." | 759–760 |
| "NB, NR, AN and AGC Fast Release stay on the right operate rail." | 806–807 |
| The AGC box text "Use right rail" / "AGC button" (box itself is replaced in §2) | 824–826 |
| "TUN uses Tune Power only. USB, LSB and DIG-U use SSB Power." | 849–850 |
| `Current mode: {ModeText}` | 851–852 |
| `RF mirror: {RfPower}%` | 853–854 |

**Do not add** the three WPF lines about NB/NR/AN, AGC Fast Release, or the right-panel AGC button.

## 6. Right-column headings and chrome

- Split `TX OPTIONS / PROCESSING` into one row, WPF-style: `TX OPTIONS` over the TX BW column (width of that column), `PROCESSING` over the AGC column. Both `mscc-face-heading`.
- `POWER LEVELS` and `DEFAULT FILTERS` stay. `POWER LEVELS` `Margin="0,0,0,6"`; `DEFAULT FILTERS` `Margin="0,4,0,6"`.
- The two inner column Borders (~705, 803) already have no fill; set `Padding="0"` (the tab well already pads 8) so they do not eat width and height.

## Fit at 1024×600

Available tab content is about **370 px** (cmd-101). New left column ≈ heading 20 + 5×28 slider rows + 10 + heading 20 + caption 14 + list 100 ≈ **304 px**. Right column is shorter (headings + list 100 + 2×2 buttons ≈ 190). No vertical scroll bar, no horizontal scroll bar, at 1024×600 or 1280×720. Report measured content height vs viewport at 1024×600.

## Checklist (Build)

- [ ] `git fetch` + `git status -sb`; report if origin moved. Do not pull.
- [ ] §1 compact-list style shared with CW; six lists, heights per table, all items visible, no list scroll.
- [ ] §2 `AgcOptions` added; AGC ListBox bound to `AgcLevel`; no new opcode.
- [ ] §3 five power sliders `mscc-physical`, boxed `%` readout; wheel handler removed on those five only.
- [ ] §4 2×2 QRP / AMP / Tune / 2 TONE, `mscc-bar`; Tune = `ToggleTuneCommand` + `mscc-tx-on`; reserved gone.
- [ ] §5 every listed help/footer line removed; no WPF help added.
- [ ] §6 headings split; inner padding 0.
- [ ] No scroll bar on RX/TX at **1024×600** or 1280×720. Report content height vs viewport.
- [ ] `dotnet build -c Release --no-incremental`: 0 errors, no new warnings.
- [ ] Screenshots at both sizes (RX/TX, and a glance at CW lists to confirm they are unchanged).
- [ ] One commit: `Avalonia: RX/TX tab compact lists, physical sliders, Tune button; fits 1024`.
- [ ] Report commit hash, files changed, measurements.

## Stew's check (printable)

- [ ] At 1024×600 the RX/TX tab has no scroll bar; the etched well is fully visible.
- [ ] POWER LEVELS: five physical sliders (same style as the left rail), boxed green `100%` (or the current value) at the right.
- [ ] LOW, HIGH, CW, TX, TX BW and AGC lists show every item; no list scrolls; selected item looks like the CW tab lists.
- [ ] Clicking a default filter or TX BW still changes the radio. Clicking SLOW/MED/FAST updates the right-rail AGC button and the radio.
- [ ] Buttons are light-bar: QRP, AMP, Tune, 2 TONE. Tune matches the right-rail TUN (red when on). QRP/AMP/2 TONE still latch as before.
- [ ] No help lines and no "Current mode" / "RF mirror" text on the tab.
- [ ] CW tab lists still look the same.
