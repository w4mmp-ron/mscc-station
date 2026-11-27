# cmd-094: Avalonia mode-button order and button text centering (WPF parity)

Host: stew-HP (Ubuntu amd64, host id ubuntu-stew). Maintained client: `Avalonia/` (`MSCC.Avalonia.sln`,
package mscc-avalonia, now 1.7.08). Do NOT edit `mscc-ui/Avalonia-Migration` (frozen), `rpi/`,
`Solidus/`, `installers/rpi/`, or the WPF tree. WPF is the reference; read it, do not change it.

## STOP

- Commit only. Do not push. Stew pushes.
- No installer, no `.deb` for Stew to ship, no GitHub Release.
- No version bump. CLAUDE.md does not require an Avalonia version bump until "finished" (Addendum
  at release). Leave `MSCC.Avalonia.csproj` at 1.7.08, and do not edit `packaging/mscc-avalonia/DEBIAN/control`,
  the Addendum, or `installers/windows/CHANGELOG.md`. The title bar will still say 1.7.08.
- Do not touch `rpi/`, `Solidus/`, `installers/rpi/`, or WPF.
- Pull first: `git pull --rebase --autostash`.

## Findings (read-only, tree at 52b8a1f2)

### 1. Mode button order

WPF (`mscc-ui/windows-work-tree/mscc-mscc/mscc-new/src/MSCC.Wpf/MainWindow.xaml`) uses a 3×2 Grid.
Comment at 2207: "MODE: 3×2, band-button size". Grid columns/rows at 2212–2219. Buttons:

| Grid | Line | Label |
|------|------|-------|
| Row 0, Col 0 | 2222 | LSB |
| Row 0, Col 1 | 2238 | USB |
| Row 0, Col 2 | 2253 | DIG-U |
| Row 1, Col 0 | 2269 | CW |
| Row 1, Col 1 | 2284 | AM |
| Row 1, Col 2 | 2299 | FM |

Avalonia (`Avalonia/src/MSCC.Avalonia/Views/MainWindow.axaml`) uses a `UniformGrid Columns="3" Rows="2"`
at 1641 (fills left to right, top to bottom). Current button order:

| Line | Label | Lands on |
|------|-------|----------|
| 1642 | USB | Row 1, Col 1 |
| 1645 | LSB | Row 1, Col 2 |
| 1648 | CW | Row 1, Col 3 |
| 1651 | AM | Row 2, Col 1 |
| 1654 | DIG-U | Row 2, Col 2 |
| 1658 | FM | Row 2, Col 3 |

So Avalonia now shows USB / LSB / CW on the first row and AM / DIG-U / FM on the second. The only
mode buttons in Avalonia are this UniformGrid. `SetModeCommand` stays as it is; only the XAML order
changes.

### 2. Why band buttons are centered and the rest are not

WPF gold buttons are centered because `MsccGoldButtonStyle` (`MainWindow.xaml` 35–82) replaces the
template. The ContentPresenter is hard-coded to Center (`MainWindow.xaml` 57–58):

```xml
<ContentPresenter HorizontalAlignment="Center"
                  VerticalAlignment="Center"
                  RecognizesAccessKey="True"/>
```

Avalonia gold buttons use Fluent's Button template, with `Button.mscc-gold` in
`Avalonia/src/MSCC.Avalonia/App.axaml` 34–44. That style sets Background, Foreground, Border,
FontWeight, FontSize, Padding (4,1), MinHeight (22) and CornerRadius. It does **not** set
`HorizontalContentAlignment` or `VerticalContentAlignment`. The matching
`Button.mscc-gold /template/ ContentPresenter#PART_ContentPresenter` style (45–50) only paints
Background / Border / CornerRadius (needed so Fluent's grey hover does not win). It also does not
set content alignment.

The 13 band-bar buttons in `MainWindow.axaml` 294–363 (2200, 630, 160, 80, 60, 40, 30, 20, 17, 15,
12, 10, GEN) each set this locally, which is why they look right. First two:

- 2200: `MainWindow.axaml` 299
- 630: `MainWindow.axaml` 306

Every later band button in that bar repeats the same two attributes.

No other button does. Count on this tree: 120 `Button.mscc-gold`, 1 `Button.mscc-stub`
(`MainWindow.axaml` 741, "(reserved)"). 13 gold buttons have the explicit Center setters; 107 gold
buttons plus the stub do not. Every button uses `Content="..."` or `Content="{Binding ...}"`. None
wrap a TextBlock or other child, so there is no child alignment to fix.

Fluent Button defaults are Center, but that is not what Stew sees on the side panels or in the
tabs. The reliable fix is the same one the band bar already uses, applied once in the shared style,
and also on the ContentPresenter that this theme paints over.

Affected files if you edited buttons one by one (do not do that):

- `Avalonia/src/MSCC.Avalonia/Views/MainWindow.axaml` — left rail (Connect, Phones, Digital, Remote,
  Lo/Hi/CW/Step, Clear RIT), right rail (PTT, TUN, AGC, AMP, CMP, S/W, mode, NB/NR/AN/MON,
  LOW/MED/HIGH, CW −/+/pitch, LOG, Save settings), and buttons in tabs MAIN, CW (R/P memory),
  RX/TX (QRP, AMP, 2 TONE, reserved stub), FAVORITES, QRP CAL, AMP CAL, RX IQ, TX IQ, FREQ CAL,
  SETTINGS
- `Avalonia/src/MSCC.Avalonia/Views/DebugLogWindow.axaml` — RESET LOGS, CLOSE
- `Avalonia/src/MSCC.Avalonia/Views/RemoteAfWindow.axaml` — Reset EQ
- `Avalonia/src/MSCC.Avalonia/Views/SpectrumWaterfallWindow.axaml` — RST, time marker, Close

## Change plan

1. In `Avalonia/src/MSCC.Avalonia/Views/MainWindow.axaml`, in the MODE `UniformGrid` at 1641, reorder
   the six Button elements to: LSB, USB, DIG-U, then CW, AM, FM. Keep every attribute, command,
   tooltip, binding, and the `FontSize="9"` on DIG-U. Do not change `MainViewModel` or the command.

2. In `Avalonia/src/MSCC.Avalonia/App.axaml`, add these two setters to `Button.mscc-gold` (style
   starts at 34):

```xml
<Setter Property="HorizontalContentAlignment" Value="Center"/>
<Setter Property="VerticalContentAlignment" Value="Center"/>
```

3. Add the same two setters to `Button.mscc-gold /template/ ContentPresenter#PART_ContentPresenter`
   (style starts at 45), so the presenter that paints the gold face also centers the label.

4. Add the same two setters to `Button.mscc-stub` (style starts at 123).

5. Leave the explicit Center attributes on the 13 band-bar buttons. They already look right.

6. Do not walk the 107 other buttons to add local attributes. The style covers them.

7. Do not change Padding, MinHeight, or FontSize unless smoke shows a clipped label. DIG-U is
   FontSize 9 on purpose. If one label clips after centering, shrink FontSize or Padding on that
   button only and note it in the commit message.

## Acceptance

- Mode row 1 is LSB, USB, DIG-U. Mode row 2 is CW, AM, FM. Each button still selects that mode
  and latches the highlight.
- Button text is centered horizontally and vertically on the left rail, the right rail, the band
  bar, and the buttons in every tab and in the Debug Log, Remote Audio, and Spectrum/Waterfall
  windows.
- Selected (mscc-selected) and TX-on (red PTT / TUN / AMP) still paint correctly.
- No clipped labels.

## Build and smoke (stew-HP, amd64)

Do not build a `.deb` and do not install over 1.7.08. Run from source:

```
cd /home/stew/Documents/GitHub/mscc-station/Avalonia
dotnet build src/MSCC.Avalonia/MSCC.Avalonia.csproj -c Release
dotnet run --project src/MSCC.Avalonia/MSCC.Avalonia.csproj -c Release --no-build
```

Checklist:

- [ ] Release build runs on stew-HP (Ubuntu amd64)
- [ ] Mode row 1 is LSB, USB, DIG-U
- [ ] Mode row 2 is CW, AM, FM
- [ ] Each mode button still selects that mode and latches
- [ ] Left rail text is centered: Connect, Phones, Digital, Remote, Lo/Hi/CW/Step, Clear RIT
- [ ] Right rail text is centered: PTT, TUN, AGC, AMP, CMP, S/W, the mode buttons, NB/NR/AN/MON, LOW/MED/HIGH, CW −/+/pitch, LOG, Save settings
- [ ] Band bar still centered (2200 through GEN)
- [ ] Tab buttons centered: MAIN, CW (R/P memory), RX/TX (QRP, AMP, 2 TONE, reserved), FAVORITES, QRP CAL, AMP CAL, RX IQ, TX IQ, FREQ CAL, SETTINGS
- [ ] Debug Log, Remote Audio, and Spectrum/Waterfall window buttons centered
- [ ] No clipped labels (watch DIG-U, Save settings, Remote, 2 TONE, Apply listen)
- [ ] Selected and TX-on (red PTT/TUN/AMP) still look right

## Commit (commit only, no installer, no GitHub release)

Stew's rule: Build commits on this host; Stew pushes. Commit, then stop. Do not push.

```
git add Avalonia/src/MSCC.Avalonia/App.axaml Avalonia/src/MSCC.Avalonia/Views/MainWindow.axaml
git status
git commit -m "Avalonia: WPF mode-button order and centered button text"
```

Never add: `.deb` files, `Avalonia/publish/`, `installers/`, Visual Studio bookkeeping, or other
people's dirty files.
