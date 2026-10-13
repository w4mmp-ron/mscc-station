# cmd-077: WPF waterfall WF SPEED (1-5, default 3) with max across skipped frames, and max (not blend) when the waterfall is squeezed

Host: **windows-new-hp**. Repo: `C:\Users\n8vet\OneDrive\Documents\GitHub\mscc-station`.
**WPF client only.** Avalonia gets the same changes in a later brief.
Source: Ron's `.mscc-coord/NOTE-FOR-STEW-WATERFALL-ANSWER-2026-10-13.md` (option (a)), plus Stew's choices: 1-5, default 3, under PALETTE COLOR.

`git pull --ff-only` first; the tree must be clean apart from this brief.
**Do NOT edit `rpi/`, `Solidus/`, `installers/rpi/`, any server, or `MSCC.Core`.** Minimal changes.
**STOP BEFORE COMMIT.** Show `git status`, `git diff --stat` and the full diffs.
Line numbers are from `f32b3d6`; check them before you edit.

**What changes:**
- The spectrum trace stays at the full 15.6 frames per second.
- The waterfall adds one row every N frames. Each row is the **per-point maximum** over those N frames, so short signals are kept.
- When the 300-row history is squeezed into fewer pixels, each pixel takes the **maximum** of the rows it covers instead of blending two rows.
- The setting is client-only: ini key `WATERFALL_SPEED_DIV` in `MSCC_Client.ini`. It is not per band and not sent to the host.

## A. Setting
1. **`Controls/SpectrumColorSettings.cs`** (renderer-side value, same pattern as `WaterfallTimeMarker`). Add near `WaterfallDirectionNormal` (about line 300):
   ```csharp
    /// <summary>cmd-077: waterfall row every N spectrum frames (1 = 15.6 rows/s). Row = per-point max over the N frames.</summary>
    public static int WaterfallSpeedDiv { get; private set; } = 3;
    public static void SetWaterfallSpeedDiv(int n) => WaterfallSpeedDiv = Math.Clamp(n, 1, 5);
   ```
2. **`SpectrumWaterfallSettings.cs`**
   - Next to `WaterfallTimeMarker` (line 56): `public static int WaterfallSpeedDiv { get; set; } = 3;` with the doc comment `cmd-077: WF SPEED 1-5, client only.`
   - Load, next to the `WATERFALL_TIME_MARKER` line (about 739): `if (LineMatchesKey(line, "WATERFALL_SPEED_DIV")) WaterfallSpeedDiv = Math.Clamp(ParseIniInt(line, WaterfallSpeedDiv), 1, 5);`
   - Save, next to `WATERFALL_DIRECTION_NORMAL`: `UpdateOrAdd(lines, "WATERFALL_SPEED_DIV", WaterfallSpeedDiv.ToString());`
   - In `ApplyLiveWaterfallToRenderer()` (about line 358, next to `SetWaterfallTimeMarker`): `SpectrumColorSettings.SetWaterfallSpeedDiv(WaterfallSpeedDiv);`
   - Also make sure it's applied at startup even if the S/W window is never opened. Check that `ApplyLiveWaterfallToRenderer` runs at startup (via `ApplyWaterfallBankForActiveBand`); if not, call `SetWaterfallSpeedDiv` at the end of `Load()`.

## B. UI: WF SPEED directly under PALETTE COLOR
**`PanadapterControlsWindow.xaml`** (about lines 221-231): inside the same `<StackPanel Margin="2">` as PALETTE COLOR, after the `WaterfallPaletteList` `</ListBox>`, add:
```xml
                                <TextBlock Text="WF SPEED" FontSize="10" Foreground="#D0D0D0" HorizontalAlignment="Center" Margin="0,6,0,0"
                                           ToolTip="Waterfall rows: 1 = every frame (fastest), 3 = default, 5 = slowest. Each row keeps the strongest signal of the frames it covers. Spectrum trace is not affected."/>
                                <ListBox x:Name="WaterfallSpeedList" Width="100" Height="55" FontSize="10"
                                         Background="#111" Foreground="White"
                                         SelectionChanged="OnWaterfallSpeedSelectionChanged">
                                    <ListBoxItem Content="1"/>
                                    <ListBoxItem Content="2"/>
                                    <ListBoxItem Content="3"/>
                                    <ListBoxItem Content="4"/>
                                    <ListBoxItem Content="5"/>
                                </ListBox>
```

**`PanadapterControlsWindow.xaml.cs`**
- In the constructor's selection block (next to `SelectItemByContent(WaterfallPaletteList, …)`, about line 59): `SelectItemByContent(WaterfallSpeedList, SpectrumWaterfallSettings.WaterfallSpeedDiv.ToString());`
- In `SuppressListSelectionHandlers` (about line 279), use the same pattern as `SmoothingList` for `WaterfallSpeedList` / `OnWaterfallSpeedSelectionChanged`.
- Add the handler:
  ```csharp
    private void OnWaterfallSpeedSelectionChanged(object sender, SelectionChangedEventArgs e)
    {
        if (sender is ListBox lb && lb.SelectedItem is ListBoxItem item &&
            int.TryParse(item.Content?.ToString(), out int n))
        {
            SpectrumWaterfallSettings.WaterfallSpeedDiv = Math.Clamp(n, 1, 5);
            SpectrumColorSettings.SetWaterfallSpeedDiv(SpectrumWaterfallSettings.WaterfallSpeedDiv);
            SpectrumWaterfallSettings.Save();
        }
    }
  ```
  It's a plain `Save()`, not `SaveLiveWaterfallAndActiveBank`, so the value is not per band.

## C. Renderer: row every N frames, per-point max (`Controls/SpectrumDisplayControl.xaml.cs`)
1. Fields, next to `_waterfallHistory` (line 24):
   ```csharp
    // cmd-077: frames since the last waterfall row, combined by per-point max (WF SPEED).
    private float[]? _wfAccum;
    private int _wfAccumCount;
   ```
2. `ClearWaterfallHistory()` (line 117), and the mixed-length clear block (about 197-203): also set `_wfAccum = null; _wfAccumCount = 0;`.
3. `UpdateSpectrumInternal` append block (about 192-231): after the length-mismatch clear, **replace** the part from `bool mark = false;` through the `while (… > MaxWaterfallHistory)` trim with:
   ```csharp
            // cmd-077: combine N frames into one row (per-point max), then push.
            float[] d = update.Data;
            if (_wfAccum == null || _wfAccum.Length != d.Length)
            {
                _wfAccum = (float[])d.Clone();
                _wfAccumCount = 1;
            }
            else
            {
                for (int i = 0; i < d.Length; i++)
                    if (d[i] > _wfAccum[i]) _wfAccum[i] = d[i];
                _wfAccumCount++;
            }

            if (_wfAccumCount >= SpectrumColorSettings.WaterfallSpeedDiv)
            {
                bool mark = false;
                string? timeLabel = null;
                int grid = SpectrumColorSettings.WaterfallTimeMarker;
                if (grid > 0)
                {
                    // (keep the existing time-marker block here unchanged)
                }

                _waterfallHistory.Add(_wfAccum);   // hand over; a new accumulator starts next frame
                _wfAccum = null;
                _wfAccumCount = 0;
                _waterfallTimeMarkers.Add(mark);
                _waterfallTimeLabels.Add(timeLabel);
                while (_waterfallHistory.Count > MaxWaterfallHistory)
                {
                    // (keep the existing trim body unchanged)
                }
            }
   ```
   Move the existing time-marker block and trim loop inside as marked; don't change their logic. Even at 5 × 64 ms = 0.32 s per row, a wall-clock second change is still caught.
   Higher values of `update.Data` mean a stronger signal, the same assumption as the cmd-068 max-per-pixel. Confirm this against `SampleWaterfallRow`.

## D. Renderer: max when squeezed (Ron's fix; same file, waterfall paint about 853-890)
Today each screen line blends the two nearest history rows (`li0` / `li1` / `t`). When `numLines > wfHeight` (e.g. 300 rows into 180 px), rows in between are skipped or diluted. Change:
1. Before the `for (int y …)` loop, add:
   ```csharp
            // cmd-077: more rows than pixels -> each pixel takes the max of the rows it covers (no lost short signals).
            bool squeeze = numLines > wfHeight;
   ```
2. Inside the `y` loop, after `li0` / `li1` / `t` are computed, work out the history rows this pixel covers (only used when `squeeze`). It runs from its own row index up to, but not including, the next pixel's; at least one row; the last pixel goes to the end:
   ```csharp
                int rLo = 0, rHi = 0;
                if (squeeze)
                {
                    double fracN = Math.Clamp((double)(y + 1 - wfStartY) / Math.Max(1, wfHeight - 1), 0.0, 1.0);
                    double lineIdxN = directionNormal ? fracN * (numLines - 1) : (1.0 - fracN) * (numLines - 1);
                    int a = (int)Math.Round(lineIdxD), b = (int)Math.Round(lineIdxN);
                    rLo = Math.Min(a, b);
                    bool lastPixel = y + 1 >= height - freqMargin;
                    rHi = lastPixel ? Math.Max(a, b) : Math.Max(rLo, Math.Max(a, b) - 1);
                    rLo = Math.Clamp(rLo, 0, numLines - 1);
                    rHi = Math.Clamp(rHi, rLo, numLines - 1);
                }
   ```
   Check by reasoning that every history row 0..numLines-1 is covered by some pixel, in both directions.
3. In the `x` loop, replace the `db0` / `db1` / `dbv` lines with:
   ```csharp
                    float dbv;
                    if (squeeze)
                    {
                        float best = float.MinValue;
                        for (int r = rLo; r <= rHi; r++)
                        {
                            float v = SampleWaterfallRow(waterfallHistory[r], dataFrac, dataFracNext, cwOffsetBins, refLen, minDb);
                            if (v > best) best = v;
                        }
                        dbv = SpectrumColorSettings.ToDisplayDb(best);
                    }
                    else
                    {
                        float db0 = SpectrumColorSettings.ToDisplayDb(
                            SampleWaterfallRow(row0, dataFrac, dataFracNext, cwOffsetBins, refLen, minDb));
                        float db1 = SpectrumColorSettings.ToDisplayDb(
                            SampleWaterfallRow(row1, dataFrac, dataFracNext, cwOffsetBins, refLen, minDb));
                        dbv = (float)(db0 * (1.0 - t) + db1 * t);
                    }
   ```
   (`ToDisplayDb` is monotonic, so the max of raw values then one conversion gives the same result as converting each.)
4. Cost: at most `numLines` (300) row samples per column per paint, the same order as today's 2 × height. Check that CPU stays OK at speed 1.
5. The time-marker line drawing (about 930-940) is unchanged.

## E. Build, version, CHANGELOG
1. Stop MSCC. `dotnet build MSCC.sln -c Release` from `mscc-ui/windows-work-tree/mscc-mscc/mscc-new`. This auto-bumps the version by date (10.13.3 if built today) and copies to `C:\mscc-net9`. No server rebuild.
2. `installers/windows/CHANGELOG.md`: a new section above R10.13.2 with the real number:
   ```
   ## R10.13.3 - 2026-10-13  (`mscc-net9-R10-13-3-install.exe`, title bar 10.13.3)

   ### Changed
   - S/W window: new WF SPEED (1-5, default 3) under PALETTE COLOR. The waterfall adds one row every N spectrum frames, and each row keeps the strongest signal of those frames, so short signals are not lost. The spectrum trace still runs at full speed.
   - Waterfall: when the window is small, each screen line now shows the strongest of the rows it covers instead of blending two, so short signals no longer fade or vanish.
   ```
3. `.mscc-coord/status/windows-new-hp.md`: a cmd-077 row (client version, WPF only, not committed).

## F. Smoke (Stew)
1. **Control:** the S/W window shows WF SPEED directly under PALETTE COLOR, with 3 selected on first start (`WATERFALL_SPEED_DIV=3` written to `MSCC_Client.ini`).
2. **Speed:** at 1 the waterfall scrolls at the full rate (as 10.13.2); at 3 about a third as fast (like the old REFRESH 3); at 5 slowest. The spectrum trace is equally lively at 1, 3 and 5.
3. **Short signals:** with CW (or short TG blips) at WF SPEED 5, every element still shows in the waterfall. It doesn't disappear between rows, though fast elements may merge.
4. **Small window:** make the main window or waterfall short (under 300 px). Short signals stay visible and bright; nothing fades or vanishes.
5. **Restart:** the WF SPEED value persists. Changing band doesn't change it (not per band).
6. **No regressions:**
   - Falling/Climbing direction and time markers line up.
   - Palette, WF HIGH/LOW/GAIN/ZERO, BASELINE and per-band levels are unchanged.
   - SMOOTHING is unchanged.
   - Max-per-pixel horizontally is still good (a carrier line stays solid when tuning).
   - CPU is OK at speed 1.
   - The title shows the new version.

## Avalonia follow-up (later, separate brief; not now)
Same three parts in `MSCC.Avalonia` (`Controls/SpectrumRenderer.cs` waterfall history/paint, `SpectrumDisplaySettings` / `ClientSettingsStore` for the ini key, the S/W window under the palette).

STOP. No commit, no push.
