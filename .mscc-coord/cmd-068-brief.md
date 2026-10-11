# cmd-068: WPF spectrum/waterfall draw the max per pixel; spectrum resolution fixed at 3200

Host: **windows-new-hp**. Repo: `C:\Users\n8vet\OneDrive\Documents\GitHub\mscc-station`.
Scope: **WPF client only** (`mscc-ui/windows-work-tree/mscc-mscc/mscc-new/src/...`). No server change, no server rebuild, no Avalonia, no protocol change.
**Do NOT touch `rpi/`, `Solidus/`, or `installers/rpi/`.** Stew: "Lets not go too crazy with the code changes." Keep each change small, with no refactors.
**STOP BEFORE COMMIT.** Show `git status` and `git diff --stat`, plus the full diff for each file.
Line numbers are from `ad54170` (2026-10-09). Check them before you edit.

## Background (read-only findings, confirmed by Stew 2026-10-09)
- On 3200 points, a steady S9 carrier's spectrum peak drops 10-15+ dB and jumps back as you tune slowly, while the S-meter stays steady.
  Stew's test: zoom 3x/4x has no drops, 2x has just under 10 dB of drops, and 1x is bad.
- Cause: the draw loop takes **one** data point per screen pixel and skips the rest.
  - `SpectrumDisplayControl.xaml.cs:729-732`: `idx = (int)(dataFrac * (data.Length - 1) + cwOffsetBins)`, then only `data[idx]` is drawn.
  - The waterfall does the same: call site 866-875, sampler `SampleWaterfallRow` 1291-1305.
- At 3200 points across a ~1000-1500 px plot, 2-3 of every 3 points are never drawn. A carrier (Hamming main lobe about +/-2 bins) can fall between drawn points.
- The recv server at 3200 already uses every FFT bin (0.96 bins per point), so it does not skip anything.
  At 800/1600 the Windows server **does** skip bins (one bin per point). Fixing the resolution at 3200 avoids that without a server change.
- Fix: draw the **max** of every data point the pixel covers. With fewer visible points than pixels (zoomed in), that is one point, which is today's behaviour.
- Resolution: `PAN_RESOLUTION` (index 0/1/2) is read at `SpectrumWaterfallSettings.cs:847-851` and saved at `:1127`. `PanResolutionBins` and `PanResolutionLabel` are at `:488-501`.
  `MainViewModel.ApplyPanResolution` (`ViewModels/MainViewModel.cs:5495-5518`, called on Start at `:5473`) sends `PanResolutionBins` via `SetPanResolutionAsync`.
  The selector is `PanadapterControlsWindow.xaml:145-157` (handler `PanadapterControlsWindow.xaml.cs:555-584`).

## 1. Max per pixel: spectrum trace and waterfall
File: `MSCC.Wpf/Controls/SpectrumDisplayControl.xaml.cs`

Pixel x covers the data points from its own first index (today's `idx`) up to, **not including**, the next pixel's first index, with at least one point.
Both indices come from the same `viewStart + t * viewWidth` (`dataFrac`) mapping and the same `cwOffsetBins` as today. This makes it correct at every zoom level and CW offset.
At zoom 1 it is the `floor(x*N/W) .. floor((x+1)*N/W)` rule, but written against the visible (zoomed) range.
At zoom levels where the next pixel's index is at most `idx + 1`, the range is one point, so behaviour is unchanged.
The last pixel (`x == width - 1`) uses one point, as today.

### 1a. Add a helper
Add it directly above `SampleWaterfallRow` (before the `/// <summary>` at line 1287):
```csharp
    /// <summary>
    /// cmd-068: largest value in data[i0 .. i1-1] (at least data[i0]). Indices are clamped.
    /// One pixel covers several data points when points &gt; pixels; drawing only one
    /// of them made a carrier dip 10-15 dB while tuning.
    /// </summary>
    private static float MaxInRange(float[] data, int i0, int i1)
    {
        int n = data.Length;
        i0 = Math.Clamp(i0, 0, n - 1);
        i1 = Math.Clamp(i1, i0 + 1, n);
        float m = data[i0];
        for (int i = i0 + 1; i < i1; i++)
            if (data[i] > m) m = data[i];
        return m;
    }
```

### 1b. Spectrum trace, lines 727-732
Replace these lines:
```csharp
            double t = width > 1 ? (double)x / (width - 1) : 0.5;
            double dataFrac = viewStart + t * viewWidth;
            int idx = (int)(dataFrac * (data.Length - 1) + cwOffsetBins);
            idx = Math.Clamp(idx, 0, data.Length - 1);

            float dbValue = SpectrumColorSettings.ToDisplayDb(data[idx]);
```
with:
```csharp
            double t = width > 1 ? (double)x / (width - 1) : 0.5;
            double dataFrac = viewStart + t * viewWidth;
            int idx = (int)(dataFrac * (data.Length - 1) + cwOffsetBins);
            idx = Math.Clamp(idx, 0, data.Length - 1);
            // cmd-068: max of all points up to (not incl.) the next pixel's first point.
            int idxNext = idx;
            if (x + 1 < width)
            {
                double dataFracNext = viewStart + ((double)(x + 1) / (width - 1)) * viewWidth;
                idxNext = (int)(dataFracNext * (data.Length - 1) + cwOffsetBins);
            }

            float dbValue = SpectrumColorSettings.ToDisplayDb(MaxInRange(data, idx, idxNext));
```
(`x + 1 < width` implies `width > 1`, so there is no divide by zero.)

### 1c. Waterfall call site, lines 866-875
Replace:
```csharp
                for (int x = 0; x < width; x++)
                {
                    double xt = width > 1 ? (double)x / (width - 1) : 0.5;
                    double dataFrac = viewStart + xt * viewWidth;

                    // Per-row Length: never index all rows with history[0].Length (Start can mix sizes).
                    float db0 = SpectrumColorSettings.ToDisplayDb(
                        SampleWaterfallRow(row0, dataFrac, cwOffsetBins, refLen, minDb));
                    float db1 = SpectrumColorSettings.ToDisplayDb(
                        SampleWaterfallRow(row1, dataFrac, cwOffsetBins, refLen, minDb));
```
with:
```csharp
                for (int x = 0; x < width; x++)
                {
                    double xt = width > 1 ? (double)x / (width - 1) : 0.5;
                    double dataFrac = viewStart + xt * viewWidth;
                    // cmd-068: the next pixel's start; the last pixel uses one point.
                    double dataFracNext = x + 1 < width
                        ? viewStart + ((double)(x + 1) / (width - 1)) * viewWidth
                        : dataFrac;

                    // Per-row Length: never index all rows with history[0].Length (Start can mix sizes).
                    float db0 = SpectrumColorSettings.ToDisplayDb(
                        SampleWaterfallRow(row0, dataFrac, dataFracNext, cwOffsetBins, refLen, minDb));
                    float db1 = SpectrumColorSettings.ToDisplayDb(
                        SampleWaterfallRow(row1, dataFrac, dataFracNext, cwOffsetBins, refLen, minDb));
```

### 1d. `SampleWaterfallRow`, lines 1287-1305
Replace the whole method (summary included) with:
```csharp
    /// <summary>
    /// Sample one waterfall history row using that row's own Length (never history[0].Length).
    /// Scales CW offset when the row bin count differs from the current spectrum.
    /// cmd-068: returns the max of the row's points from dataFrac up to (not incl.) dataFracNext.
    /// </summary>
    private static float SampleWaterfallRow(
        float[]? row, double dataFrac, double dataFracNext, int cwOffsetBins, int refLen, float fallbackDb)
    {
        if (row == null || row.Length == 0)
            return fallbackDb;
        int len = row.Length;
        if (len == 1)
            return row[0];
        int cw = refLen > 0
            ? (int)Math.Round(cwOffsetBins * (len / (double)refLen))
            : 0;
        int idx = (int)(dataFrac * (len - 1) + cw);
        int idxNext = (int)(dataFracNext * (len - 1) + cw);
        return MaxInRange(row, idx, idxNext);
    }
```
`SampleWaterfallRow` has no other callers (checked with `git grep`). Leave the peak marker (`FindLocalPeakBin`) as is; it already scans the data.

Expected side effect: at 1x zoom the noise floor (grass) reads a little higher and smoother, because it is now the largest of 2-3 noise points per pixel. That is normal for a peak-per-pixel display.

## 2. Resolution fixed at 3200 (Max)
Keep the server protocol as is: the client still sends 0x5F index 2 (3200) plus refresh blocks 3 through `SetPanResolutionAsync`. Pi, Linux and Windows servers all accept it.

### 2a. `MSCC.Wpf/SpectrumWaterfallSettings.cs`
1. Lines 482-485: change the summary and default:
   ```csharp
    /// <summary>
    /// cmd-068: spectrum resolution is fixed at Max (3200 bins across 72 kHz). Index kept only
    /// so PAN_RESOLUTION still reads/writes; old 0/1 (800/1600) values are overridden to 2.
    /// </summary>
    public static int PanResolutionIndex { get; set; } = 2;
   ```
2. Lines 487-501: replace both switch properties with constants:
   ```csharp
    /// <summary>Bins sent to SDRcore: always 3200 (cmd-068).</summary>
    public static int PanResolutionBins => 3200;

    /// <summary>Log label.</summary>
    public static string PanResolutionLabel => "Max (3200)";
   ```
3. Lines 847-851 (load): ignore the saved value:
   ```csharp
                    if (LineMatchesKey(line, "PAN_RESOLUTION"))
                    {
                        // cmd-068: fixed at Max (3200); an old 0/1 (800/1600) is overridden.
                        PanResolutionIndex = 2;
                    }
   ```
4. Line 1127 (save): no change. It now always writes `PAN_RESOLUTION=2`, so the ini heals on the next save.

### 2b. Hide the selector: `MSCC.Wpf/PanadapterControlsWindow.xaml` lines 145-157
On the `<StackPanel Margin="12,0,0,0" VerticalAlignment="Top">` that holds the "SPECTRUM RESOLUTION" TextBlock and `PanResolutionList` (line 146), add `Visibility="Collapsed"`. Change the comment on line 145 to
`<!-- cmd-068: spectrum resolution fixed at Max (3200); selector hidden. Display zoom is separate. -->`.
This hides the label, the list and both tooltips. Leave the ListBox in the XAML, so `PanadapterControlsWindow.xaml.cs` (`SelectPanResolutionList` 555-563, `OnPanResolutionSelectionChanged` 565-584, and the suppress code at 292-295) still compiles unchanged.
Nothing else is needed: `ApplyPanResolution` (MainViewModel 5495) already sends `PanResolutionBins`, which is now always 3200, on every Start.
`UdpRadioService` (constructor `SetPanResolutionLocal(800)` at line 194) is overridden by `ApplyPanResolution("start")` right after connect. Leave MSCC.Core as is.

## 3. Version (do not hand-edit)
`ClientVersion.txt` is 10.8.1. The build target `IncrementClientVersion` runs `bump-client-version.ps1`, which writes M.D.I: same day means I+1, and a new day resets I to 0.
So the **first Release build today (2026-10-09) gives 10.9.0** (if you build more than once, it goes 10.9.1, 10.9.2, ...). Use whatever the final build prints ("MSCC client version → ..."),
and use that same number in the CHANGELOG heading and the installer name.
The exe in `C:\mscc-net9` (`MSCC.Wpf.exe`, ProductVersion) and the title bar must show that number (standing rule: Stew packages from `C:\mscc-net9`).

## 4. CHANGELOG: `installers/windows/CHANGELOG.md`
Insert above `## R10.8.1 ...` (replace 10.9.0 / 10-9-0 with the final client version from step 3 if it differs):
```
## R10.9.0 — 2026-10-09  (`mscc-net9-R10-9-0-install.exe`, title bar 10.9.0)

### Fixed
- Spectrum and waterfall: a steady signal no longer dips by 10-15 dB (and its waterfall line no longer breaks up) as you tune, at any zoom. Each screen pixel now shows the strongest point it covers instead of a single sample.

### Changed
- SPECTRUM RESOLUTION is fixed at Max (3200) and the selector is gone from the S/W controls. A saved Normal/High setting is ignored. Use the spectrum zoom to see more detail.
- At 1x zoom the noise floor on the spectrum looks slightly higher and smoother than before (same signal levels).
```

## 5. Build and deploy (Windows, WPF only)
1. No server rebuild: trans, recv and ms-sdr stay as they are in `C:\mscc-net9` (built 2026-10-08).
2. From `mscc-ui/windows-work-tree/mscc-mscc/mscc-new`: `dotnet build MSCC.sln -c Release`. This runs the `ClientVersion.txt` auto-bump (M.D.I) and the `CopyToMsccNet9` target (copies the output to `C:\mscc-net9`).
3. Check that `(Get-Item C:\mscc-net9\MSCC.Wpf.exe).VersionInfo.ProductVersion` starts with the new M.D.I and has today's time. Record the client M.D.I.

## 6. Smoke (NEW-HP, signal generator or a steady S9 carrier)
1. 1x zoom, main window maximized: tune slowly +/- across a few kHz in small steps. The carrier peak stays within about 1-2 dB, with no 10-15 dB drops. The S-meter stays steady as before.
2. Same at 2x, 3x and 4x zoom: no drops (2x used to give about 10 dB).
3. Waterfall: the carrier line stays solid (no gaps or dashed segments) while tuning at 1x and 2x.
4. Resize the main window narrower and wider: the carrier no longer comes and goes.
5. S/W controls window: the SPECTRUM RESOLUTION label and list are gone. The rest of the window (REFRESH, AUTO SNAP, palettes, etc.) works as before.
6. Old ini: close MSCC, and in `%LocalAppData%\MSCC-NET9\MSCC_Client.ini` change `PAN_RESOLUTION=2;` to `PAN_RESOLUTION=0;` (800). Start.
   The log shows "Pan resolution (start): Max (3200) → 3200 bins" and the recv log "PAN RESOLUTION → 3200 bins". At 4x zoom the detail is fine (narrow carrier, 3200-point look).
   After a settings save the ini reads `PAN_RESOLUTION=2;` again.
7. CW mode with a CW pitch offset: the trace and waterfall line up with the filter shading as before (no shift).
8. The title bar and AUD/SYS show the new client version (10.9.0 or the final M.D.I).

## What NOT to touch
`rpi/`, `Solidus/`, `installers/rpi/`, all servers (`SDRcore-recv`, `SDRcore-trans`, `ms-sdr-MKII`, Linux), MSCC.Core (`UdpRadioService`), the 0x5F protocol, Avalonia, and firmware.
The dB scale (RawYToDb / dB CAL) is a separate item (NOTE-FOR-STEW-SPECTRUM-DB-SCALE-2026-10-07); don't change it here.

## Report
Full diffs (expect 4 code files + CHANGELOG: SpectrumDisplayControl.xaml.cs, SpectrumWaterfallSettings.cs, PanadapterControlsWindow.xaml, ClientVersion.txt (auto) + installers/windows/CHANGELOG.md), the final client M.D.I, the `C:\mscc-net9\MSCC.Wpf.exe` version and time, and smoke results 1-8 (or "not run, no radio").

STOP. No commit, no push.
