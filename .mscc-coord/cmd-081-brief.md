# cmd-081: Avalonia slider mouse wheel (Part A, done) and QRP CAL TUNE mode (Part B)

Host: **ubuntu-stew** (stew-HP). Repo: `/home/stew/Documents/GitHub/mscc-station`.
HEAD on stew-HP is already `87e12aa` (Ron's AGC/packaging reply) plus uncommitted Part A+B (Avalonia 1.7.03). The Finished / commit section still starts with `git pull --rebase --autostash`. Do not revert or redo the Part A or Part B files.

**Avalonia client only.** No WPF changes (that brief comes later, on Shack). No server changes, no firmware.
**Do not edit** `rpi/`, `linux/` servers, `Solidus/`, or `installers/rpi/`.
Minimal change. Line numbers below for Part B are from the current stew-HP tree after Part A; check them before you edit.

**Part A and Part B are done and tested.** Stew said "finished" (t902u). The **Finished / commit** section at the end of this file is now in force. Do not start cmd-082 until that commit is done.

---

# Part A — slider mouse wheel (DONE — do not redo)

**Status:** Built and installed as Avalonia 1.7.03. Stew tested it: **image moves live per notch.** Leave the wheel handler, SmallChange values, AGC exclusion, and 1.7.03 version alone.

The original Part A instructions are kept below for the record only.

## What to do (Part A, already done)

WPF already wheels every main-window slider: one notch = `SmallChange` (or 1 if it is 0), clamped to Min/Max, `e.Handled = true`, no Ctrl / LargeChange. That is `Slider_PreviewMouseWheel` in `MSCC.Wpf/MainWindow.xaml.cs` 856-863. The panadapter / S/W window has a different Ctrl+LargeChange handler; do not copy that.

Avalonia has no slider wheel today. Only the VFO face / digit text have `PointerWheelChanged`. Leave those alone.

Add **one** Avalonia handler that mirrors the WPF method exactly, attach it to the same main-window sliders WPF wheels (list below), and set missing `SmallChange` values to match WPF. **Leave the AGC FAST RELEASE slider alone** until Ron answers the range question in `.mscc-coord/NOTE-FOR-RON-AGC-FAST-RELEASE-RANGE-2026-11-15.md` (commit `62bf9d9f`). Do not change its Maximum (100), do not add SmallChange, do not attach the wheel handler.

## Handler (add once, in `Views/MainWindow.axaml.cs`)

Put it next to the existing VFO wheel helpers. Reuse `WheelDirection` (297-301): `e.Delta.Y > 0` is one step up, `< 0` one step down, `0` is a no-op. One event = one `SmallChange`, same as WPF (`e.Delta > 0 ? step : -step`), not a multiple of the raw delta.

```csharp
private void Slider_OnPointerWheelChanged(object? sender, PointerWheelEventArgs e)
{
    if (sender is not Slider slider)
        return;
    int direction = WheelDirection(e);
    if (direction == 0)
        return;
    double step = slider.SmallChange > 0 ? slider.SmallChange : 1.0;
    slider.Value = Math.Max(slider.Minimum, Math.Min(slider.Maximum, slider.Value + direction * step));
    e.Handled = true;
}
```

`e.Handled = true` is required so the notch does not also scroll a parent or reach the VFO. Do not change the VFO handlers. They already return early when `e.Handled` is set.

## How to attach (simplest clean method)

Add `PointerWheelChanged="Slider_OnPointerWheelChanged"` on each included slider in `Views/MainWindow.axaml`, the same way WPF puts `PreviewMouseWheel="Slider_PreviewMouseWheel"` on each slider.

Do **not** hang the handler on the `Slider.mscc-slider` style in `App.axaml`. That class is also used by the AGC slider (and a style hook would make the AGC exclusion easy to miss). Do not attach at the window or tab level. Do not add a behavior / attached-property framework for this.

The AGC exclusion is then visible in the axaml: that slider simply does not get the attribute.

## SmallChange (match WPF)

Set `SmallChange` only where Avalonia is missing it. Do not add LargeChange, TickFrequency, or IsSnapToTick unless they are already there. In particular, **RIT = 10** and the **IQ sliders = 1**.

| Slider | Binding | axaml lines (check) | SmallChange now | Set to |
|---|---|---|---|---|
| Phones volume | `PVolume` | 70-72 | unset | **1** |
| Phones mic | `PMicGain` | 77-79 | unset | **1** |
| Digital volume | `DVolume` | 87-89 | unset | **1** |
| Digital mic | `DMicGain` | 94-96 | unset | **1** |
| Spectrum zoom | `SpectrumZoom` | 149-151 | unset | **1** |
| RIT offset | `RitOffset` | 160-162 | unset | **10** |
| Tune power | `TunePowerPercent` | 601-603 | unset | **1** |
| CW power | `CwPowerPercent` | 609-611 | unset | **1** |
| SSB power | `SsbPowerPercent` | 617-619 | unset | **1** |
| AM carrier | `AmCarrierPercent` | 625-627 | unset | **1** |
| FM power | `FmPowerPercent` | 634-637 | unset | **1** |
| QRP cal power | `PowerCalSliderValue` | 885-891 | already 1 | keep |
| AMP cal power | `AmpCalSliderValue` | 990-996 | already 1 | keep |
| RX IQ offset | `RxIqOffset` | 1084-1086 | unset | **1** |
| RX IQ LO fine | `RxIqFreqOffsetHz` | 1101-1103 | unset | **1** |
| TX IQ power | `TxIqPower` | 1172-1174 | unset | **1** |
| TX IQ offset | `TxIqOffset` | 1184-1186 | unset | **1** |
| TX IQ phase | `TxIqPhase` | 1197-1201 | already 1 | keep |
| RF power (right rail) | `RfPower` | 1599-1601 | unset | **1** |
| Compression | `Compression` | 1606-1608 | unset | **1** |
| NB pulse | `NbPulse` | 1669-1671 | unset | **1** |

**Exclude (do not attach, do not change range or SmallChange):**

- AGC FAST RELEASE (`AgcFastRelease`, 1613-1615). Max stays 100. Pending Ron.
- Settings color sliders (`UiBgR` / `UiBtn*` / `UiAcc*`, around 1371-1443).
- `SpectrumWaterfallWindow.axaml` (zoom, baseline, grid, waterfall, dB CAL).
- `RemoteAfWindow.axaml`.
- VFO wheel (already implemented).

## Wheel must send the same as a drag (important)

WPF has no "send on mouse-up only" path for these sliders. `Slider_PreviewMouseWheel` sets `slider.Value`; the TwoWay binding updates the ViewModel immediately; `OnXxxChanged` sends the command. QRP / AMP are the same (`OnPowerCalSliderValueChanged` / `OnAmpCalSliderValueChanged`). There is no DragCompleted handler. Mirror that: set `slider.Value` so the existing binding and command path run. Do not add a second opcode path.

Avalonia already documents a quirk: **int bindings often only commit on pointer-up.** QRP and AMP work around it with `ValueChanged="PowerCalSlider_OnValueChanged"` / `AmpCalSlider_OnValueChanged` (`MainWindow.axaml.cs` 154-164) calling `ApplyPowerCalSliderLive` / `ApplyAmpCalSliderLive`. Those send 0xA2 / 0x08 while dragging, not only on release. Setting `slider.Value` in the wheel handler will fire `ValueChanged`, so QRP / AMP should send. **Leave those live handlers as they are.**

TX IQ phase / offset / power, RX IQ offset / LO fine, RIT, volumes, and the power sliders have **no** live `ValueChanged` helper. They send only from the ViewModel `OnXxxChanged` hooks (for example `OnTxIqPhaseChanged` at 5469, which sends 0x2A when TX IQ is on). Setting `slider.Value` from the wheel is not a thumb drag, so the TwoWay binding should write the source immediately — but if it does not, the thumb would move and the radio would not get the command until a later click.

**Required check during the build:** with TX IQ TX on, one wheel notch on PHASE must send 0x2A the same way a drag-and-release does (watch the client log / status). Also confirm RX IQ offset (session on) sends 0x52, and QRP / AMP (CALIBRATE on) send 0xA2 / 0x08. If a bound int property stays at the old value after you set `slider.Value`, write the rounded value into that same ViewModel property from the handler so `OnXxxChanged` runs. Do not invent a new send path.

## Version (Part A, already done): Avalonia 1.7.02 → 1.7.03

Already bumped in the csproj and `DEBIAN/control`. **Part B must leave the version at 1.7.03** (uncommitted and unreleased). Do not bump to 1.7.04.

## Part A smoke (already passed)

Stew: title bar 1.7.03; **image moves live per notch** on the IQ sliders. VFO wheel unchanged. AGC unchanged.

---

# Part B — QRP CAL forces TUNE

**Do this now.** Minimal client change only.

## Why

QRP CAL **CALIBRATE** and **TX/TUNE** currently send only **0xAC** (`SetCalibrationTuneAsync`). They never set mode TUNE or 0xA6. After a main-tab mode change to USB/LSB/DIG-U, 0xAC keys the transmitter in that voice/digital mode (phone audio on the SA) instead of a tune carrier. AMP CAL, TX IQ and the main TUN button already set mode TUNE first. Selecting a QRP band (0xA1) happens to put trans/recv in TUNE, which is why re-selecting a band seemed to "fix" it; the next main-tab mode change breaks it again.

## What to change

`Avalonia/src/MSCC.Avalonia/ViewModels/MainViewModel.cs` only, plus a small leave-tab hook in `Views/MainWindow.axaml.cs` (needed because Avalonia does not currently unkey QRP when you leave the tab). Do not touch Part A wheel code, servers, WPF, or the version.

Mirror the existing AMP CAL pattern:

- Field, next to `_modeBeforeAmpCal` (89) / `_modeBeforeTxIq` (90) / `_modeBeforeTune` (92): add **`_modeBeforePowerCal`**, same style, default `"USB"`. Do not reuse the AMP field (the tabs must not share saved mode).
- Start path (AMP CAL 4830-4834 and 4913-4917): save current `ModeText` (if blank, use `"USB"`), `SetModeAsync("TUNE")`, `ModeText = "TUNE"`, **`SetAutoTuneAsync(true)` (this is 0xA6, `UdpRadioService.cs` 1159-1162)**, then the existing 0xAC ON.
- Stop path (AMP CAL `StopAmpCalTuneCarrierAsync` 4943-4956): 0xAC OFF, `SetAutoTuneAsync(false)`, restore the saved mode (`SetModeAsync` + `ModeText`). If the saved value is blank or `"TUNE"`, restore `"USB"`.

### Start: `TogglePowerCalTxAsync` (4462-4504) and `TogglePowerCalCalibrateAsync` start (4507-4564)

When turning TX on, or when starting CALIBRATE (the `if (!PowerCalCalibrating)` branch that already does 0xA2 0 then 0xAC 1):

1. Save the mode **only if `ModeText` is not already TUNE**. If it is already TUNE, keep the existing `_modeBeforePowerCal`. That way TX and CALIBRATE pressed in sequence cannot overwrite the real previous mode (USB etc.) with TUNE.
2. `await _radio.SetModeAsync("TUNE")`, `ModeText = "TUNE"`.
3. `await _radio.SetAutoTuneAsync(true)` (0xA6), same as AMP CAL.
4. Then the existing `SetCalibrationTuneAsync(true)` (0xAC).

Do not add a second opcode path. Do not send tune-power 100 unless you find QRP already does that; this brief does not ask for it.

### Stop and restore

On every 0xAC OFF, also drop 0xA6 and restore `_modeBeforePowerCal` (same guard as AMP: blank or TUNE → USB):

- `TogglePowerCalTxAsync` when turning TX off (the `turnOn == false` path, currently only 0xAC off at 4496).
- `TogglePowerCalCalibrateAsync` stop branch (4566-4598, 0xAC off at 4570). Restore mode after 0xAC OFF, even though the save-prompt still follows.
- **`ForceStopPowerCal`** (4684-4706). This is already called on disconnect (`Disconnect` 1932), close (`PrepareForCloseAsync` 2108), and AMP on (2683). Today it only sends 0xAC OFF. Add 0xA6 OFF and mode restore here so those exits are covered in one place.

### Leaving the QRP tab (required for the smoke test)

`MainTabs_OnSelectionChanged` (`Views/MainWindow.axaml.cs` 110-152) only handles leaving the frequency-cal tab. Leaving QRP while keyed does not unkey today. When the removed tab is `QrpCalTab`, call `ForceStopPowerCal("leave tab")` (after you have taught that function to restore the mode). Keep the existing Freq Cal logic. Do not build a second stop path.

## Version and build

Leave the version at **1.7.03** (csproj and `DEBIAN/control`). Do not bump it.

Rebuild the amd64 deb from the repo root:

```bash
pwsh -NoProfile -File Avalonia/build-mscc-avalonia-deb.ps1 -Arch amd64
```

Do not build arm64. Do not copy into `installers/rpi/`. Do not `sudo` — Build cannot install the package. Tell Stew to reinstall with:

```bash
sudo apt install --reinstall -y /home/stew/Documents/GitHub/mscc-station/Avalonia/mscc-avalonia_1.7.03_amd64.deb
```

## Smoke test (Stew), after the reinstall

Title bar still 1.7.03.

1. On the main tab pick **USB**, open QRP CAL and press **CALIBRATE** without re-selecting a band. You should get a **carrier** on the SA and **no mic audio**.
2. Stop, and the main tab is back on **USB**.
3. Same with **TX/TUNE** on QRP (carrier, not mic; stop returns USB).
4. Start CALIBRATE or TX, then **leave the QRP tab** while still keyed: TX must stop and the mode must return to USB.
5. **AMP CAL**, **main TUN**, and the **wheel** still work (image still moves live per notch). AGC is still unchanged.

## Out of scope

WPF (later, on Shack), servers, firmware, `rpi/`, `linux/` servers, `Solidus/`, `installers/rpi/`, AGC range, version bump, commit, push, GitHub Release. The Ron note `.mscc-coord/NOTE-FOR-RON-QRP-CAL-TUNE-MODE-2026-11-15.md` is already written; leave it untracked.

STOP. No commit, no push.


---

# Finished / commit (Stew said "finished", 2026-11-15)

Stew's preference: **Build commits on this host; Stew pushes.** Do the commit, then stop. **No push. No GitHub Release.** Avalonia 1.7.03 will ride the next GitHub Release with the firmware.

This is an **Avalonia-only** job. Scope the root `CLAUDE.md` "finished" checklist to what applies. Skip: Pi ms-sdr `version.h`, Windows/Pi/Debian installer builds, `installers/rpi/*.deb`, firmware guide, `rpi/CLAUDE.md` (Ron just edited it in `87e12aa`), WPF, servers.

## 1. Pull first

```bash
git pull --rebase --autostash
```

Tell Stew what came in (who, when, one line each).

As of the brief update (stew-HP already at `87e12aa` = `origin/main`): the only commit after the AGC question `62bf9d9f` is Ron's **`87e12aa`** (15 Nov 2026, 12:47:58 PM EST), "Notes: tooltip item on Ron's list; reply to Stew on AGC range and packaging". It adds `.mscc-coord/NOTE-FOR-STEW-AGC-AND-PACKAGING-REPLY-2026-11-15.md` and edits `rpi/CLAUDE.md` only. **It does not touch any Avalonia file this brief changed**, so the rebase should be clean. Ron did **not** fix the AGC slider himself; he asked Stew to do it (that work is **cmd-082**, not this commit). If a newer fetch brings Avalonia edits, stop and report the conflict.

Confirm `git status` still shows only the Part A+B Avalonia files plus this brief and the QRP TUNE Ron note (and any docs you add below). **Do not add cmd-082 files** if they already exist (`.mscc-coord/cmd-082-brief.md`, `.mscc-coord/NOTE-FOR-RON-QRP-AMP-CAL-FREQ-2026-11-15.md`).

## 2. Docs for the Avalonia 1.7.03 bump

Version numbers live in the Software Versions Addendum (`CLAUDE.md`). 1.7.02 did this: Addendum `.md` + `.pdf`, `installers/windows/CHANGELOG.md`, `installers/rpi/INSTALL.md`. Avalonia `.deb` files were **not** committed (`Avalonia/.gitignore` has `*.deb`; 1.7.02 did not add a deb). Do the same.

**Addendum** (`docs/manuals/MSCC-Software-Versions-Addendum.md`):

- Change Avalonia **1.7.02 → 1.7.03** in the programs table and in the Pi installer filename (`mscc-avalonia_1.7.03_arm64.deb`).
- Keep the release header at **R11-14-3**. There is no new GitHub Release yet.
- Add a short plain line that Avalonia 1.7.03 is built and tested on Ubuntu (slider wheel + QRP CAL TUNE mode) and will be attached at the next GitHub Release, with the firmware. Do not invent an arm64 deb that was not built on this host (cmd-081 built amd64 only).

Regenerate the PDF with:

```bash
python docs/manuals/make-addendum-pdf.py
```

That script needs the `markdown` module and **Microsoft Edge**, so it will likely fail on stew-HP. If it fails, **leave the PDF uncommitted** (do not commit a 1.7.02 PDF next to a 1.7.03 markdown). Report it; the PDF can be built on Shack before the next release. Still commit the updated `.md`.

**`installers/windows/CHANGELOG.md`**: add an Avalonia 1.7.03 entry dated 2026-11-15. This is **not** a new Windows `R11-…` installer. WPF and the Windows servers are unchanged. Suggested text (plain language, keep the file's style):

- Added: main-window sliders move one `SmallChange` per mouse-wheel notch, like WPF (RIT step 10, IQ sliders step 1). AGC FAST RELEASE was left at 0–100 in this build; Ron has since confirmed 0–2000 ms, step 10, for a follow-up.
- Fixed: QRP CAL **CALIBRATE** and **TX/TUNE** now set mode TUNE and send 0xA6 before keying 0xAC, and restore the previous mode on stop, leaving the tab, or disconnect. That stops phone audio on the tune carrier.

**`installers/rpi/INSTALL.md`**: it still has a versions table with `mscc-avalonia` **1.7.02** (line 16). Update that cell to **1.7.03**, as 1.7.02 did. Do not add version numbers to guides that don't already have them.

**Do not edit:** `installers/windows/INSTALL.md` (no Avalonia version), `installers/linux/CHANGELOG.md` (stale at 0.6.75; 1.7.02 did not touch it), `rpi/CLAUDE.md`, `mscc-ui/windows-work-tree/CLAUDE.md`, firmware guide.

## 3. One commit, no push

`git add` **only these paths** (skip any that you did not actually change):

```
Avalonia/packaging/mscc-avalonia/DEBIAN/control
Avalonia/src/MSCC.Avalonia/MSCC.Avalonia.csproj
Avalonia/src/MSCC.Avalonia/ViewModels/MainViewModel.cs
Avalonia/src/MSCC.Avalonia/Views/MainWindow.axaml
Avalonia/src/MSCC.Avalonia/Views/MainWindow.axaml.cs
.mscc-coord/cmd-081-brief.md
.mscc-coord/NOTE-FOR-RON-QRP-CAL-TUNE-MODE-2026-11-15.md
docs/manuals/MSCC-Software-Versions-Addendum.md
docs/manuals/MSCC-Software-Versions-Addendum.pdf
installers/windows/CHANGELOG.md
installers/rpi/INSTALL.md
```

**Never add:** `Avalonia/mscc-avalonia_1.7.03_amd64.deb` (gitignored), `Avalonia/publish/`, anything under `installers/linux/*.deb` or `installers/rpi/*.deb`, Visual Studio bookkeeping (`.tlog`, `.recipe`, `.iobj`, `.ipdb`, `.lastbuildstate`), cmd-082 files, or other people's dirty files.

Commit message (Stew / "With Ron" convention):

```
Avalonia 1.7.03: slider mouse wheel and QRP CAL TUNE mode

- Main-window sliders wheel one SmallChange per notch, like WPF (RIT=10, IQ=1).
  AGC FAST RELEASE left at 0-100 in this build. Ron has confirmed 0-2000 ms,
  step 10 (NOTE-FOR-STEW-AGC-AND-PACKAGING-REPLY-2026-11-15.md); Stew will
  do that in Avalonia next (cmd-082).
- QRP CAL CALIBRATE and TX/TUNE now set mode TUNE and 0xA6 before 0xAC, and
  restore the previous mode on stop, leave tab, or disconnect.
- Note for Ron: optional server safety net, Cal_tune_button also sending
  MODE_TUNE (.mscc-coord/NOTE-FOR-RON-QRP-CAL-TUNE-MODE-2026-11-15.md).
- Addendum / Windows CHANGELOG / Pi INSTALL: Avalonia 1.7.03. No GitHub
  Release now; this rides the next release with the firmware. WPF unchanged
  (later on Shack).

With Ron
- FYI: optional server safety net, Cal_tune_button also sending MODE_TUNE
  (NOTE-FOR-RON-QRP-CAL-TUNE-MODE-2026-11-15.md). Client 1.7.03 already
  sets TUNE before 0xAC.
- Thanks for the AGC 0-2000 / step 10 answer; Stew will do that in
  Avalonia in cmd-082.
- Packaging: thanks for the OK. We'll sort the Pi build-script / manifest
  question before Phase 2, as you asked.
```

Show `git status`, `git diff --stat` (staged), and the resulting hash. **Do not `git push`.** Do not `gh release`. Do not start cmd-082 until this commit is done and you have reported the hash.
