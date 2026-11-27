# cmd-099 — Avalonia VFO polish + VFO button row (A>B, B>A, A<>B, SPLIT, STEP)

> **STOP — read first.** Build commits only. **No push, no installer, no GitHub Release, no version bump** (Avalonia stays **1.7.08**). Avalonia only: `Avalonia/src/MSCC.Avalonia`. **Do not touch** `rpi/`, `Solidus/`, `installers/rpi/`, the WPF tree, `Controls/UiChromeTheme.cs`, or `mscc-ui/Avalonia-Migration`. No new radio protocol commands. Do not commit bin/obj/publish/.deb/VS files or `.mscc-coord/status/ubuntu-stew.md`. Do not pull, merge or rebase. Stew pushes. Ask Stew before any destructive git step.

Follows cmd-098 (Build `9ecffa23`). Stew tested cmd-098 on the radio: all checks passed. The ALC meter is clipped about 9 px at 1024×600; this command fixes that.

## Goal

1. No green border around the VFOs. The dimmed (gray) VFO is the inactive indicator.
2. A red LED dot next to "VFO A" / "VFO B" on the active VFO only.
3. Larger text: VFO labels, mode text, frequency digits (big digits and Hz digits).
4. No hover step text on VFO A.
5. Top row fits at 1024×600: ALC meter fully visible.
6. A row of `mscc-bar` light-bar buttons under the two VFOs: **A>B, B>A, A<>B, SPLIT, STEP**.
7. Everything fits at 1024×600. The band row and the tabs do not move down.

## What already exists (investigated by the Commander — use these, nothing new on the wire)

| Need | Existing path | File |
|---|---|---|
| Split on/off | `IRadioService.SetSplitAsync(bool)` → `CMD_SET_SPLIT` (0x31) | `MSCC.Core/Services/IRadioService.cs` 153–159, `UdpRadioService.cs` 712–732 |
| Split RX freq | `SetSplitRxFreqAsync(long)` → `CMD_SET_SPLIT_RX_FREQ` (0x34) | same |
| Split TX freq | `SetSplitTxFreqAsync(long)` → `CMD_SET_SPLIT_TX_FREQ` (0x39) | same |
| Push active VFO freq+mode | `PushActiveVfoToRadioAsync(bool force)` (CMD_SET_VFO, freq, mode) | `ViewModels/MainViewModel.cs` ~7074 |
| Select VFO | `SelectVfoAsync(bool useVfoA)` | `MainViewModel.cs` ~7056 |
| Tune active VFO | `ApplyFrequencyAsync(long hz, string reason)` | `MainViewModel.cs` ~6958 |
| Step cycle | `CycleStepCommand` (`CycleStep()`, sends `SetStepAsync`) | `MainViewModel.cs` ~2543 |
| FM offset split (RX=A, TX=B) | `ApplyFmOffsetPolicyAsync()` / `ClearFmSplitAsync()` / `IsFmOffsetActive()` | `MainViewModel.cs` ~7191 / ~7231 / ~7013 |

Server side (read-only check, no change): `linux/ms-sdr-linux/source/main-controller.c` 1505–1527 stores `G_Split`, `G_Split_RX_FREQ`, `G_Split_TX_FREQ`; `main.c` 869–885 retunes the hardware to `G_Split_TX_FREQ` on PTT and back to `G_Split_RX_FREQ` on release when split is on. This is mode-agnostic. The same handlers exist in the Pi and Windows `ms-sdr` sources. The server does not change mode on TX: **split TX uses the current (VFO A) mode.**

WPF (`MSCC.Wpf/MainWindow.xaml`) has **no** A>B, B>A, A<>B or SPLIT buttons. WPF uses split only for the FM offset (Simplex unchecked → TX on VFO B = A − 100 kHz). So these buttons are new Avalonia-only VM logic on top of existing commands. That is allowed in this command.

## Changes

### 1. Remove the green border
- `Views/MainWindow.axaml`, `VfoAPanel` and `VfoBPanel`: delete the inline `BorderBrush="…"` and `BorderThickness="…"`.
- `App.axaml`, styles `Border.mscc-vfo-active` / `Border.mscc-vfo-idle`: remove the `BorderBrush` and `BorderThickness` setters. Keep `Opacity` (active 1, idle 0.78). The idle dimming stays exactly as it is.
- The bezel (`UiVfoBezelBrush`, CornerRadius 4, Padding 3) stays.

### 2. Red active dot
- `App.axaml`: add `UiVfoActiveDotBrush` next to the other `UiVfo*` brushes. Use a small radial gradient so it reads as an LED: center `#FFFF5A4A`, edge `#FFB01818`.
- In each VFO's label cell, put the label in a horizontal StackPanel: `Ellipse` 7×7, `Fill="{DynamicResource UiVfoActiveDotBrush}"`, Margin `0,0,4,0`, VerticalAlignment Center, then the "VFO A" / "VFO B" TextBlock.
- Visibility: `IsVisible="{Binding UseVfoA}"` on A, `IsVisible="{Binding UseVfoB}"` on B. Use `IsVisible`, not Opacity, so the dot is not drawn at all on the idle VFO. Keep the ellipse's layout space: wrap it in a 7×7 Panel so the label text does not shift when the dot hides.

### 3. Larger text
Both VFOs get the **same** sizes (they are now the same width, see §5):

| Element | cmd-098 | cmd-099 |
|---|---|---|
| "VFO A" / "VFO B" label | 8 | **10** Bold |
| Mode text | 10 | **12** Bold |
| Big digits (MHz.kHz) A / B | 26 / 22 | **27 / 27** Bold |
| Hz digits A / B | 18 / 15 | **19 / 19** Bold |
| Band strip text | 9 | 9 (unchanged) |

- Content Grid in each LCD: `Margin="4,2,4,3"`, `RowDefinitions="15,*,15"` (row 0 grows from 13 to 15 for the 12 pt mode text).
- Keep the Hz TextBlock bottom-aligned with its `Margin="3,0,0,2"` and `Tag="0"`; big digits keep `Tag="3"`.
- Digit hit-test: `TryGetDigitStepHz` measures the rendered TextBlock text, so it scales with font size. Do not change its math. Verify it after the font change (checklist below).

### 4. Drop the hover step text
- Remove the `VfoTuneStepDisplay` TextBlock from VFO A. Mode text sits alone in the right side of row 0, right-aligned.
- In `MainWindow.axaml.cs`, keep the digit hit-test and wheel tuning. Stop calling `vm.SetHoverTuneStep(...)` (or leave the call; nothing binds to it). Remove `VfoTuneStepDisplay` and `HoverTuneStepLabel` from the VM only if nothing else binds them (`grep -rn "VfoTuneStepDisplay\|HoverTuneStepLabel" Avalonia/src`).
- STEP moves to the new button row (§6).

### 5. Top row width at 1024×600

Current (cmd-098): center column = 1024 − 8 (window margin) − 320 (two 160 rails) − 3 (center margin) = **693**. Top panel border 2 + padding 8 (`Padding="4,2"`) → **683** usable. Content 180 (S) + 176 (VFO A) + 156 (VFO B) + 180 (ALC) = **692** → **9 px over** (the clipped ALC rim).

New layout:
- Top Grid `ColumnDefinitions="*,Auto,*,Auto,*,Auto,*"` (7 columns): S-meter col 1, **VFO block** col 3, ALC col 5.
- VFO block = one Grid: `ColumnDefinitions="154,6,154"`, `RowDefinitions="92,4,24"`, `Margin="3,0"`. VFO A at (0,0), VFO B at (0,2), button row at (2,0) ColumnSpan 3.
- Block width 154 + 6 + 154 + 6 (margin) = **320**.
- Row total 180 + 320 + 180 = **680 ≤ 683** → 3 px spare at 1024 (shared by the star columns). At 1280 the star columns share 939 − 680 = 259 px.

Digit fit inside a 154-wide VFO: 154 − 6 (bezel padding 3+3) − 8 (content margin 4+4) = **140 px**. Worst case on stew-HP: `Consolas` is not installed, `fc-match` falls back to **DejaVu Sans Mono** (advance 0.602 em). "28.074" at 27 pt = 6 × 0.602 × 27 = **97.5**; "000" at 19 pt = 3 × 0.602 × 19 = **34.3**; + 3 margin = **134.8 ≤ 140**. Widest real value is 6 characters ("54.000", "0.136" is 5). On Windows Consolas is narrower (0.55 em). Do not go above 27/19.

### 6. VFO button row

Five `Button Classes="mscc-bar"` in a `UniformGrid Columns="5"` (or Grid with 5 equal `*` columns, ColumnSpacing 3), Height 24, FontSize 10, Bold, spanning the full 314 px under both VFOs → (314 − 12) / 5 = **60.4 px** each.

| Button | Content | Command (new, in `MainViewModel`) | Lit (`mscc-selected`) |
|---|---|---|---|
| A>B | `A>B` (XAML `A&gt;B`) | `CopyVfoAToBCommand` | never |
| B>A | `B>A` | `CopyVfoBToACommand` | never |
| A<>B | `A<>B` (XAML `A&lt;&gt;B`) | `SwapVfosCommand` | never |
| SPLIT | `SPLIT` | `ToggleSplitCommand` | `Classes.mscc-selected="{Binding SplitDisplayOn}"` |
| STEP | `{Binding StepLabel}` | existing `CycleStepCommand` | never |

Tooltips (plain): A>B "Copy VFO A frequency and mode to VFO B." · B>A "Copy VFO B frequency and mode to VFO A." · A<>B "Swap VFO A and VFO B." · SPLIT "Split: receive on VFO A, transmit on VFO B." · STEP "Tuning step. Click to change." Also keep VFO A double-click → `CycleStepCommand` as today.

All four new commands: `[RelayCommand(CanExecute = nameof(CanOperate))]` (same CanExecute as `CycleStep`, already notified on connect/disconnect).

**Guard (all four):** if `IsCalibrationOrIqSessionActive()` → `StatusText = "VFO buttons are off during calibration / IQ."`, return. If `IsFmOffsetActive()` (FM with Simplex unchecked) → `StatusText = "FM offset controls VFO B. Check Simplex to use VFO buttons."`, return. FM offset owns VFO B and split; these buttons never fight it.

**CopyVfoAToB:**
```
_vfoBFrequencyHz = _frequencyHz; VfoBDisplayMhz = FormatMhz(_frequencyHz); VfoBModeText = ModeText;
if (!UseVfoA) await PushActiveVfoToRadioAsync(force: false);   // radio is on B → retune it
if (SplitOn) await _radio.SetSplitTxFreqAsync(_vfoBFrequencyHz);
AppendLog("A>B ..."); ScheduleSaveClientSettings();
```
**CopyVfoBToA:** mirror: `_frequencyHz = _vfoBFrequencyHz; UpdateFrequencyUi(_frequencyHz); ModeText = VfoBModeText; NotifyModeFlags();` then `if (UseVfoA) await PushActiveVfoToRadioAsync(force: false);` then `if (SplitOn) await _radio.SetSplitRxFreqAsync(_frequencyHz);`
**SwapVfos:** swap freq and mode between A and B (update both displays, `NotifyModeFlags()`), then `await PushActiveVfoToRadioAsync(force: false)` (the active VFO now holds the other value), then if `SplitOn` send `SetSplitRxFreqAsync(_frequencyHz)` and `SetSplitTxFreqAsync(_vfoBFrequencyHz)`.
Skip any copy when the source frequency is ≤ 0 (idle connect, no radio report yet).

**SPLIT** — new `[ObservableProperty] private bool _splitOn;` (not saved to settings; false at start). `SplitDisplayOn => SplitOn || (IsConnected && IsFmOffsetActive())` (notify when `SplitOn`, `FmSimplex`, `ModeText`, `IsConnected` change) so the button is also lit while the FM offset runs split.
- **On:** if `!UseVfoA` → `await SelectVfoAsync(true)` (RX is VFO A). Then `SetSplitRxFreqAsync(_frequencyHz)`, `SetSplitTxFreqAsync(_vfoBFrequencyHz)`, `SetSplitAsync(true)` — the same order as `ApplyFmOffsetPolicyAsync`. `SplitOn = true`. Log `Split on — RX A=… TX B=…`. If VFO B frequency is ≤ 0, refuse with StatusText "Set VFO B first."
- **Off:** `SetSplitAsync(false)`; `SplitOn = false`; log `Split off`.
- **While SplitOn:**
  - `ApplyFrequencyAsync` with `UseVfoA`: after `SetFrequencyAsync(hz)`, also `await _radio.SetSplitRxFreqAsync(hz)` (otherwise PTT release returns to the old RX frequency).
  - Tuning VFO B (wheel/digit on VFO B, `NudgeFrequencyByDigitAsync(..., vfoA:false)`): **do not select VFO B.** Update `_vfoBFrequencyHz` / `VfoBDisplayMhz` locally and send `SetSplitTxFreqAsync(hz)` only. The receiver stays on VFO A. (Add an early branch at the top of `NudgeFrequencyByDigitAsync`.)
  - Clicking the VFO B face (`SelectVfoAsync(false)`): turn split off first (`SetSplitAsync(false)`, `SplitOn = false`, StatusText "Split off — VFO B selected"), then select B as today.
  - Entering FM (`SetModeAsync` path where `nowFm` and FM offset applies): set `SplitOn = false` before `ApplyFmOffsetPolicyAsync()`. FM then owns split as today. Leaving FM: unchanged (`ClearFmSplitAsync` turns split off).
  - Disconnect: if `SplitOn`, send `SetSplitAsync(false)` before closing (best effort, catch/log), then `SplitOn = false`. The server keeps `G_Split` across client sessions, so leaving it on would make the next session transmit on a stale frequency.
- TX mode in split is VFO A's mode (server behavior). Do not add a mode send on PTT.

### 7. Height budget (top row must not grow)

The top row height is set by the S-meter column: meter 120 + caption margin 2 + caption line (11 pt Bold ≈ 14–15) ≈ **136–137 px**. ALC is 120.

VFO block: 92 (VFOs) + 4 (gap) + 24 (button row) = **120 ≤ 136**. Set the VFO block `VerticalAlignment="Center"` → the VFOs sit ~8 px higher than now and the button row ends ~8 px above the panel bottom. The top panel, band row and tabs stay where they are. VFO height stays **92**.

If Build measures the top panel taller after the change (compare against cmd-098 at 1024×600), reduce the button row to Height 22 before anything else.

## Checklist (Build)

- [ ] `git fetch` + `git status -sb`; report if origin moved. Do not pull.
- [ ] §1 border removed (inline + style setters); Opacity dimming kept.
- [ ] §2 `UiVfoActiveDotBrush` + dot on active VFO only; label does not shift.
- [ ] §3 font sizes per table; row 0 = 15; margins 4,2,4,3.
- [ ] §4 hover step text removed from VFO A.
- [ ] §5 7-column top grid; VFO block 154/6/154, margin 3,0.
- [ ] §6 five buttons; four new commands + `SplitOn` / `SplitDisplayOn`; all guards.
- [ ] §7 VFO block centered; top panel height unchanged.
- [ ] `dotnet build` of `MSCC.Avalonia` clean (0 errors, no new warnings).
- [ ] Run at **1024×600** and at 1280×720: screenshot each. ALC right rim fully visible at 1024; no clipping anywhere; band row and tabs at the same Y as cmd-098.
- [ ] Measure and report: top panel height before/after, VFO block width, rendered width of "28.074" + "000" in VFO A and VFO B.
- [ ] Digit hit-test (code check + log if possible): hover math still maps each big digit to 10^n kHz and each Hz digit to 10^n Hz.
- [ ] Commit only the changed Avalonia files. One commit: `Avalonia: VFO polish (no border, active dot, larger text) + A>B/B>A/A<>B/SPLIT/STEP row; top row fits 1024`.
- [ ] Report: commit hash, files changed, measurements, anything that did not fit.

## Stew's radio test (printable)

- [ ] No green border on either VFO. Idle VFO is dimmed.
- [ ] Red dot next to the active VFO label only; clicking the other VFO moves the dot.
- [ ] Labels, mode and digits are larger and readable; no text clipped.
- [ ] No "step …" text on VFO A.
- [ ] At 1024×600 the ALC meter is fully visible.
- [ ] Wheel over each big digit and each Hz digit tunes by that digit (both VFOs).
- [ ] STEP shows the current step; click cycles it; left-panel Step button shows the same value. VFO A double-click still cycles.
- [ ] A>B: VFO B shows VFO A's frequency and mode.
- [ ] B>A: VFO A shows VFO B's frequency and mode; radio retunes if A is active.
- [ ] A<>B: values swap; radio follows the active VFO.
- [ ] SPLIT on (SSB, dummy load, low power): button lit blue, VFO A active. Set VFO B 5 kHz away. Key TX: transmits on VFO B frequency; unkey: receives on VFO A.
- [ ] With SPLIT on: tune VFO A → RX follows; key/unkey → RX returns to the new A frequency.
- [ ] With SPLIT on: wheel VFO B → B changes, receiver stays on A; key → TX on new B.
- [ ] With SPLIT on: click VFO B face → SPLIT goes dark, VFO B active.
- [ ] FM with Simplex unchecked: SPLIT lit (FM offset); A>B/B>A/A<>B/SPLIT show the "FM offset controls VFO B" status and do nothing. Check Simplex → SPLIT dark.
- [ ] SPLIT on → Disconnect → Connect: SPLIT is dark and the radio is not in split (key → TX on VFO A).
