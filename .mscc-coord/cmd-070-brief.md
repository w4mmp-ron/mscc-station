# cmd-070: WPF S-meter digital dBm readout (click toggles S / dBm), S-unit rounding, spectrum dB CAL centre −83.6

Host: **windows-new-hp**. Repo: `C:\Users\n8vet\OneDrive\Documents\GitHub\mscc-station`.
**cmd-068 and cmd-069 are built, smoke-passed and NOT committed. Leave their changes in place**; cmd-070 goes on top in the same uncommitted tree.
**WPF client only** (`mscc-ui/windows-work-tree/mscc-mscc/mscc-new/src/MSCC.Wpf/...`). No server change, no server rebuild, no MSCC.Core change, no Avalonia.
**Do NOT touch `rpi/`, `Solidus/`, or `installers/rpi/`.** Stew: "Lets not go too crazy with the code changes." Keep each change small, with no refactors.
**STOP BEFORE COMMIT.** Show `git status` and `git diff --stat`, plus the full diff for each file.
Line numbers are from `6a813c5` **plus the uncommitted cmd-068/069 edits** (2026-10-11). Check them before you edit.

## Background (read-only findings)
- **Where the dBm comes from:** ms-sdr sends the S-meter as a whole-dB int (opcode 0xD4).
  - `MSCC.Core/Services/UdpRadioService.cs:1679-1686` raises `SmeterReported(dbm)`.
  - `ViewModels/MainViewModel.cs:5831-5835` turns it into S units: `SMeter = Db_to_Smeter(dbm)`. The dBm value itself is then thrown away.
- **S-unit conversion:** `Db_to_Smeter` (`MainViewModel.cs:418-456`): S9 = −73 dBm, 6 dB per S unit below S9, 10 dB steps above (capped at 12 = S9+30), 0 at −130 and below.
  - Below S9 it uses `9 + (dbm + 73) / 6`. C# integer division truncates toward zero, so −74…−78 all show S9, and the S8 band is −79…−84 (Ron's note: "S9 for −78 to −73").
  - Above S9 it already rounds to the nearest step (`(db_over_s9 + 5) / 10`).
- **Readout:** the meter face is `Controls/AnalogSMeterControl.xaml(.cs)`, bound in `MainWindow.xaml:494-504` (`Value="{Binding SMeter}"`).
  - The digital readout is `ReadingBorder` / `ReadingText` (`AnalogSMeterControl.xaml`), filled by `UpdateDigitalReadout` (`.xaml.cs:852-901`) from `FormatSmeterReading` (`:914-919`: S0…S9, S9+10…).
  - The readout is already clickable: `WireReadingClick` / `OnReadingClick` (`:214-233`), but today the click is only used for the red SWR-fault RESET (`PowerMode && Fault`).
- **TX:** `PowerMode` (= `ShowExternalSwrFace`) switches the face to FWD/SWR, and the readout then shows SWR or RESET. That path is untouched below.
- **Settings:** the meter's HOLD / Peak checkboxes are kept in `MSCC_Client.ini` (`SpectrumWaterfallSettings.cs` `SmeterHold`/`SmeterPeak` 462-464, load 825-826, save 1124-1125; VM `OnSmeterHoldChanged` 127-131; startup sync `MainWindow.xaml.cs:95-96`). The new toggle copies that pattern.
- **Spectrum centre:** Stew's cmd-069 TG step test on NEW-HP put the dB CAL trim at **−8.6** at −70 dBm, with all other steps within 2 dB. So the effective absolute offset is −75 − 8.6 = **−83.6**.
  This is the first absolute measurement (TG + attenuator). Ron's −75 estimate came from the S-meter needle, which can't resolve better than about 6 dB. Part 3 makes −83.6 the default centre, so trim 0 is right out of the box.

## Part 1. S-meter digital dBm readout (click toggles S ↔ dBm)

### 1a. `Controls/AnalogSMeterControl.xaml.cs`: two new dependency properties
After `SwrThresholdProperty` (ends line 82), add:
```csharp
    /// <summary>cmd-070: last S-meter report in whole dBm (from ms-sdr 0xD4).</summary>
    public static readonly DependencyProperty DbmProperty =
        DependencyProperty.Register(
            nameof(Dbm),
            typeof(int),
            typeof(AnalogSMeterControl),
            new PropertyMetadata(-130, OnReadoutModeChanged));

    /// <summary>cmd-070: true = digital readout shows dBm, false = S units. Click toggles (RX face only).</summary>
    public static readonly DependencyProperty ShowDbmProperty =
        DependencyProperty.Register(
            nameof(ShowDbm),
            typeof(bool),
            typeof(AnalogSMeterControl),
            new FrameworkPropertyMetadata(false,
                FrameworkPropertyMetadataOptions.BindsTwoWayByDefault, OnReadoutModeChanged));
```
After the `SwrThreshold` CLR property (about line 136), add:
```csharp
    public int Dbm
    {
        get => (int)GetValue(DbmProperty);
        set => SetValue(DbmProperty, value);
    }

    public bool ShowDbm
    {
        get => (bool)GetValue(ShowDbmProperty);
        set => SetValue(ShowDbmProperty, value);
    }
```
Next to `OnPowerReadingChanged` (about line 317), add:
```csharp
    private static void OnReadoutModeChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
    {
        if (d is AnalogSMeterControl c && !c.PowerMode)
            c.UpdateNeedles();
    }
```
(`UpdateNeedles` ends with `UpdateDigitalReadout(main)`; it already returns early before the face is built.)

### 1b. Click toggles on the RX face; the RESET click is unchanged
Replace `OnReadingClick` (lines 228-233):
```csharp
    private void OnReadingClick(object sender, MouseButtonEventArgs e)
    {
        if (!PowerMode || !Fault) return;
        ResetRequested?.Invoke(this, new RoutedEventArgs());
        e.Handled = true;
    }
```
with:
```csharp
    private void OnReadingClick(object sender, MouseButtonEventArgs e)
    {
        if (!PowerMode)
        {
            // cmd-070: RX face — toggle S units / dBm (persisted via the ShowDbm binding).
            ShowDbm = !ShowDbm;
            e.Handled = true;
            return;
        }
        if (!Fault) return;
        ResetRequested?.Invoke(this, new RoutedEventArgs());
        e.Handled = true;
    }
```
(Both `ReadingBorder` and `ReadingText` are wired to this handler. With `e.Handled = true` on the first one, the toggle fires once per click; check this in smoke step 1.)

### 1c. `UpdateDigitalReadout`: RX branch (lines 856-868)
Replace:
```csharp
            ReadingText.Text = FormatSmeterReading((int)Math.Round(main));
            ReadingText.Foreground = ReadingGreen;
            ReadingText.Cursor = Cursors.Arrow;
            if (ReadingBorder != null)
            {
                ReadingBorder.Background = ReadingNormalBg;
                ReadingBorder.BorderBrush = ReadingNormalBorder;
                ReadingBorder.Cursor = Cursors.Arrow;
                ReadingBorder.ToolTip = "S-meter reading";
            }
            return;
```
with:
```csharp
            // cmd-070: dBm = raw last report (whole dB); S units follow the needle as before.
            ReadingText.Text = ShowDbm ? $"{Dbm} dBm" : FormatSmeterReading((int)Math.Round(main));
            ReadingText.Foreground = ReadingGreen;
            ReadingText.Cursor = Cursors.Hand;
            if (ReadingBorder != null)
            {
                ReadingBorder.Background = ReadingNormalBg;
                ReadingBorder.BorderBrush = ReadingNormalBorder;
                ReadingBorder.Cursor = Cursors.Hand;
                ReadingBorder.ToolTip = ShowDbm
                    ? "S-meter in dBm (click for S units)"
                    : "S-meter reading (click for dBm)";
            }
            return;
```
The power/SWR/RESET branches below stay as they are. The readout has `MinWidth="56"`, which fits "-110 dBm" in Consolas 11. If it clips, raise `MinWidth` to 64 in `AnalogSMeterControl.xaml` (`ReadingBorder`); no other XAML change.

### 1d. `ViewModels/MainViewModel.cs`: expose dBm and the toggle
1. After `private int _sMeter;` (lines 51-52), add:
   ```csharp

    /// <summary>cmd-070: last S-meter report in whole dBm (shown when SmeterShowDbm).</summary>
    [ObservableProperty]
    private int _sMeterDbm = -130;

    /// <summary>cmd-070: S-meter digital readout shows dBm instead of S units. Persisted SMETER_DBM.</summary>
    [ObservableProperty]
    private bool _smeterShowDbm;
   ```
2. Next to `OnSmeterHoldChanged` (lines 127-131), add:
   ```csharp

    partial void OnSmeterShowDbmChanged(bool value)
    {
        SpectrumWaterfallSettings.SmeterShowDbm = value;
        SpectrumWaterfallSettings.Save();
    }
   ```
3. In the `SmeterReported` handler (lines 5831-5835), add one line before `SMeter = Db_to_Smeter(dbm);`:
   ```csharp
            SMeterDbm = dbm;
   ```

### 1e. `MainWindow.xaml` (lines 494-504): bind the two properties
Add to the `<controls:AnalogSMeterControl ...>` element (for example after `PeakHoldSeconds="2"`):
```xml
                                                  Dbm="{Binding SMeterDbm}"
                                                  ShowDbm="{Binding SmeterShowDbm, Mode=TwoWay}"
```

### 1f. Persist: `SpectrumWaterfallSettings.cs` + startup sync
1. After `public static bool SmeterPeak { get; set; } = false;` (line 464):
   ```csharp
    /// <summary>cmd-070: S-meter digital readout in dBm (true) or S units (false). Default S units.</summary>
    public static bool SmeterShowDbm { get; set; } = false;
   ```
2. Load, after the `SMETER_PEAK` line (826):
   `if (LineMatchesKey(line, "SMETER_DBM")) SmeterShowDbm = ParseIniBool(line, SmeterShowDbm);`
3. Save, after the `SMETER_PEAK` line (1125):
   `UpdateOrAdd(lines, "SMETER_DBM", SmeterShowDbm ? "1" : "0");`
4. `MainWindow.xaml.cs`, after `vm.PeakNeedleOn = SpectrumWaterfallSettings.SmeterPeak;` (line 96):
   `vm.SmeterShowDbm = SpectrumWaterfallSettings.SmeterShowDbm;`

TX: nothing changes. With the external SWR face (`PowerMode`), the readout shows SWR / RESET as today and a click is still only RESET. Without it, the meter keeps doing what it does today; the readout shows the same reports, in dBm if toggled.

## Part 2 (optional, separate hunk). S units rounded to the nearest unit
`MainViewModel.cs:429`, in `Db_to_Smeter`, replace
```csharp
                smeter_value = 9 + (dbm + 73) / 6;
```
with
```csharp
                // cmd-070: nearest S unit (was truncation toward zero: −74…−78 all read S9).
                smeter_value = 9 + (int)Math.Round((dbm + 73) / 6.0, MidpointRounding.AwayFromZero);
```
Result: S9 = −76…−73, S8 = −82…−77 (each S unit ±3 dB around its nominal level). The needle moves accordingly. The over-S9 rounding and the 0 / 1 / 12 limits stay as they are.
Skip this hunk if Stew prefers the meter exactly as it was; Part 1 doesn't depend on it.

## Part 3. Spectrum dB CAL default centre −75 → −83.6 (Stew's TG measurement)
1. `Controls/SpectrumColorSettings.cs:61`: `public const float SpectrumDbCalCenter = -83.6f;`
   - Summary at line 60: `/// <summary>Centre cal (slider "0") for the true-dB scale: TG step test on NEW-HP 2026-10-10 (−70 dBm reads −70 at −83.6).</summary>`
   - Comment at 55 (−75 → −83.6), and 66-67 (`// −103.6`, `// −63.6`).
2. View defaults move with the floor (about 8.6 dB lower, so near −140): **GRID MIN −145 → −155**. All of them:
   - `SpectrumColorSettings.cs:87`, and the fallback at line 96: `minDb = maxDb - 135f; // e.g. −20 → −155`.
   - `SpectrumWaterfallSettings.cs:37`, `ShipHfGridMin`/`ShipLfGridMin` (152, 160), and the load repair (the two `-145f` at about 710/716).
   - `PanadapterControlsWindow.xaml.cs` `GridRangeReset_Click` (`-145f` → `-155f`).
   - The GridMin slider `Value="-145"` and `GridMinValueText` (`PanadapterControlsWindow.xaml:126,129`) and the tooltips there (−145 → −155, grass ~−131 → ~−140). The slider range −160…−90 already allows it.
   - **Waterfall defaults move by about −9**:
     - `WaterfallHighDefault`/`LowDefault` and the two backing fields (`SpectrumColorSettings.cs:202-206`) −30/−140 → **−40/−150**.
     - `ShipHfHighDb`/`ShipHfLowDb` −30/−105 → **−40/−115**.
     - `ShipLfHighDb`/`ShipLfLowDb` −25/−95 → **−35/−105**.
     - XAML slider values/text and the reset tooltip/label text to match.
   - Comments in `SpectrumWaterfallSettings.cs:29-37` and `MainViewModel.cs` (`EnrichSpectrumUpdate` comment) to match.
3. **Reset logic: no change.** dB CAL is stored as an **absolute** offset (`SPECTRUM_DB_OFFSET`, `_HF`, `_LF`).
   - Users whose ini already has `SPECTRUM_DB_LAW=20` (only cmd-069 test machines; cmd-069 was never released) keep their absolute offset and view values. On NEW-HP the saved −83.6 now shows as trim 0.
   - Everyone else gets the one-time cmd-069 reset to the new defaults. −75 is still inside the new trim range (−103.6…−63.6), so no clamping surprises.
4. Avalonia note for later: its centre is `Controls/SpectrumDisplaySettings.cs:79` (`DbCalCenterAbsolute`), also to become −83.6 in its follow-up brief.

## Part 4. Version, CHANGELOG, build
1. Don't hand-edit `ClientVersion.txt` (10.9.1). The first build on 2026-10-11 gives **10.11.0**; use the number the build prints.
2. `installers/windows/CHANGELOG.md`: **combine**. R10.9.1 was never packaged, so rename the existing heading ``## R10.9.1 - 2026-10-09  (`mscc-net9-R10-9-1-install.exe`, title bar 10.9.1)`` to
   ``## R10.11.0 - 2026-10-11  (`mscc-net9-R10-11-0-install.exe`, title bar 10.11.0)``. Use the final number if it differs.
   Keep all cmd-068/069 bullets; in the cmd-069 reset bullet change "centre −75, GRID MIN −145, waterfall −140…−30" to "centre −83.6, GRID MIN −155, waterfall −150…−40" and "about −130" to "about −140". Add:
   - Under **Added**: `- S-meter: click the digital readout to switch between S units (S9+10) and dBm (-63 dBm). The choice is remembered.`
   - Under **Changed**: `- S-meter S units now round to the nearest unit (S9 is -76 to -73 dBm; it used to show S9 down to -78).` (only if Part 2 is done)
   - Under **Changed**: `- Spectrum dB CAL centre set from a signal-generator step test (-70 dBm reads -70 with the trim at 0).`
3. Build: `dotnet build MSCC.sln -c Release` from `mscc-ui/windows-work-tree/mscc-mscc/mscc-new` (version bump + copy to `C:\mscc-net9`). **No server rebuild**: recv 3.145, trans 3.146 and ms-sdr 3.182 stay as they are in `C:\mscc-net9`.
4. Check that `C:\mscc-net9\MSCC.Wpf.exe` ProductVersion is the new M.D.I with today's time. Update `.mscc-coord/status/windows-new-hp.md` (cmd-070 row, client M.D.I, not committed).

## Part 5. Smoke (NEW-HP, TG + step attenuator; Stew)
1. **Toggle:** click the S-meter digital readout. It switches S units ↔ "-NN dBm" once per click, and the tooltip changes. The needle and the HOLD/Peak checkboxes behave as before.
2. **dBm vs TG:** CW carrier inside the RX filter at −30, −50, −70, −90 and −110 dBm (TG level + attenuator). Record the dBm readout at each.
   - Expect within a couple of dB. This measures the S-meter's own accuracy (server side, peak-based), which cmd-070 doesn't change.
   - If it is off by a constant, or squeezed, write it down for Ron (his open item: "spectrum centre once the S meter shows dBm").
   - Also note at what level the readout stops following (noise).
3. **Persist:** leave it in dBm, close MSCC and reopen. It is still dBm, and `MSCC_Client.ini` has `SMETER_DBM=1;`. Toggle back to S and restart: S units.
4. **TX unaffected:** key TUNE/CW. The meter behaves as before (FWD/SWR face if the external meter is on; RESET on a latched fault still works with one click). Back on RX, the readout returns to the chosen mode.
5. **S rounding (if Part 2 done):** −76 dBm shows S9, −77 shows S8, −73 shows S9, −63 shows S9+10.
6. **Spectrum with the new centre:**
   - S/W window: on NEW-HP the dB CAL trim now reads **0.0** (the saved −83.6 kept).
   - The TG at −30/−50/−70/−90/−110 still reads within ±2 dB on the spectrum.
   - On a machine with an old ini (no `SPECTRUM_DB_LAW`), the first start resets to trim 0, GRID MIN −155 and waterfall −115/−40 (HF), and the TG again reads within ±2 dB with trim 0.
7. cmd-068/069 still good: no peak drops while tuning at 1x; the resolution selector is gone; the waterfall looks normal; the title shows the new version.

## What NOT to touch
`rpi/`, `Solidus/`, `installers/rpi/`, all servers (`SDRcore-recv`, `SDRcore-trans`, `ms-sdr-MKII`, Linux), MSCC.Core, the 0xD4 protocol, the S-meter server math, Avalonia, and firmware.
Ron's newer Pi recv changes (2026-10-10: running-average smoothing in `panadapter.c`, LO-point levelling and DC_BLOCK_A 0.9999 in `dsputils.c`, `.mscc-coord/NOTE-FOR-STEW-SPECTRUM-SMOOTHING-2026-10-10.md`) are a **separate** later port; don't fold them in here.

## Report
Full diffs, client M.D.I, the `C:\mscc-net9\MSCC.Wpf.exe` version and time, and smoke results 1-7 (TG table: set level / S-meter dBm / spectrum peak).

STOP. No commit, no push.
