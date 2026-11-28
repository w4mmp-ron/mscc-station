# cmd-103 — Avalonia: light-bar buttons on FAVORITES; light-bar buttons + physical sliders on QRP CAL, AMP CAL, TX IQ, FREQ CAL

> **STOP — read first.** Build commits only. **No push, no installer, no GitHub Release, no version bump** (Avalonia stays **1.7.08**). Avalonia only: `Avalonia/src/MSCC.Avalonia`. **Read the WPF tree for reference only — do not edit it.** **Do not touch** `rpi/`, `Solidus/`, `installers/rpi/`, **`Controls/UiChromeTheme.cs`**, or `mscc-ui/Avalonia-Migration`. No radio protocol commands. **Bindings, commands, CommandParameter, IsEnabled, tooltips, layout and text stay the same.** Do not commit bin/obj/publish/.deb/VS files or `.mscc-coord/status/ubuntu-stew.md`. Do not pull, merge or rebase. Stew pushes. Ask Stew before any destructive git step.

Follows cmd-102 (Build `f1479112`). Stew: layout and text on these tabs are all good; change **only buttons and sliders**. **RX IQ and SETTINGS are not in this command** (they need more work next).

Line numbers are from `f1479112` (`Views/MainWindow.axaml`).

## Rule

- Every `mscc-gold` button on the five tabs below → `Classes="mscc-bar"`. Keep every other class already on the button (`mscc-selected`, `mscc-tx-on`). Keep Width, Height, Margin, Padding, Content, Command, IsEnabled, ToolTip.
- Lit state:
  - **Red** (`Classes.mscc-tx-on`) only where a TX-on binding already exists (same as the right-rail TUN).
  - **Blue** (`Classes.mscc-selected`) where a selected/latched binding already exists.
  - Otherwise no lit class — do not invent new properties.
- Every Fluent `Classes="mscc-slider"` on these tabs → `Classes="mscc-physical"` (cmd-096 `MsccPhysicalSliderTheme`, 9 px track, no white highlight; the theme already sets `SliderWheel.OneStepPerNotch`).
- **Remove** `PointerWheelChanged="Slider_OnPointerWheelChanged"` from each slider you convert, so the wheel does not step twice. Leave the handler in `MainWindow.axaml.cs` — RX IQ and SETTINGS still use it. Keep `ValueChanged` handlers (QRP and AMP CAL). Keep SmallChange, LargeChange, TickFrequency, IsSnapToTickEnabled, Min/Max, Value, IsEnabled, ToolTip.
- TX IQ OFFSET and PHASE: wheel must stay **one step per notch**, matching WPF (pending todo, still required). `mscc-physical` without the extra wheel handler is how that stays true.
- Boxed values already sit beside these sliders. Restyle the box to match cmd-102 (Background `#FF444444`, BorderBrush `#FF888888`, BorderThickness 1, Padding `3,1`, value TextBlock FontSize 11 Bold, `UiAccentBrush`, centered). Keep a larger `MinWidth` when the text is a step label, not a short number (QRP `PowerCalStepLabel` box stays `MinWidth="150"`; AMP `AmpCalStepLabel` stays `MinWidth="90"`). Do not add or remove boxes. The FREQ CAL PPM box (no slider) gets the same chrome; − / + around it become `mscc-bar`.
- Do not change headings, help text, lists, checkboxes, or layout (rows, columns, spacing). Do not restyle RX IQ or SETTINGS.

## Inventory (convert all of these)

### FAVORITES (866–938) — buttons only, no sliders

| Line | Content | Command | Lit |
|---|---|---|---|
| 884 | SAVE | `SaveFavoriteCommand` | none |
| 887 | RECALL | `RecallFavoriteCommand` | none |
| 890 | DELETE | `DeleteFavoriteCommand` | none |

### QRP CAL (939–1050)

**Buttons**

| Line | Content | Command | Lit (keep) |
|---|---|---|---|
| 985–989 | band `BandLabel` (ItemsControl, one per band) | `SelectPowerCalBandCommand` | `mscc-selected` = `IsSelected` |
| 1020–1022 | `PowerCalTxButtonText` | `TogglePowerCalTxCommand` | **red** `mscc-tx-on` = `PowerCalTxOn` |
| 1025–1027 | `PowerCalCalibrateButtonText` | `TogglePowerCalCalibrateCommand` | blue `mscc-selected` = `PowerCalCalibrating` |
| 1029 | RESET ALL | `HardResetPowerCalCommand` | none |
| 1036 | Yes | `AcceptPowerCalCommand` | none |
| 1039 | No | `CancelPowerCalCommand` | none |

**Slider** (1003–1019): POWER, `mscc-slider` → `mscc-physical`, Value `PowerCalSliderValue`, Min 0 Max 100, `IsEnabled="{Binding PowerCalCalibrating}"`, keep `ValueChanged="PowerCalSlider_OnValueChanged"`, drop wheel handler. Boxed `PowerCalStepLabel` (MinWidth 150) — restyle only.

### AMP CAL (1051–1162)

**Buttons**

| Line | Content | Command | Lit (keep) |
|---|---|---|---|
| 1097–1101 | band `BandLabel` | `SelectAmpCalBandCommand` | `mscc-selected` = `IsSelected` |
| 1132–1134 | `AmpCalTxButtonText` | `ToggleAmpCalTxCommand` | **red** `mscc-tx-on` = `AmpCalTxOn` |
| 1137–1139 | `AmpCalCalibrateButtonText` | `ToggleAmpCalCalibrateCommand` | blue `mscc-selected` = `AmpCalCalibrating` |
| 1141 | RESET ALL | `HardResetAmpCalCommand` | none |
| 1148 | Accept | `AcceptAmpCalCommand` | none |
| 1150 | Reject | `RejectAmpCalCommand` | none |
| 1152 | Cancel | `CancelAmpCalCommand` | none |

**Slider** (1108–1128): POWER, `mscc-slider` → `mscc-physical`, Value `AmpCalSliderValue`, Min −99 Max 0, `IsEnabled="{Binding AmpCalCalibrating}"`, keep `ValueChanged="AmpCalSlider_OnValueChanged"`, drop wheel handler. Two boxes: `AmpCalSliderValue` (MinWidth 48) and `AmpCalStepLabel` (MinWidth 90) — restyle only.

### TX IQ (1269–1372)

**Buttons**

| Line | Content | Command | Lit (keep) |
|---|---|---|---|
| 1289–1294 | band `BandLabel` | `SelectTxIqBandCommand` | `mscc-selected` = `IsSelected` |
| 1345–1347 | `TxIqTxButtonText` | `ToggleTxIqTxCommand` | **red** `mscc-tx-on` = `TxIqTxOn` |
| 1349 | SAVE | `ApplyTxIqCommand` | none |
| 1352 | RESET ALL | `ResetAllTxIqCommand` | none |
| 1359 | Confirm | `ConfirmResetAllTxIqCommand` | none |
| 1361 | Cancel | `CancelResetAllTxIqCommand` | none |

**Sliders** — all `mscc-slider` → `mscc-physical`, drop wheel handler, restyle the boxed value (MinWidth 48 / 56 as now):

| Line | Label | Value | Min/Max | IsEnabled |
|---|---|---|---|---|
| 1304 | POWER | `TxIqPower` | 0…100 | (as now) |
| 1317 | OFFSET | `TxIqOffset` | −200…200 | `TxIqTxOn` |
| 1331 | PHASE | `TxIqPhase` (box shows `TxIqPhaseText`) | −100…100, TickFrequency 1, snap on | `TxIqTxOn` |

PHASE SmallChange stays 1 (0.05° per tick in the VM). One wheel notch = one tick.

### FREQ CAL (1373–1484) — buttons only, no sliders

| Line | Content | Command | Lit (keep or add from existing state) |
|---|---|---|---|
| 1384 | `FreqCalLooseButtonText` | `ToggleFreqCalLooseCommand` | add `mscc-selected` = `FreqCalLoose` (property already exists; text is already LOOSE/TIGHT) |
| 1388 | AUTO | `FreqCalAutoCommand` | none |
| 1392–1394 | `FreqCalManualButtonText` | `ToggleFreqCalManualCommand` | blue `mscc-selected` = `FreqCalManualMode` (already there) |
| 1397 | CHECK | `FreqCalCheckCommand` | none |
| 1401 | RESET | `FreqCalResetCommand` | none |
| 1405 | STOP | `FreqCalStopCommand` | none (`IsEnabled` is already `FreqCalInProgress`) |
| 1417 | − | `FreqCalPpmMinusCommand` | none |
| 1425 | + | `FreqCalPpmPlusCommand` | none |
| 1440 | Coarse ±250 Hz | `FreqCalAutoCoarseCommand` | none |
| 1442 | Fine ±50 Hz | `FreqCalAutoFineCommand` | none |
| 1444 | Cancel (auto prompt) | `FreqCalAutoCancelCommand` | none |
| 1451 | Accept (manual) | `AcceptFreqCalManualCommand` | none |
| 1453 | Reject | `RejectFreqCalManualCommand` | none |
| 1460 | Confirm (reset) | `ConfirmFreqCalResetCommand` | none |
| 1462 | Cancel (reset) | `CancelFreqCalResetCommand` | none |

Restyle the PPM box at 1421–1423 only (chrome). No slider on this tab.

## Fit

Chrome only, so sizes stay essentially the same. Physical sliders are the same row height as on RX/TX (24-ish). At 1024×600 and 1280×720: no **new** scroll bar on FAVORITES, QRP CAL, AMP CAL, TX IQ or FREQ CAL (if a tab already scrolls, it must not get worse). Report any tab that scrolls and whether that bar was already there.

## Checklist (Build)

- [ ] `git fetch` + `git status -sb`; report if origin moved. Do not pull.
- [ ] Every gold button in the inventory is `mscc-bar`; lit classes as in the tables; no new properties except the one `mscc-selected` on FreqCalLoose.
- [ ] Every listed slider is `mscc-physical`; wheel handlers removed on those sliders only; `ValueChanged` kept; OFFSET/PHASE one step per notch.
- [ ] Boxed readouts restyled; long step-label MinWidths kept.
- [ ] RX IQ and SETTINGS untouched. Layout and text on the five tabs untouched.
- [ ] No new scroll bars at **1024×600** or 1280×720. Report which of these five tabs scroll.
- [ ] `dotnet build -c Release --no-incremental`: 0 errors, no new warnings.
- [ ] Screenshots of all five tabs at both sizes.
- [ ] One commit: `Avalonia: light-bar buttons on Favorites and cal tabs; physical sliders on QRP/AMP/TX IQ`.
- [ ] Report commit hash, files changed, measurements.

## Stew's check (printable)

- [ ] FAVORITES: SAVE, RECALL, DELETE look like the front-panel light-bar buttons and still work.
- [ ] QRP CAL and AMP CAL: band buttons, RESET, and the Yes/No or Accept/Reject/Cancel prompts are light-bar; TX is red when transmitting; CALIBRATE is blue when a run is on; the POWER slider matches the left rail and the boxed value still updates.
- [ ] TX IQ: TX is red when on; POWER/OFFSET/PHASE sliders match the left rail; one wheel notch moves OFFSET or PHASE one step; SAVE / RESET ALL still work.
- [ ] FREQ CAL: all buttons are light-bar; LOOSE is blue when loose is on; MANUAL is blue when manual is on; − / + still step PPM.
- [ ] Layout and wording on these tabs are unchanged. RX IQ and SETTINGS are unchanged.
- [ ] At 1024×600, no new scroll bar on these five tabs.
