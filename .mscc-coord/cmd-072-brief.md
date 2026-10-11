# cmd-072: Ubuntu: Linux catches up with Windows cmd-068/069/070/071 (recv 3.144 + Avalonia 0.6.74), mscc 1.0.52 / mscc-ui 0.6.74

Host: **stew-HP** (Ubuntu, x86_64). Repo: `/home/stew/Documents/GitHub/mscc-station`, at `32d6c3c` (clean, up to date with origin/main; checked 2026-10-11).
Scope: **Ubuntu recv** (`linux/SDRcore-recv-linux/sources/`) and **Avalonia** (`mscc-ui/Avalonia-Migration/src/MSCC.Avalonia/`), plus the version, packaging and CHANGELOG.
**Do NOT edit `rpi/`, `Solidus/`, or `installers/rpi/`** (Ron's). Read `rpi/` only, as the source of the port.
Don't touch: MSCC.Core (already true dB since cmd-069), WPF, Windows recv, `SDRcore-trans-linux`, `ms-sdr-linux` source (only its auto-bumped `version.h`), the panadapter wire format, firmware.
Avalonia arm64/Pi packaging is Ron's area and out of scope: don't run `mscc-ui-arm64.sh` / `build-mscc-ui-deb-arm64.sh`.
Stew: "Lets not go too crazy with the code changes." Keep every change small. No refactors.
**STOP BEFORE COMMIT.** Show `git status` and `git diff --stat`, plus the full diff of every source file.
Line numbers are from `32d6c3c`. Check them before you edit.

References (read-only): `.mscc-coord/cmd-068-brief.md`, `cmd-069-brief.md`, `cmd-070-brief.md`, `cmd-071-brief.md`; the Windows commits `git show f952111` (cmd-068/069/070) and `git show 32d6c3c` (cmd-071); Ron's Pi commits `2b4da34`, `8d7eca4`, `a8094ee`.
(The Windows cmd-068 work is inside `f952111`; there is no separate `d010da8` in this repo.)

---

## Part A. Ubuntu recv: smoothing, max-over-bins, LO-point levelling (recv 3.144)

What was checked on stew-HP:
- `linux/.../panadapter.c` differs from the Pi file only in the smoothing code (old 4-frame history vs Ron's running average). It already has the cmd-069 32-bit averaging fix. So **copy the Pi file verbatim**.
- `linux/.../dsputils.c` differs from Pi `a8094ee` (= HEAD) in these places:
  - **Pi-only spectrum changes, which are the port:** max-over-bins (`2b4da34`, the cmd-069 Windows change), the LO-point levelling (16 bins, after-TX hold), and `DC_BLOCK_A` 0.9999 with its comment.
  - **Ubuntu-only code, which stays:** two `#if WINDBG > 0` blocks in `doAGC` (dead code, since `sdrcore.h:29` sets `WINDBG 0`) and the old calibration casts in `doRxCalibrate`.
  - **Pi-only calibration change, NOT ported:** the `cal_mag_scaled()` helper and the `calMagLowF/calMagF/calMagHighF` assignments. The Linux `sdrcore.h` has no `calMag*F` fields, and the Pi change also needs `sdrcore.h` and `udp_thread.c`. So **the Pi file verbatim would not compile on Ubuntu.**
  - **Approach chosen:** copy the Pi file, then put back the Ubuntu-only code (A2 below). The net change is the spectrum hunks only.
  - Cross-check: the result's diff against the Windows `dsputils.c` (32d6c3c) should show only Windows comment wording (the "(Ported from the Pi recv, cmd-071.)" line, the packing comment, FM-filter comments) and the `(sp_float) sqrt` cast.
- `extern.h` already declares `G_Smoothing`, `G_Panadapter_Blocks`, `G_tx_mode` and `G_Pause_Panadapter`, and includes `<math.h>` (for `expf`/`sqrt`). `sdrcore.h`/`dsputils.h` need no change.
- **How smoothing reaches the Ubuntu recv (no change needed):**
  - `ms-sdr-linux/source/user_controls.c:346` reads `PANADAPTER_AVERAGE` from `~/.local/mscc/user_controls.ini` and sends it to recv as 0xDA.
  - `udp_thread.c:1176` sets `G_Smoothing = value + 2` and logs `CHANGED/unchanged`.
  - On stew-HP the ini has `PANADAPTER_AVERAGE=0;`, and the template `linux/mscc-init-files-linux/user_controls.ini` also has 0. With 0 it behaves as today. Avalonia never sends 0xDA.

### A1. `panadapter.c`: verbatim from the Pi
```
cp rpi/SDRcore-recv-linux/sources/panadapter.c linux/SDRcore-recv-linux/sources/panadapter.c
cmp rpi/SDRcore-recv-linux/sources/panadapter.c linux/SDRcore-recv-linux/sources/panadapter.c && echo IDENTICAL
```
The `cmp` must print `IDENTICAL`. `git diff` on the file should show only the `8d7eca4` smoothing change:
- `prev[MAX_X]`/`avg[MAX_X]`, `pan_average_primed`;
- `PAN_AVG_MAX_SETTING 100`, `smoothing = G_Smoothing - 2`, `alpha = 1 - expf(...)`;
- 0 = (this + previous)/2, 1..100 = running average; no update while pausing;
- the old 4-frame `memcpy` history removed.

If anything else shows, stop and report. Don't hand-merge.

### A2. `dsputils.c`: Pi file, then put back the Ubuntu-only code
1. `cp rpi/SDRcore-recv-linux/sources/dsputils.c linux/SDRcore-recv-linux/sources/dsputils.c`
2. In `doAGC`, directly after `        int j;` (the line before `        if (AGC_Initializing)return;`), insert:
```c
#if WINDBG > 0
        static int blocks = 0;
        char buf[255];
#endif
```
3. In `doAGC`, after the `for` loop's closing `        }` and before the function's closing `}` (directly after `samps[j].imag = qoutput;` / `        }`), insert:
```c
#if WINDBG > 0
        blocks++;
        if (blocks == 10) {
                blocks = 0;
                sprintf(buf, "%14.8f Peak RX Signal\n", peak);
                OutputDebugStringA(buf);
                sprintf(buf, "%14.8f AGC Gain\n\n", agcstate.gain);
                OutputDebugStringA(buf);
        }
#endif
```
4. Delete the whole `/* mag * 1e6 as int, clamped ... */ static unsigned int cal_mag_scaled(...) { ... }` helper (9 lines plus its blank line) just above `/***** 3-freq Goertzel detector`.
5. In `doRxCalibrate`, replace these lines:
```c
        mycalstate.calMagLowF = magm;
        mycalstate.calMagF = mag;
        mycalstate.calMagHighF = magp;
        // Scaled ints (image check) saturate at INT_MAX, as the old cast did on arm64.
        mycalstate.calMagLow = cal_mag_scaled(magm);
        mycalstate.calMag = cal_mag_scaled(mag);
        mycalstate.calMagHigh = cal_mag_scaled(magp);
```
with the Ubuntu lines:
```c
        mycalstate.calMagLow = (int) (magm * 1000000.0f);
        mycalstate.calMag = (int) (mag * 1000000.0f);
        mycalstate.calMagHigh = (int) (magp * 1000000.0f);
```
**Checks:**
- `diff rpi/SDRcore-recv-linux/sources/dsputils.c linux/SDRcore-recv-linux/sources/dsputils.c` shows **only** steps 2-5 (two WINDBG inserts, the helper, the cal lines).
- `git diff linux/SDRcore-recv-linux/sources/dsputils.c` shows **only** the spectrum changes:
  - the `PAN_DC_*` comment + defines before `doPanadapter`;
  - the `dc_*` statics/locals;
  - `if (G_tx_mode) dc_hold = PAN_DC_HOLD_FFTS;`;
  - the levelling block after the second shuffle loop;
  - max-over-bins in the pixel loop (`bend`/`best`, `fpos += fstep` moved up);
  - the `DC_BLOCK_A` comment and `0.98f` → `0.9999f`.
- `git diff --no-index linux/SDRcore-recv-linux/sources/dsputils.c mscc-ui/windows-work-tree/SDRcore-recv/sources/dsputils.c --ignore-cr-at-eol` shows only Windows comment wording and the `(sp_float) sqrt` cast.

### A3. Not ported: Pi S-meter average power (`sdrcore.c`, `48a04b6`, Pi mscc 1.0.58)
Windows doesn't have it, so leave it out to keep Linux = Windows. **Open option** for a later command (Windows + Ubuntu together, if Stew wants it).

### A4. Version
`linux/SDRcore-recv-linux/sources/extern.h:41`: `#define VERSION_MINOR 143` → `144` (recv **3.144**; Ubuntu numbers itself, and Windows is 3.146). Nothing else.

### A5. Build the mscc amd64 deb (1.0.52)
1. `linux/mscc-deb/packaging/DEBIAN/control:2`: `Version: 1.0.51` → `Version: 1.0.52`.
2. Stop MSCC (UI and servers).
3. `./linux-build/build-mscc-deb-amd64.sh`. It builds all servers into `$HOME/mscc` via `mscc-linux.sh` (ms-sdr `version.h` auto-bumps; report the new ms-sdr number), packages `linux/mscc-deb/mscc_1.0.52_amd64.deb`, and runs `drop-installers.sh linux`.
4. No new gcc warnings in `panadapter.c`/`dsputils.c` (show the compile lines for them). Report recv 3.144, trans (3.142, unchanged) and the ms-sdr version.
5. Install it for the smoke: `sudo apt install ./installers/linux/mscc_1.0.52_amd64.deb`.

---

## Part B. Avalonia: cmd-068/069/070 (mscc-ui 0.6.74)
Path prefix: `mscc-ui/Avalonia-Migration/src/MSCC.Avalonia/`. Avalonia builds the shared MSCC.Core (`MSCC.Avalonia.csproj:35`). `UdpRadioService.RawYToDb` is already true dB (scale 75 / bias 80 / clamp 140). **No Core change.**

### B1. cmd-068: max per pixel, resolution fixed at 3200

**a) `Controls/SpectrumRenderer.cs`: add a helper** next to `ToDisplayDb` (line 11). `ToDisplayDb` is `raw + offset`, so taking the max before converting is correct.
```csharp
    /// <summary>
    /// cmd-068: largest value in data[i0 .. i1-1] (at least data[i0]). Indices are clamped.
    /// One pixel covers several points when points &gt; pixels; drawing only one of them
    /// made a carrier dip 10-15 dB while tuning.
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
**b) Spectrum loop (lines 90-96).** Replace
```csharp
            int idx = (int)Math.Round(dataFrac * (data.Length - 1) + cwOffsetBins);
            idx = Math.Clamp(idx, 0, data.Length - 1);

            float dbv = ToDisplayDb(data[idx], dbOffset);
```
with
```csharp
            int idx = (int)Math.Round(dataFrac * (data.Length - 1) + cwOffsetBins);
            idx = Math.Clamp(idx, 0, data.Length - 1);
            // cmd-068: max of all points up to (not incl.) the next pixel's first point.
            int idxNext = idx;
            if (x + 1 < width)
            {
                double dataFracNext = viewStart + ((double)(x + 1) / (width - 1)) * viewWidth;
                idxNext = (int)Math.Round(dataFracNext * (data.Length - 1) + cwOffsetBins);
            }

            float dbv = ToDisplayDb(MaxInRange(data, idx, idxNext), dbOffset);
```
**c) Waterfall loop (lines 201-208).** Same pattern, using `Math.Round` at both ends to match:
```csharp
                    int idx = (int)Math.Round(dataFrac * (lineLen - 1) + cwOffsetBins);
                    idx = Math.Clamp(idx, 0, lineLen - 1);
                    // cmd-068: max of the row's points up to (not incl.) the next pixel's first point.
                    int idxNext = idx;
                    if (x + 1 < width)
                    {
                        double dataFracNext = viewStart + ((double)(x + 1) / (width - 1)) * viewWidth;
                        idxNext = (int)Math.Round(dataFracNext * (lineLen - 1) + cwOffsetBins);
                    }

                    float dbv = ToDisplayDb(MaxInRange(line, idx, idxNext), dbOffset);
```
(`MaxInRange` clamps to `line.Length`, so a row of another size can't index out of range.) The last pixel uses one point, as today. When zoomed in (fewer points than pixels), the range is one point, so behaviour is unchanged. Expected side effect: at 1x the grass sits a little higher and smoother.

**d) Hide the selector: `Views/SpectrumWaterfallWindow.axaml:154`.** On the `<StackPanel>` that holds the "SPECTRUM RESOLUTION" TextBlock and its ListBox (lines 154-165), add `IsVisible="False"`, and put the comment `<!-- cmd-068: spectrum resolution fixed at Max (3200); selector hidden. -->` above it. Leave the ListBox and its binding in place, so nothing else needs changing.

**e) `Services/ClientSettingsStore.cs`:**
- Line 132-133: change the summary to `cmd-068: fixed at Max (3200); index kept only so PAN_RESOLUTION still reads/writes.` and the default to `public int PanResolutionIndex { get; set; } = 2;`.
- Lines 405-417: replace both switches with
```csharp
    /// <summary>Bins sent to SDRcore: always 3200 (cmd-068).</summary>
    public static int PanResolutionBins(int index) => 3200;

    public static string PanResolutionLabel(int index) => "Max (3200)";
```
  (Keep `ClampPanResolutionIndex` at line 403.)
- Load (lines 736-739): `case "PAN_RESOLUTION": s.PanResolutionIndex = 2; break; // cmd-068: old 0/1 overridden`.
- `ViewModels/MainViewModel.cs:55`: `private int _panResolutionIndex = 2;`. Leave `ApplyPanResolutionAsync` (6537), which now always sends 3200.

### B2. cmd-069/070 view defaults + one-time reset

**a) `Controls/SpectrumDisplaySettings.cs`:**
- Line 78-79: `/// <summary>Absolute dB CAL at trim 0 (true-dB scale, TG step test 2026-10-10; WPF SpectrumDbCalCenter).</summary>` and `public const float DbCalCenterAbsolute = -83.6f;`. The trim range stays ±20 (139, 146 already).
- Bank property defaults, lines 8-9 and 12: `WaterfallHighDb = -40f`, `WaterfallLowDb = -150f`, `GridMinDb = -155f`.
- `CreateProficioDefaults` (Proficio/HF bank, lines 22-26): `WaterfallHighDb = -40f, WaterfallLowDb = -115f, GridMinDb = -155f` (comment: `// cmd-069/070 true-dB defaults (WPF ShipHf*)`).
- `CreateGeminusDefaults` (Geminus/LF bank, lines 34-38): `WaterfallHighDb = -35f, WaterfallLowDb = -105f, GridMinDb = -155f` (`// WPF ShipLf*`).
- Live fields, lines 96-98: `GridMinDb = -155f`, `WaterfallHighDb = -40f`, `WaterfallLowDb = -150f`.
- `SetGrid` fallback, line 157: `minDb = maxDb - 135f; // e.g. −20 → −155`.

**b) Same live values elsewhere:**
- `Services/ClientSettingsStore.cs:137-139`: `GridMinDb = -155f`, `WaterfallHighDb = -40f`, `WaterfallLowDb = -150f`.
- `ViewModels/SpectrumWaterfallViewModel.cs:58-60`: `-155`, `-40`, `-150`.

**c) Text:** in `Views/SpectrumWaterfallWindow.axaml:117`, the tooltip becomes `"Relative trim ±20 dB around −83.6 absolute"`. The slider ranges already allow the new values: GridMin −180…−40, WF high −100…−20, WF low −160…−40.

**d) One-time reset in `ClientSettingsStore.Load()` (lines 195-229).** Dropping the old values is safe because Avalonia keeps dB CAL as a **relative** trim (`DB_CAL_REL`), so resetting it to 0 gives the new −83.6 centre.
- Before the `foreach`, add `bool hadDbLaw = false;`.
- In the loop, before `Apply(s, key, val);`, add `if (key.Equals("SPECTRUM_DB_LAW", StringComparison.OrdinalIgnoreCase)) hadDbLaw = true;`.
- After `MigrateLegacyLiveIntoBanks(s, hadBankKeys);`, add:
```csharp

            // cmd-069/070: spectrum is now true dB (was half-dB). dB CAL trim, grid floor and
            // waterfall levels saved for the old scale are reset once to the new defaults, then
            // written back (SPECTRUM_DB_LAW=20) so this runs only once.
            if (!hadDbLaw)
            {
                var hf = SpectrumSwBank.CreateProficioDefaults();
                var lf = SpectrumSwBank.CreateGeminusDefaults();
                s.DbCalRelative = 0f;
                s.GridMinDb = -155f;
                s.WaterfallHighDb = -40f;
                s.WaterfallLowDb = -150f;
                s.HfBank.GridMinDb = hf.GridMinDb;
                s.HfBank.WaterfallHighDb = hf.WaterfallHighDb;
                s.HfBank.WaterfallLowDb = hf.WaterfallLowDb;
                s.LfBank.GridMinDb = lf.GridMinDb;
                s.LfBank.WaterfallHighDb = lf.WaterfallHighDb;
                s.LfBank.WaterfallLowDb = lf.WaterfallLowDb;
                Save(s);
            }
```
  (A missing ini returns early at line 199-200 with the new defaults. The marker is then written by the first normal save.)
- In `Save()`, "# Global S/W" block (after `DB_CAL_REL`, line 337), add `sb.AppendLine("SPECTRUM_DB_LAW=20");`.
- `Apply` needs no case for it: its `switch` has no `default`, so unknown keys are ignored.

### B3. cmd-070: S-meter readout click toggles S units / dBm (persisted), nearest-S rounding
Avalonia's S-meter is RX-only: there is no FWD/SWR/RESET face on this control. So there is no TX path to protect; TX behaviour stays as it is.

**a) `ViewModels/MainViewModel.cs`:**
- After line 320 (`_smeterPeak`), add:
```csharp
    /// <summary>cmd-070: last S-meter report in whole dBm.</summary>
    [ObservableProperty] private int _sMeterDbm = -130;
    /// <summary>cmd-070: S-meter readout shows dBm instead of S units. Persisted SMETER_DBM.</summary>
    [ObservableProperty] private bool _smeterShowDbm;
```
- Next to `OnSmeterPeakChanged` (~2088): `partial void OnSmeterShowDbmChanged(bool value) => ScheduleSaveClientSettings();`
- `ApplySmeterSample` (1705): add `SMeterDbm = dbm;` as the first line.
- `DbToSmeter` (1683-1685): replace `int smeterValue = 9 + (dbm + 73) / 6;` with
```csharp
            // cmd-070: nearest S unit, a tie goes up (WPF): −76…−73 = S9, −82…−77 = S8.
            int smeterValue = 9 + (int)Math.Floor((dbm + 73) / 6.0 + 0.5);
```
  The over-S9 branch and the limits stay as they are.
- Load (after `SmeterPeak = s.SmeterPeak;`, 6720): `SmeterShowDbm = s.SmeterDbm;`. Save builder (after `SmeterPeak = SmeterPeak,`, ~6860): `SmeterDbm = SmeterShowDbm,`.

**b) `Services/ClientSettingsStore.cs`:**
- After `SmeterPeak` (line 125): `public bool SmeterDbm { get; set; }`.
- Save (after line 331): `sb.AppendLine($"SMETER_DBM={(s.SmeterDbm ? "1" : "0")}");`.
- Apply (after `SMETER_PEAK`, line 720): `case "SMETER_DBM": s.SmeterDbm = IsTruthy(val); break;`.

**c) `Controls/AnalogSMeterControl.axaml` (lines 30-43):** give the readout Border `x:Name="ReadingBorder"`, add `Cursor="Hand"` and `ToolTip.Tip="Click: S units / dBm"`, and change `MinWidth="48"` to `MinWidth="60"` ("-110 dBm" fits). If the HOLD/Peak row clips at the 180 px meter width, report it rather than redesigning.

**d) `Controls/AnalogSMeterControl.axaml.cs`:**
- After `PeakHoldSecondsProperty` (35-36), add:
```csharp
    /// <summary>cmd-070: last S-meter report in whole dBm.</summary>
    public static readonly StyledProperty<int> DbmProperty =
        AvaloniaProperty.Register<AnalogSMeterControl, int>(nameof(Dbm), -130);

    /// <summary>cmd-070: readout in dBm (true) or S units (false). Click toggles.</summary>
    public static readonly StyledProperty<bool> ShowDbmProperty =
        AvaloniaProperty.Register<AnalogSMeterControl, bool>(
            nameof(ShowDbm), false, defaultBindingMode: BindingMode.TwoWay);

    public int Dbm { get => GetValue(DbmProperty); set => SetValue(DbmProperty, value); }
    public bool ShowDbm { get => GetValue(ShowDbmProperty); set => SetValue(ShowDbmProperty, value); }
```
- In `OnPropChanged` (89-95), add `else if (e.Property == DbmProperty || e.Property == ShowDbmProperty) UpdateNeedles();`.
- In `WireCheckBoxes` (96+), add:
```csharp
        if (ReadingBorder != null)
        {
            ReadingBorder.PointerPressed -= OnReadingPressed;
            ReadingBorder.PointerPressed += OnReadingPressed;
        }
```
  and add the handler:
```csharp
    // cmd-070: click the readout to toggle S units / dBm (persisted via the ShowDbm binding).
    private void OnReadingPressed(object? sender, global::Avalonia.Input.PointerPressedEventArgs e)
    {
        ShowDbm = !ShowDbm;
        e.Handled = true;
    }
```
- In `UpdateNeedles` (492-493): `ReadingText.Text = ShowDbm ? $"{Dbm} dBm" : FormatSmeterReading((int)Math.Round(main));`.

**e) `Views/MainWindow.axaml:182-189`:** on `<ctrl:AnalogSMeterControl ...>`, add `Dbm="{Binding SMeterDbm}"` and `ShowDbm="{Binding SmeterShowDbm, Mode=TwoWay}"`. Change the tooltip to `"S-meter (live). Click the reading for S units / dBm."`.

### B4. Version 0.6.73 → 0.6.74, then build the amd64 deb
1. Version strings:
   - `MSCC.Avalonia.csproj:8`: `<Version>0.6.74</Version>`.
   - `ViewModels/MainViewModel.cs:494`: `_clientVersionText = "0.6.74"` (feeds the window title).
   - `ViewModels/MainViewModel.cs:188`: `AppendLog("MSCC Avalonia 0.6.74 — spectrum true dB + max per pixel, S-meter dBm click.");`.
   - `mscc-ui/Avalonia-Migration/packaging/mscc-ui/DEBIAN/control:2`: `Version: 0.6.74`. (This template is shared with the arm64 builder. That's fine, just note it for Ron.)
   - Check with `grep -rn "0\.6\.73" mscc-ui/Avalonia-Migration --include=*.cs --include=*.csproj --include=*.axaml --include=control`: nothing left.
2. Build:
```
rm -rf mscc-ui/Avalonia-Migration/publish/linux-x64-sc && ./linux-build/mscc-ui-x64.sh && ./linux-build/build-mscc-ui-deb-amd64.sh
```
   - No new warnings in the changed files. The scripts copy `mscc-ui_0.6.74_amd64.deb` to `mscc-ui/Release/avalonia/x86_64/` and `installers/linux/`, and remove 0.6.73 there.
   - Install it: `sudo apt install ./installers/linux/mscc-ui_0.6.74_amd64.deb`.

---

## Part C. CHANGELOG, installers, status
1. `installers/linux/`: check that only `mscc_1.0.52_amd64.deb` and `mscc-ui_0.6.74_amd64.deb` are there (1.0.51 / 0.6.73 removed by `drop-installers.sh`). firmware, init-gui and portaudio are unchanged. Show `ls installers/linux`.
2. `installers/linux/CHANGELOG.md`:
   - In the "Currently shipped" table: mscc **1.0.52**, mscc-ui **0.6.74**.
   - Add a new section above 1.0.51; mark 1.0.51 `(superseded)`.
   - Remove the "Spectrum strongest bin per display point (done on the Pi, not yet ported)" line from "Not yet in an installer".

   The new section:
```
## mscc 1.0.52 / mscc-ui 0.6.74 — 2026-10-11

### Fixed
- Spectrum: a steady signal no longer drops 10-15 dB and jumps back while tuning (each screen pixel now shows the strongest of the points it covers, in the client and in the receiver).
- Spectrum dB scale now matches real dB: a 20 dB change in signal moves the trace 20 dB, so one dB CAL holds at every level.
- Spectrum: no more dark hole (or spike) at VFO -12 kHz; the noise there is levelled to match its surroundings. After a transmission the middle of the spectrum no longer shows a slow-fading dip.

### Added
- S-meter: click the reading to switch between S units and dBm. The choice is remembered.

### Changed
- Spectrum resolution fixed at Max (3200); the selector is gone.
- On first start, dB CAL trim, GRID MIN and the waterfall LOW/HIGH are reset once to new defaults (trim 0 = -83.6, GRID MIN -155, waterfall Proficio -115/-40, Geminus -105/-35). Set dB CAL once with a known signal if you need exact levels.
- S-meter S units round to the nearest unit (S9 is -76 to -73 dBm).
- Spectrum smoothing (PANADAPTER_AVERAGE in ~/.local/mscc/user_controls.ini): 0 looks as before; 1 to 100 give a calmer trace at full frame rate (value x 0.025 s).
- Receive server 3.144 (Ubuntu). Install mscc and mscc-ui together.
```
3. `.mscc-coord/status/ubuntu-stew.md`:
   - Header: Build/Last command id `cmd-072`, State `running`, Updated `2026-10-11`.
   - Add a cmd-072 row at the top of the ACK log: `Ubuntu = Windows cmd-068..071. recv 3.144 (Pi panadapter.c verbatim, dsputils.c Pi spectrum hunks, Ubuntu cal/WINDBG kept), Avalonia max-per-pixel, 3200 fixed, true-dB defaults + SPECTRUM_DB_LAW reset, S-meter dBm click. mscc 1.0.52 ui 0.6.74 ms-sdr <N>. Not committed. No rpi. No Solidus. No push.`
   - Fill in the smoke results once Stew has run it.

---

## Part D. Smoke (stew-HP, Stew with TG + step attenuator)
Install both debs, then start MSCC. Use a CW carrier a few hundred Hz off the tuning marker, away from VFO -12 kHz. A carrier between bins can read up to about 1.7 dB low, so nudge the VFO a few Hz for the highest reading.
1. **First start (reset):** the S/W window shows dB CAL 0.0, GRID MIN -155, and waterfall LOW -115 / HIGH -40 (Proficio; Geminus -105/-35). `~/.config/MSCC/mscc-avalonia.ini` has `SPECTRUM_DB_LAW=20`. Change something, restart: it is kept (no second reset).
2. **Peak steady:** S9 carrier, 1x zoom, tune slowly across it. The peak stays steady within 1-2 dB (before: 10-15 dB drops). 2x/4x the same.
3. **Selector gone:** there is no SPECTRUM RESOLUTION list in the S/W window. The log shows the pan resolution as Max (3200).
4. **dB CAL / step test:** TG at **-70 dBm**. Set dB CAL once so the peak reads -70 and note the trim (expect about 0). Without touching it: -30/-50/-70/-90/-110 dBm each read within **±2 dB**. Record the five readings and the floor.
5. **S-meter:**
   - Click the reading: it toggles S units ↔ "-NN dBm" once per click.
   - Restart MSCC: the mode is kept (`SMETER_DBM=1` in the ini). Toggle back to S and restart: S units again.
   - -76 dBm = **S9**, -77 = **S8**, -73 = S9, -63 = S9+10.
   - TX (TUNE): the meter behaves as before.
6. **LO point (VFO -12 kHz):** no hole and no spike in the trace or waterfall (allow about 2 s to settle). Key TUNE for 2 s and for 20 s: back on RX, any dip in the middle clears in **under 1 s**.
   - **Not a fail:** Ron's LO-point levelling has known pumping near the centre. Edge lines on the slice, a carrier parked there gets pushed down and leaves a slow hole, signals jump while fine-tuning within ±375 Hz. That work is paused; see `.mscc-coord/NOTE-FOR-RON-LO-POINT-LEVELLING-2026-10-11.md`. Note what you see.
7. **Smoothing:**
   - Stop MSCC, edit `~/.local/mscc/user_controls.ini` to `PANADAPTER_AVERAGE=9;` (ms-sdr writes it back, so only edit with MSCC stopped), start. The trace is clearly calmer, the frame rate unchanged, and levels unchanged (the -70 TG still reads -70 ±2).
   - The recv log shows `PAN SMOOTHING CHANGED → 11 (raw index 9, ...)`.
   - Set it back to `0;` (or leave the value Stew likes and note it). At 0 it looks like before.
8. **Version:** the title shows **MSCC: 0.6.74**, and recv reports 3.144 (recv log / version line; Core shows whatever ms-sdr reports).
9. **Remote:** Avalonia 0.6.74 connected to the Pi host (mscc 1.0.62, same recv spectrum code). It looks the same as local, and the same TG step reads within ±2 dB with the same dB CAL, if a TG is available there.

## What NOT to touch
`rpi/`, `Solidus/`, `installers/rpi/`, MSCC.Core, WPF, the Windows recv, `SDRcore-trans-linux`, `ms-sdr-linux` sources, the wire format (log/bias/150, `MAX_Y`), the server S-meter math (the Pi `48a04b6` average-power change stays out), Avalonia arm64 packaging, firmware.

## Report
- Part A identity checks (A1 `IDENTICAL`; the A2 diffs as described).
- Full source diffs and build warnings.
- recv/trans/ms-sdr versions, deb names and `ls installers/linux`.
- The CHANGELOG diff and smoke 1-9 (TG table: set level / spectrum reading / S-meter dBm; trim; floor).

STOP. No commit, no push.
