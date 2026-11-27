# cmd-099b — Avalonia: SPLIT safety gaps + S-meter / ALC meter faces and frames

> **STOP — read first.** Build commits only. **No push, no installer, no GitHub Release, no version bump** (Avalonia stays **1.7.08**). Avalonia only: `Avalonia/src/MSCC.Avalonia`. **Do not touch** `rpi/`, `Solidus/`, `installers/rpi/`, the WPF tree, `Controls/UiChromeTheme.cs`, or `mscc-ui/Avalonia-Migration`. No new radio protocol commands: SPLIT changes use the existing `SetSplitAsync(false)` only. Do not commit bin/obj/publish/.deb/VS files or `.mscc-coord/status/ubuntu-stew.md`. Do not pull, merge or rebase. Stew pushes. Ask Stew before any destructive git step.

Follows cmd-099 (Build `6789baeb`). Stew radio-tested cmd-099: all OK. FM is a work in progress: keep FM changes to the one line in §2.

Line numbers below are from `6789baeb`; find each anchor by name if they moved.

## 1. Turn user SPLIT off when a calibration / IQ session starts

Problem: the VFO row guard blocks the SPLIT button during cal/IQ, but if SPLIT is already on when a session starts, the server still retunes to `G_Split_TX_FREQ` on PTT. A QRP/amp/TX-IQ calibration would transmit on VFO B's frequency.

Add one helper in `ViewModels/MainViewModel.cs` next to `ClearUserSplitOnDisconnectAsync` (~7245):

```csharp
/// <summary>A cal/IQ session must transmit on its own frequency. Drop a user split first.</summary>
private async Task EndUserSplitForSessionAsync(string session)
{
    if (!SplitOn)
        return;
    if (_radio != null && IsConnected)
    {
        try
        {
            await _radio.SetSplitAsync(false).ConfigureAwait(true);
            AppendLog($"Split off ({session})");
        }
        catch (Exception ex)
        {
            AppendLog($"Split off ({session}) failed: {ex.Message}");
        }
    }
    SplitOn = false;
}
```

Call it with `await EndUserSplitForSessionAsync("…")` **in the start branch only**, **before the first frequency, mode, TUNE or PTT send** of the session. Start points (all in `MainViewModel.cs`):

| Session | Method (line) | Start branch sets | Label |
|---|---|---|---|
| QRP power cal TX | `TogglePowerCalTxAsync` (4696) | `PowerCalTxOn = true` | `"QRP cal TX"` |
| QRP power cal calibrate | `TogglePowerCalCalibrateAsync` (4752) | `PowerCalCalibrating = true` (4786) | `"QRP cal"` |
| Amp cal TX | `ToggleAmpCalTxAsync` (5074) | `AmpCalTxOn = true` (5113) | `"amp cal TX"` |
| Amp cal calibrate | `ToggleAmpCalCalibrateAsync` (5128) | `AmpCalCalibrating = true` (5197) | `"amp cal"` |
| RX IQ | `StartRxIqAsync` (5419) | `RxIqSessionActive = true` (5482) | `"RX IQ"` |
| TX IQ | `ToggleTxIqTxAsync` (5770) | `TxIqTxOn = true` (5818) | `"TX IQ"` |
| Freq cal auto | `StartFreqCalAutoAsync` (6108) | `FreqCalInProgress = true` (6125, and the second start at 6335 if it is a separate entry) | `"freq cal"` |
| Freq cal manual | `ToggleFreqCalManualAsync` (6149) | `FreqCalManualMode = true` (6169) | `"freq cal manual"` |

- `ToggleFreqCalLooseAsync` (6048) is a setting, not a session: no call.
- Grep for any other place that sets one of the ten `IsCalibrationOrIqSessionActive()` flags (`PowerCalCalibrating`, `PowerCalTxOn`, `PowerCalAcceptPrompt`, `AmpCalCalibrating`, `AmpCalTxOn`, `AmpCalAcceptPrompt`, `TxIqTxOn`, `RxIqSessionActive`, `FreqCalInProgress`, `FreqCalManualMode`) to `true` from idle. Accept prompts follow a calibrate run, so they need no call. Report the final list of call sites.
- **FM offset split is not touched** (FM is work in progress). The helper only acts on the user `SplitOn` flag.

## 2. `ClearFmSplitAsync` clears the SPLIT flag

`ClearFmSplitAsync` (~7463): right after `await _radio.SetSplitAsync(false)`, add `SplitOn = false;`. Nothing else in the FM code changes.

## 3. Lost link clears the SPLIT flag

`OnIsConnectedChanged` (~1260), inside the existing `if (!value)` block: add `SplitOn = false;`. This covers every path that sets `IsConnected = false` (connect failure ~1953, disconnect ~2021, close ~2264, link loss). Disconnect still sends split off first through `PrepareDisconnectAsync` → `ClearUserSplitOnDisconnectAsync`, because that runs before `IsConnected` goes false. No radio send in `OnIsConnectedChanged`.

## 4. Meter labels: S-METER inside the face, larger text, S-meter level with ALC

### 4a. Remove the caption under the S-meter
- `Views/MainWindow.axaml` ~206: delete the `TextBlock Text="{Binding MeterCaption}"`. Replace the S-meter `StackPanel` wrapper with the `AnalogSMeterControl` directly in `Grid.Column="1"` with `VerticalAlignment="Center"` (same as the ALC control in column 5).
- Leave `MeterCaption` in the VM. Remove the `TextBlock.meter-caption` styles in `App.axaml` (~722–727) only if nothing else uses the class (`grep -rn meter-caption Avalonia/src --include=*.axaml`).

### 4b. S-meter center legend (`Controls/AnalogSMeterControl.axaml.cs`)
The caption carried the FWD / SWR and SWR FAULT text. The face legend takes it over:

| Meter state | Legend text | Color |
|---|---|---|
| RX (`!PowerMode`) | `S-METER` | `#FFA8A8A8` |
| TX power face (`PowerMode && !Fault`) | `FWD / SWR` | `#FF00CC99` (current power-legend green) |
| Fault (`PowerMode && Fault`) | `SWR FAULT` | `#FFFF5252` |

- `_centerLegend` (built ~513, `UpdateCenterLegend` ~660): FontSize **11**, Bold, same font family. Set text and color from the table in `UpdateCenterLegend`, and call `UpdateCenterLegend()` from the existing `FaultProperty` change handler (~160) as well as the `PowerModeProperty` one (~149).
- `PlaceCenterLegend` (~654): stop estimating width (`Length * 5.0`). Measure: `_centerLegend.Measure(Size.Infinity)`, then left = `_cx - DesiredSize.Width / 2`. Keep the top at `_cy - _radius * 0.42`.
- Width check: "SWR FAULT" / "FWD / SWR" = 9 chars × 0.602 × 11 ≈ **60 px** in DejaVu Sans Mono (Consolas is not installed on stew-HP), inside a ~170 px face. If it touches a scale label, move it down (raise the 0.42 factor to 0.36), do not shrink it.
- `FormatMultiplier` ("W") is no longer the legend. Leave it if still used elsewhere; remove it if unused.

### 4c. Scale numbers on both faces
- S-meter scale labels (`AddLabel` ~625–650, `UpdatePowerLabels` ~676–690): FontSize 8 → **9**, small 7 → **8**. Width estimates 4.8 → **5.4**, 4.0 → **4.8**.
- ALC scale labels (`Controls/AnalogAlcMeterControl.axaml.cs` ~353–365): FontSize 8 → **9**, the 100 label 7 → **8**; width estimates 4.8 → **5.4**, 4.2 → **4.8**.
- If any two neighboring labels touch at 180×120 on either face, keep that face's scale at the old sizes and report it.

### 4d. ALC center legend
- `AnalogAlcMeterControl.axaml.cs` ~368–378: `ALC` FontSize 8 → **11** Bold, color `#FFA8A8A8` (same as S-METER). Center by measuring (as in 4b), top `_cy - _radius * 0.42`.

### 4e. Level
Both meters: 180×120, `VerticalAlignment="Center"`, no wrapper. Their tops line up with each other and with the VFO block (also 120 tall: 92 + 4 + 24).

## 5. Meter frames match the VFO recess

Stew likes the VFO borders; both meters get the same frame. Reuse the VFO keys; one set of skin brushes for all recessed displays. In `App.axaml` add a comment above `UiVfoBezelBrush`: `<!-- Also used by the S-meter and ALC frames. -->`.

In **both** `Controls/AnalogSMeterControl.axaml` and `Controls/AnalogAlcMeterControl.axaml`, replace the outer `Border` (Background `#FF0C0C0C`, BorderBrush `#FF666666`, BorderThickness 1, CornerRadius 4, Padding 3,2) with:

```xml
<Border Background="{DynamicResource UiVfoBezelBrush}" CornerRadius="4" Padding="3">
  <Grid>
    <Border Background="#FF0C0C0C" CornerRadius="2" ClipToBounds="True"/>
    <Grid RowDefinitions="*,Auto" Margin="3,3,3,2">
      <!-- existing FaceCanvas + HOLD | reading | Peak row, unchanged -->
    </Grid>
    <Border BorderThickness="2,2,0,0" BorderBrush="{DynamicResource UiVfoBevelDarkBrush}"
            CornerRadius="2" IsHitTestVisible="False"/>
    <Border BorderThickness="0,0,2,2" BorderBrush="{DynamicResource UiVfoBevelLightBrush}"
            CornerRadius="2" IsHitTestVisible="False"/>
  </Grid>
</Border>
```

- The face stays dark (no cream LCD on meters). No `UiVfoLcdShadeBrush` on the meters.
- HOLD / Peak checkboxes, the reading box, the click-to-toggle S/dBm and the red RESET stay exactly as they are.
- Control size stays 180×120. `FaceCanvas` is now 180 − 6 − 6 = **168** wide (was 180 − 2 − 6 = 172). The face is built from `FaceCanvas.Bounds` (~449), so it rescales; check the S9/+60 and 100 labels are not clipped.

## Fit at 1024×600

- **Width:** unchanged. 180 (S) + 320 (VFO block) + 180 (ALC) = **680 ≤ 683**.
- **Height:** the top row was set by the S-meter stack (120 + 2 + ~15 caption ≈ 137). Now all three items are 120 → top panel = 120 + 4 (padding 2+2) + 2 (border) = **126**, about **16 px shorter**. The band row and the tabs move **up** about 16 px. Nothing moves down.

## Checklist (Build)

- [ ] `git fetch` + `git status -sb`; report if origin moved. Do not pull.
- [ ] §1 helper added; called in every start branch before the first send; final list of call sites reported.
- [ ] §2 `SplitOn = false` in `ClearFmSplitAsync`; no other FM change.
- [ ] §3 `SplitOn = false` in `OnIsConnectedChanged(false)`.
- [ ] §4 caption removed; S-meter legend S-METER / FWD / SWR / SWR FAULT at 11 pt; ALC legend 11 pt; scale labels 9/8 (or report why not); S-meter and ALC level.
- [ ] §5 both meter frames = VFO bezel + bevels; HOLD/Peak/reading unchanged.
- [ ] `dotnet build -c Release --no-incremental` for `MSCC.Avalonia`: 0 errors, no new warnings.
- [ ] Run at **1024×600** and 1280×720; screenshot each. Report: top panel height before/after, S-meter top Y vs ALC top Y (must be equal), any clipped label.
- [ ] One commit: `Avalonia: split off for cal/IQ and lost link; S-METER legend in face; meter frames match VFO`.
- [ ] Report commit hash, files changed, measurements.

## Stew's radio test (printable)

- [ ] No "S-METER" text under the meter; the face reads S-METER, larger.
- [ ] Key TX with the SWR meter on: face reads FWD / SWR. A latched fault reads SWR FAULT in red and RESET still works.
- [ ] ALC face text is larger and readable.
- [ ] S-meter and ALC sit at the same height, level with the VFOs.
- [ ] Both meter frames look like the VFO frames.
- [ ] HOLD and Peak work on both meters; click the S-meter reading toggles S / dBm.
- [ ] Band row and tabs moved up a little; nothing clipped at 1024×600.
- [ ] SPLIT on (SSB), then open QRP cal and start TX into the dummy load: SPLIT goes dark first and the cal transmits on the cal frequency, not VFO B.
- [ ] Same check for amp cal and TX IQ.
- [ ] SPLIT on → pull the network or stop the servers → SPLIT goes dark.
