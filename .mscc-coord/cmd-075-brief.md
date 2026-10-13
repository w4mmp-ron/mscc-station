# cmd-075: WPF S/W window: SMOOTHING (was REFRESH), waterfall Falling by default ("Climbing" option), BASELINE + waterfall levels per band

Host: **windows-new-hp**. Repo: `C:\Users\n8vet\OneDrive\Documents\GitHub\mscc-station`.
**WPF client only** (`MSCC.Wpf`, plus one small additive method in `MSCC.Core`, see A1). Avalonia gets the same three changes in a later brief.
Background: `.mscc-coord/NOTE-FOR-STEW-SMOOTHING-CONTROL-2026-10-12.md` and `.mscc-coord/NOTE-FOR-STEW-BASELINE-PER-BAND-2026-10-12.md` (Ron).

**PRECONDITIONS. Check them first; if either is not met, STOP and report:**
1. **Stew has packaged `mscc-net9-R10-13-0-install.exe` from `C:\mscc-net9`.** This build overwrites `C:\mscc-net9\MSCC.Wpf.exe`.
2. **cmd-074 is committed:** `git status` is clean apart from `.mscc-coord/cmd-075-brief.md`. cmd-075 edits `MainViewModel.cs`, which cmd-074 also changed, so nothing may be uncommitted from cmd-074.

Then `git pull --ff-only`.

**Do NOT edit `rpi/`, `Solidus/`, or `installers/rpi/`** (read only).
Don't touch: any server (recv/trans/ms-sdr: no server change is needed), Avalonia, the spectrum math (true dB, max-per-pixel, dB CAL/grid), the protocol other than *sending* the existing 0xDA.
Stew: "Lets not go too crazy with the code changes." Small changes, no refactors.
**STOP BEFORE COMMIT.** Show `git status`, `git diff --stat`, and the full diff of each source file.
Line numbers are from `0dff8fe` plus cmd-074; check them before you edit.

---

## Part A. REFRESH → SMOOTHING (server smoothing 0xDA), no more client frame dropping

**Today:**
- **REFRESH:** `PanadapterControlsWindow.xaml:134-143` (`RefreshList` 1/2/3/4/5/8/10, handler `OnRefreshSelectionChanged` `.xaml.cs:542-553`, selected at `.xaml.cs:58`) sets `SpectrumWaterfallSettings.SpectrumRefresh` (`SpectrumWaterfallSettings.cs:347`, default 4, key `SPECTRUM_REFRESH` load 805 / save 1105).
  - `MainViewModel.cs:2096-2104` then draws 1 frame in N and drops the rest (field `_spectrumFrameCounter`, line 49). Nothing goes to the host.
- **Server smoothing already exists**, on every platform:
  - opcode `CMD_GET_SET_PANADAPTER_SMOOTHING` = **0xDA**, data = value 0..100.
  - ms-sdr stores it as `PANADAPTER_AVERAGE`, saves it, and forwards it to recv (Windows `user_controls.c:1381-1386`).
  - recv applies it on the next frame: 0 = 2-frame average, N = time constant N × 25 ms.
- **The host does not report the value back.** No ms-sdr (Windows, Ubuntu or Pi) sends 0xDA or `PANADAPTER_AVERAGE` to the client. `User_Controls_Send_To_Gui` doesn't include it, and the `PANADAPTER_AVERAGE=%d;` strings are only the ini file and a commented-out log line. Showing the host's value at connect would need a server change, which is out of scope.
- **Judgment call:** the client keeps the value (client ini `SPECTRUM_SMOOTHING`, **default 3**) and **pushes it to the host at Start and on every change**. The host stores it as before. So the S/W window always shows what the host is using.

### A1. `MSCC.Core` (additive only; Avalonia compiles unchanged, `UdpRadioService` is the only `IRadioService`)
1. `Protocol/Opcodes.cs`, after line 149 (`CMD_GET_SET_PANADAPTER_REFRESH = 0x5F;`):
   ```csharp

    /// <summary>Server spectrum smoothing (ms-sdr PANADAPTER_AVERAGE → recv). 0 = 2-frame average; N = running average, time constant N × 25 ms.</summary>
    public const byte CMD_GET_SET_PANADAPTER_SMOOTHING = 0xDA;
   ```
2. `Services/IRadioService.cs`, after line 162 (`SetPanResolutionAsync`):
   ```csharp
    /// <summary>cmd-075: server spectrum smoothing 0..100 (0xDA; ms-sdr stores PANADAPTER_AVERAGE).</summary>
    Task SetPanadapterSmoothingAsync(int value, CancellationToken cancellationToken = default);
   ```
3. `Services/UdpRadioService.cs`, after the end of `SetPanResolutionAsync` (about line 242):
   ```csharp

    public async Task SetPanadapterSmoothingAsync(int value, CancellationToken cancellationToken = default)
    {
        if (!_started) return;
        short v = (short)Math.Clamp(value, 0, 100);
        await _transport.SendAsync(Opcodes.CMD_GET_SET_PANADAPTER_SMOOTHING, v, cancellationToken);
        DebugMonitor.MonitorTextBoxText($" Send CMD_GET_SET_PANADAPTER_SMOOTHING (0xDA): {v}");
    }
   ```

### A2. `SpectrumWaterfallSettings.cs`
- Line 347: replace `public static int SpectrumRefresh { get; set; } = 4;` with
  ```csharp
    /// <summary>cmd-075: server spectrum smoothing 0..30 (0xDA). Default 3 (Stew). Replaces client REFRESH (frame dropping).</summary>
    public static int SpectrumSmoothing { get; set; } = 3;
  ```
- Load (line 805): replace the `SPECTRUM_REFRESH` line with
  `if (LineMatchesKey(line, "SPECTRUM_SMOOTHING")) SpectrumSmoothing = Math.Clamp(ParseIniInt(line, SpectrumSmoothing), 0, 100);`
  An old `SPECTRUM_REFRESH` key is ignored; leave it in the file.
- Save (line 1105): replace with `UpdateOrAdd(lines, "SPECTRUM_SMOOTHING", SpectrumSmoothing.ToString());`
- Fix any other reference to `SpectrumRefresh`; there must be none left.

### A3. `PanadapterControlsWindow.xaml` (lines 134-143)
- Label `REFRESH` → `SMOOTHING`, with the tooltip `Server spectrum smoothing (time constant = value × 25 ms; 0 = light 2-frame average). Applies to spectrum and waterfall.`
- Rename the list to `x:Name="SmoothingList"` and the handler to `SelectionChanged="OnSmoothingSelectionChanged"`.
- Items: `0, 1, 2, 3, 4, 6, 8, 10, 15, 20, 30`. The width/height stay as they are; it scrolls.

### A4. `PanadapterControlsWindow.xaml.cs`
- Line 58: `SelectItemByContent(SmoothingList, SpectrumWaterfallSettings.SpectrumSmoothing.ToString());`. Check `SuppressListSelectionHandlers` and any other `RefreshList` reference, and rename them too.
- Replace `OnRefreshSelectionChanged` (542-553) with:
  ```csharp
    private void OnSmoothingSelectionChanged(object sender, SelectionChangedEventArgs e)
    {
        if (sender is ListBox lb && lb.SelectedItem is ListBoxItem item &&
            int.TryParse(item.Content?.ToString(), out int v) && v >= 0)
        {
            SpectrumWaterfallSettings.SpectrumSmoothing = v;
            SpectrumWaterfallSettings.Save();
            if (Application.Current?.MainWindow is MainWindow mw)
                mw.ViewModel?.ApplySpectrumSmoothing("S/W");
        }
    }
  ```
  Use the same `MainWindow mw` access pattern as `ApplyPanResolution("S/W")` at line 581.

### A5. `ViewModels/MainViewModel.cs`
1. **Frame dropping off:** delete lines 2096-2104 (the `// Throttle display updates…` comment, `int div …`, `_spectrumFrameCounter = …` and the `if (… != 0) { return; }` block). Delete the now unused field `private int _spectrumFrameCounter;` (line 49).
2. **Push at Start:** after `ApplyPanResolution("start");` (line 5499), add `ApplySpectrumSmoothing("start");`.
3. Next to `ApplyPanResolution` (line 5521), add:
   ```csharp
    /// <summary>cmd-075: send the S/W SMOOTHING value (0xDA) to the host.</summary>
    public void ApplySpectrumSmoothing(string reason = "settings")
    {
        int v = SpectrumWaterfallSettings.SpectrumSmoothing;
        _ = _radioService.SetPanadapterSmoothingAsync(v);
        MonitorTextBoxText($" Spectrum smoothing {v} ({reason})");
    }
   ```

**Effect to expect:** the spectrum and waterfall now run at the full 15.6 frames/rows per second. With the old REFRESH 3-4 the **waterfall scrolled 3-4× slower**, so it will now move faster. That's a deliberate consequence, called out in smoke step 3; if Stew wants a slower waterfall, that's a follow-up (a waterfall SPEED control).

---

## Part B. Waterfall direction: default **Falling** (newest line at top, moving down); option labelled **Climbing**

**Today:** `WaterfallDirectionNormal` = true means "oldest at top, newest at bottom", i.e. **climbing**. It's the default (`SpectrumWaterfallSettings.cs:57`, HF bank 171, LF bank 183; `SpectrumColorSettings.cs:300`). The renderer is `SpectrumDisplayControl.xaml.cs:851-858`, unchanged. The checkbox (`PanadapterControlsWindow.xaml:286-291`) toggles its text between "Direction Normal" and "Direction Reversed" (`.xaml.cs:252-258`). Checked = climbing.

**Change (meaning of the bool unchanged; only the default, the label and a one-time migration):**
1. **Defaults → false (falling):** `SpectrumWaterfallSettings.cs:57`, `:171`, `:183`, and `SpectrumColorSettings.cs:300`. Update the doc comments at `SpectrumColorSettings.cs:296-299`: true = climbing (newest at bottom), false = falling (newest at top, default).
2. **Label:** in the XAML, `Content="Climbing"` with the tooltip `Checked = Climbing (new lines appear at the bottom and move up). Unchecked = Falling (default: new lines at the top, moving down).` The `.xaml.cs:252-258` `UpdateWaterfallDirectionCheckboxText` body becomes `WaterfallDirectionCheckBox.Content = "Climbing";`. Keep the method and its calls; that's the smallest diff.
3. **One-time migration** (`SpectrumWaterfallSettings.cs` load, next to the cmd-069 `SPECTRUM_DB_LAW` block about line 960). `WATERFALL_DIRECTION_NORMAL` was always saved, mostly as the old default `1`, so a saved `1` can't be told apart from a deliberate choice. Move everyone to the new default once:
   ```csharp
                // cmd-075: waterfall default is now Falling. The old default (Normal = climbing) was
                // always saved, so move everyone to Falling once; "Climbing" can be ticked again.
                if (!fileLines.Any(l => LineMatchesKey(l, "WATERFALL_FALLING_DEFAULT")))
                {
                    WaterfallDirectionNormal = false;
                    WaterfallHfDirectionNormal = false;
                    WaterfallLfDirectionNormal = false;
                }
   ```
   In `Save()`, add `UpdateOrAdd(lines, "WATERFALL_FALLING_DEFAULT", "1");` next to `SPECTRUM_DB_LAW`.
   Users who already had falling (`=0`, like Stew's NEW-HP ini) see no change.

---

## Part C. BASELINE + waterfall levels remembered per band

**Today:** BASELINE and the waterfall HIGH / LOW / GAIN / ZERO sliders are kept per radio family only, in the HF bank (160 m-10 m, GEN) or the LF bank (2200/630 m): `SpectrumWaterfallSettings.cs` 165-187. They're captured by `SaveLiveWaterfallAndActiveBank` (340) on every S/W slider change and re-loaded by `ApplyWaterfallBankForActiveBand` (266) on every band change (`MainWindow.xaml.cs:111-118` → `:427-435`). Per-band freq/mode/filters live in `MSCC_LastUsed.ini` as `{BAND}_FREQ` etc. (`SaveLastUsedForBand` 1273, `GetValidBandPrefixes`, `LineMatchesKey`).

**Design (judgment calls):**
- **Per band:** BASELINE, WATERFALL HIGH, LOW, GAIN and ZERO. Not the grid, dB CAL, palette or direction: those stay per family. Ron's note asks for BASELINE only; Stew adds the waterfall levels.
- **Stored in the VFO A `MSCC_LastUsed.ini`** as `{BAND}_SW_BASELINE`, `{BAND}_WF_HIGH`, `{BAND}_WF_LOW`, `{BAND}_WF_GAIN`, `{BAND}_WF_ZERO`. That's always the VFO A file: it's a display setting, not a VFO setting. These keys start with a valid band prefix, so `SaveLastUsedForBand`'s cleanup keeps them.
- **A band with no saved value** keeps whatever the HF/LF bank holds. That's the current behaviour, and in practice it means the values from the band last adjusted in that family.

### C1. `SpectrumWaterfallSettings.cs`
1. Near `ApplyWaterfallBankForActiveBand` (266), add:
   ```csharp
    /// <summary>cmd-075: band whose S/W levels are live (normalized key, "" = unknown).</summary>
    private static string _swLevelsBand = "";

    /// <summary>cmd-075: save BASELINE + waterfall HIGH/LOW/GAIN/ZERO for one band (MSCC_LastUsed.ini, VFO A file).</summary>
    private static void SaveSwLevelsForBand(string band)
    {
        string b = NormalizeBandKey(band);
        if (string.IsNullOrEmpty(b) || !GetValidBandPrefixes().Contains(b)) return;
        string path = GetLastUsedPath(forVfoB: false);
        if (string.IsNullOrEmpty(path)) return;
        var lines = File.Exists(path) ? File.ReadAllLines(path).ToList() : new List<string>();
        var ci = System.Globalization.CultureInfo.InvariantCulture;
        UpdateOrAdd(lines, $"{b}_SW_BASELINE", SpectrumBaseline.ToString(ci));
        UpdateOrAdd(lines, $"{b}_WF_HIGH", WaterfallHighDb.ToString("0.0", ci));
        UpdateOrAdd(lines, $"{b}_WF_LOW", WaterfallLowDb.ToString("0.0", ci));
        UpdateOrAdd(lines, $"{b}_WF_GAIN", WaterfallGain.ToString(ci));
        UpdateOrAdd(lines, $"{b}_WF_ZERO", WaterfallZero.ToString(ci));
        EnsureLastUsedFileExists(path);
        File.WriteAllLines(path, lines);
    }

    /// <summary>cmd-075: load saved per-band levels into live fields. False = nothing saved for that band.</summary>
    private static bool LoadSwLevelsForBand(string band)
    {
        string b = NormalizeBandKey(band);
        string path = GetLastUsedPath(forVfoB: false);
        if (string.IsNullOrEmpty(b) || string.IsNullOrEmpty(path) || !File.Exists(path)) return false;
        bool any = false;
        foreach (string line in File.ReadAllLines(path))
        {
            if (LineMatchesKey(line, $"{b}_SW_BASELINE")) { SpectrumBaseline = ParseIniInt(line, SpectrumBaseline); any = true; }
            else if (LineMatchesKey(line, $"{b}_WF_HIGH")) { WaterfallHighDb = ParseIniFloat(line, WaterfallHighDb); any = true; }
            else if (LineMatchesKey(line, $"{b}_WF_LOW")) { WaterfallLowDb = ParseIniFloat(line, WaterfallLowDb); any = true; }
            else if (LineMatchesKey(line, $"{b}_WF_GAIN")) { WaterfallGain = ParseIniInt(line, WaterfallGain); any = true; }
            else if (LineMatchesKey(line, $"{b}_WF_ZERO")) { WaterfallZero = ParseIniInt(line, WaterfallZero); any = true; }
        }
        return any;
    }
   ```
   (Use the existing `ParseIniInt` / `ParseIniFloat` / `UpdateOrAdd` helpers. If `ParseIniFloat` is culture-sensitive, write the floats the way the existing `WATERFALL_HF_HIGH` save does instead.)
2. `ApplyWaterfallBankForActiveBand` (266-269) becomes:
   ```csharp
    public static void ApplyWaterfallBankForActiveBand(string? band)
    {
        SwitchRadioModelWaterfall(nowGeminus: IsLfWaterfallBand(band));
        // cmd-075: per-band BASELINE + waterfall levels on top of the HF/LF bank.
        _swLevelsBand = NormalizeBandKey(band ?? "");
        if (LoadSwLevelsForBand(_swLevelsBand))
            ApplyLiveWaterfallToRenderer();
    }
   ```
3. `SaveLiveWaterfallAndActiveBank` (340-344): before `Save();` add
   ```csharp
        if (_swLevelsBand.Length > 0)
            SaveSwLevelsForBand(_swLevelsBand);
   ```
   (This runs on every S/W slider change. Grid / dB CAL changes also re-save that band's five levels, which is harmless: same values.)

### C2. Keep an open S/W window in step on band change
The S/W window is non-modal (`MainViewModel.cs:4141`, field `_specWaterfallWindow` at 1394). On a band change its sliders must show the new band's values without firing their save handlers.
1. `PanadapterControlsWindow.xaml.cs`, add:
   ```csharp
    /// <summary>cmd-075: re-read BASELINE + waterfall level sliders after a band change (no save).</summary>
    public void SyncLevelSlidersFromSettings()
    {
        if (BaselineSlider != null) BaselineSlider.ValueChanged -= OnBaselineChanged;
        if (WaterfallHighSlider != null) WaterfallHighSlider.ValueChanged -= OnWaterfallHighChanged;
        if (WaterfallLowSlider != null) WaterfallLowSlider.ValueChanged -= OnWaterfallLowChanged;
        if (WaterfallGainSlider != null) WaterfallGainSlider.ValueChanged -= OnWaterfallGainChanged;
        if (WaterfallZeroSlider != null) WaterfallZeroSlider.ValueChanged -= OnWaterfallZeroChanged;
        try
        {
            if (BaselineSlider != null) BaselineSlider.Value = SpectrumWaterfallSettings.SpectrumBaseline;
            if (WaterfallHighSlider != null) WaterfallHighSlider.Value = SpectrumColorSettings.WaterfallHighDb;
            if (WaterfallLowSlider != null) WaterfallLowSlider.Value = SpectrumColorSettings.WaterfallLowDb;
            UpdateWaterfallRangeValueText();
            if (WaterfallGainSlider != null) WaterfallGainSlider.Value = SpectrumColorSettings.WaterfallGain;
            if (WaterfallGainValueText != null) WaterfallGainValueText.Text = SpectrumColorSettings.WaterfallGain.ToString();
            if (WaterfallZeroSlider != null) WaterfallZeroSlider.Value = SpectrumColorSettings.WaterfallZero;
            if (WaterfallZeroValueText != null) WaterfallZeroValueText.Text = SpectrumColorSettings.WaterfallZero.ToString();
        }
        finally
        {
            AttachSliderHandlers();
        }
    }
   ```
   If the BASELINE value text has its own update method, call it as well.
2. `MainViewModel.cs`, add `public void SyncSpecWaterfallWindow() => _specWaterfallWindow?.SyncLevelSlidersFromSettings();`
3. `MainWindow.xaml.cs` `ApplyWaterfallBankForActiveBand()` (427-435): after `SpectrumWaterfallSettings.ApplyWaterfallBankForActiveBand(band);` add `ViewModel.SyncSpecWaterfallWindow();`.

---

## Part D. Build, version, CHANGELOG
1. Stop MSCC. **WPF only:** `dotnet build MSCC.sln -c Release` from `mscc-ui/windows-work-tree/mscc-mscc/mscc-new` (auto-bumps and copies to `C:\mscc-net9`). No server rebuild.
2. **Version:** date-based. 10.13.0 has shipped as R10-13-0, so today's next build gives **10.13.1**. If it's built on another day, it's that day's M.D.0. Use what the build prints; don't hand-edit `ClientVersion.txt`.
3. `installers/windows/CHANGELOG.md`: a **new section above R10.13.0**, same heading style:
   ```
   ## R10.13.1 - 2026-10-13  (`mscc-net9-R10-13-1-install.exe`, title bar 10.13.1)

   ### Changed
   - S/W window: REFRESH is now SMOOTHING. It sets the host's spectrum smoothing (0 = light, higher = calmer; default 3) and the spectrum and waterfall always run at full speed (no more dropped frames). The waterfall therefore scrolls faster than with the old REFRESH 3-4.
   - Waterfall: new lines now appear at the top and move down (Falling) by default. Tick "Climbing" for the old direction. Existing settings are moved to Falling once.
   - S/W window: BASELINE and the waterfall HIGH / LOW / GAIN / ZERO are remembered per band (bands with different noise keep their own settings). A band you have not adjusted keeps the current values.
   ```
   (Use the real number.)
4. Update `.mscc-coord/status/windows-new-hp.md` (cmd-075 row: client 10.13.1, WPF only, not committed).
5. Stew packages `mscc-net9-R10-13-1-install.exe` from `C:\mscc-net9` after the smoke test.

## Part E. Smoke (NEW-HP; Stew)
1. **SMOOTHING:**
   - The S/W window shows SMOOTHING (no REFRESH), with 3 selected on first start.
   - The ms-sdr log shows `CMD_GET_SET_PANADAPTER_SMOOTHING: 3` at Start, and `%LocalAppData%\MSCC-NET9\user_controls.ini` has `PANADAPTER_AVERAGE=3`.
2. **Smoothing effect:**
   - 0 gives a lively trace, 10 is calmer, 30 is slow.
   - Each change takes effect at once and appears in the ms-sdr log.
   - Levels are unchanged: TG -70 still reads -70 ±2 at 0 and at 10.
   - Restart MSCC: the last value is restored and pushed again.
3. **Full frame rate:** the trace updates smoothly at 15.6 fps, and the waterfall scrolls about 3× faster than before (it used REFRESH 3). Note whether that speed is acceptable (otherwise a later waterfall SPEED control). CPU load on NEW-HP is acceptable (Task Manager).
4. **Waterfall direction:**
   - New lines appear at the top and move down. The checkbox reads "Climbing" and is unticked.
   - Tick it: lines appear at the bottom and move up. It survives a restart, and Proficio/Geminus banks behave the same.
   - With an old-style ini (`WATERFALL_DIRECTION_NORMAL=1`, no `WATERFALL_FALLING_DEFAULT`), the first start is Falling.
5. **Per band:**
   - On 20 m set BASELINE and waterfall LOW/HIGH (and GAIN/ZERO) to one look. Change to 15 m and set a different look. Back to 20 m: the 20 m look returns, and the sliders in the open S/W window move with it. Back to 15 m: the 15 m look returns.
   - `MSCC_LastUsed.ini` has `20M_SW_BASELINE`, `20M_WF_HIGH`, etc.
   - A band never adjusted (e.g. 17 m) keeps the current look.
   - 630 m/2200 m (LF bank) work the same, and the HF/LF bank switch still works.
   - Restart: the per-band values persist.
6. **Regressions:**
   - Spectrum max-per-pixel and dB scale (TG -70 → -70), and dB CAL / grid unchanged (not per band).
   - S-meter needle and dBm toggle; ALC button still gone.
   - Remote host (Pi): SMOOTHING changes take effect there too (Pi recv has the same smoothing).
   - Title shows 10.13.1.

## Avalonia follow-up (later, separate brief; not now)
Avalonia has its own S/W settings (`Controls/SpectrumDisplaySettings.cs`: `WaterfallDirectionNormal` default true at 10/23/35/101; BASELINE and the waterfall levels in `Services/ClientSettingsStore.cs`). Check there for a REFRESH/frame-skip path before porting. Port A-C there once Windows is approved. The `MSCC.Core` method from A1 is already available to it.

STOP. No commit, no push.
