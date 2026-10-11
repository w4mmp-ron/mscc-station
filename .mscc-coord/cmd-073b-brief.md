# cmd-073b: Windows port of cmd-073: smooth S-meter needle (WPF) + average-power S-meter (recv 3.147)

Host: **windows-new-hp**. Repo: `C:\Users\n8vet\OneDrive\Documents\GitHub\mscc-station`, at `df84613` (clean, up to date; includes cmd-071 `32d6c3c`, cmd-072 `4d8a583`, cmd-073 `df84613`).
Scope:
- **WPF** (`mscc-ui/windows-work-tree/mscc-mscc/mscc-new/src/MSCC.Wpf/`): S-meter needle only.
- **Windows recv** (`mscc-ui/windows-work-tree/SDRcore-recv/sources/`): S-meter only.
- Versions, CHANGELOG, status.

**Do NOT edit `rpi/`, `Solidus/`, or `installers/rpi/`** (read only).
Don't touch: MSCC.Core, Avalonia, `linux/`, `SDRcore-trans`, `ms-sdr-MKII`, the spectrum code, the 0xD4 protocol, firmware.
Stew: "Lets not go too crazy with the code changes." Small changes, no refactors.
**STOP BEFORE COMMIT.** Show `git status`, `git diff --stat`, and the full diff of each source file.
Line numbers are from `df84613`; check them before you edit.
Reference: `.mscc-coord/cmd-073-brief.md` and `git show df84613` (the Linux/Avalonia version, smoke-passed on stew-HP). The recv change is Ron's Pi `48a04b6`.

---

## Part A. WPF: smooth S-meter needle

**Today:**
- `ViewModels/MainViewModel.cs:5850-5854`: `SMeter = Db_to_Smeter(dbm)` gives a whole S unit, and `_sMeter` is an `int` (line 52).
- `Controls/AnalogSMeterControl.xaml.cs`: `Value` is an `int` dependency property (lines 19-24, CLR wrapper 101-105). So the needle jumps one S unit (6 dB) at a time.
- **The power face doesn't use `Value`.** It's driven by `ForwardWatts` / `Swr` / `Fault` → `ApplyPowerSample` / `WattsToUnits`. `Value` is read only on the RX path: lines 232 and 349 `ApplyRawSample(Value, …)`, line 312 `OnValueChanged`, and line 408 `CurrentRawUnits` (RX branch). So changing `Value` to `double` doesn't affect the power face.
- **The meter scale** (`MaxSUnit = 15`, linear in angle; `UnitToAngle` 948) is 1..9 = S1..S9, then above S9 one unit = 10 dB (10 = +10 … 15 = +60).

### A1. `Controls/AnalogSMeterControl.xaml.cs`
1. **Lines 19-24:** register `Value` as `double`. **The default must be `0.0`.** A plain `0` (an int) makes WPF throw at startup: "Default value type does not match type of property".
   ```csharp
    public static readonly DependencyProperty ValueProperty =
        DependencyProperty.Register(
            nameof(Value),
            typeof(double),
            typeof(AnalogSMeterControl),
            new PropertyMetadata(0.0, OnValueChanged));
   ```
2. **Lines 101-105:**
   ```csharp
    /// <summary>RX needle in fractional S units 0–15 (cmd-073b). Not used by the power face.</summary>
    public double Value
    {
        get => (double)GetValue(ValueProperty);
        set => SetValue(ValueProperty, value);
    }
   ```
3. **Line 312** (`OnValueChanged`): `c.ApplyRawSample((double)e.NewValue, force: false);`
4. **Lines 444-446** (`ApplyRawSample`): change the signature to `private void ApplyRawSample(double raw, bool force)`; the body stays as it is (`Math.Clamp(raw, 0, MaxSUnit)`). Lines 232/349 `ApplyRawSample(Value, …)` then compile without change. Line 408 `Math.Clamp(Value, 0, MaxSUnit)` already works with a double.
5. **Line 901** (`UpdateDigitalReadout`, RX branch): replace `FormatSmeterReading((int)Math.Round(main))` with `FormatSmeterReading(ReadoutUnits(main))`:
   ```csharp
            ReadingText.Text = ShowDbm ? $"{Dbm} dBm" : FormatSmeterReading(ReadoutUnits(main));
   ```
   (`Math.Round` uses banker's rounding: 8.5 → 8 would show −76 as S8. The new rule rounds ties up, matching `Db_to_Smeter`.)
6. **After `FormatSmeterReading`** (ends about line 963), add the same helper as Avalonia:
   ```csharp

    /// <summary>
    /// cmd-073b: whole S unit for the readout from the fractional needle. Nearest unit, a tie
    /// goes up (same as Db_to_Smeter): −76…−73 = S9, −77 = S8; above S9 nearest 10 dB (−68 = S9+10).
    /// </summary>
    private static int ReadoutUnits(double main)
    {
        int u = (int)Math.Floor(main + 0.5);
        if (main > 0.0 && u < 1) u = 1;
        return Math.Clamp(u, 0, (int)MaxSUnit);
    }
   ```
Leave everything else alone: HOLD/Peak (`ApplyLevelSample`, ballistics, which already work in double), the power face, RESET, the dBm toggle and `UnitToAngle(int)`.

### A2. `ViewModels/MainViewModel.cs`
1. **Line 52:** `private int _sMeter;` → `private double _sMeter;` (keep `[ObservableProperty]`). Add a comment above it: `/// <summary>S-meter needle, fractional S units 0–15 (cmd-073b).</summary>`
2. **Keep `Db_to_Smeter` (432-470) as it is.** Add the needle mapping directly after it (after line 470, before the commented-out old version):
   ```csharp

    /// <summary>
    /// cmd-073b: smooth needle. dBm → fractional S units on the meter's own scale:
    /// 6 dB per unit up to S9 (−73 dBm), 10 dB per unit above (unit 10 = +10 … 15 = +60).
    /// The readout text still rounds to whole units (S9+10 style).
    /// </summary>
    private static double DbToSmeterNeedle(int dbm)
    {
        double units = dbm <= -73
            ? 9.0 + (dbm + 73) / 6.0
            : 9.0 + (dbm + 73) / 10.0;
        return Math.Clamp(units, 0.0, 15.0);
    }
   ```
3. **Lines 5850-5854** (`SmeterReported`):
   ```csharp
            SMeterDbm = dbm;
            SMeter = DbToSmeterNeedle(dbm); // cmd-073b: smooth needle (was Db_to_Smeter, whole S unit)
   ```
4. **Lines 2105 and 2114** (old PowerOut estimate): replace `(SMeter > 8 ? 12 : 0)` with `(Db_to_Smeter(SMeterDbm) > 8 ? 12 : 0)`. That keeps the exact old threshold (whole-unit S9) and keeps `Db_to_Smeter` in use. It still compiles with a double `SMeter`, but the threshold would otherwise move by about 3 dB.

### A3. `MainWindow.xaml:494`
`Value="{Binding SMeter}"` needs **no change** (double → double). Verify only.

---

## Part B. Windows recv: average-power S-meter (Ron's `48a04b6`, as on Ubuntu `df84613`) → 3.147
Windows `sdrcore.c` matches the Pi file before `48a04b6`. The only differences are two NFM comments, so the hunks apply exactly.

### B1. `sources/sdrcore.c`
1. Insert after line 27 (`/* NFM discriminator + de-emphasis … */`), before `static sp_float fm_prev_i` (line 28). This is where the Pi and Ubuntu put it:
   ```c
/* S-meter: average power of each block, smoothed in dB (fast rise, slow fall) */
#define METER_RISE_ALPHA 0.5f   // share of an upward step taken per block (about 2 blocks)
#define METER_FALL_SEC   0.3f   // fall time constant, seconds
#define METER_MIN_POWER  1.0e-20f // floor, avoids log10(0) on an all-zero block
static sp_float meter_db = 0.0f;
static int meter_started = 0;
   ```
2. **Line 100**, `sp_float peakmag = 0.0f;` → 
   ```c
        sp_float powaccum = 0.0f;
        sp_float blockpow;
        sp_float blockdb;
        sp_float blocksec;
   ```
3. **Lines 156-157:** replace
   ```c
                // capture peak magnitude for s-meter
                if (mag > peakmag) peakmag = mag;
   ```
   with
   ```c
                // sum the power for the s-meter (average, not peak: a peak reads noise high)
                powaccum += mag * mag;
   ```
4. **Line 255:** replace `mystate.peakRxSignalDbm = 20.0f * log10(peakmag) - 20.0f;` with
   ```c
        /* S-meter. Average power of this block in dB; a steady carrier reads the same as
         * the old peak did (same -20 constant). Smoothed in dB: up fast, down slowly. */
        blockpow = powaccum / ((sp_float) mystate.nfft - mystate.filtertaps);
        if (blockpow < METER_MIN_POWER) blockpow = METER_MIN_POWER;
        blockdb = 10.0f * log10(blockpow) - 20.0f;
        if (!meter_started) {
                meter_db = blockdb;
                meter_started = 1;
        } else if (blockdb > meter_db) {
                meter_db += (blockdb - meter_db) * METER_RISE_ALPHA;
        } else {
                blocksec = ((sp_float) mystate.nfft - mystate.filtertaps) / mystate.samplerate;
                meter_db += (blockdb - meter_db) * (blocksec / (METER_FALL_SEC + blocksec));
        }
        mystate.peakRxSignalDbm = meter_db;
   ```
   If MSVC warns C4244 (double→float) on the `log10` line, `(sp_float) log10(blockpow)` is the only allowed deviation.

### B2. `sources/udp_thread.c:453`
`i_smeter = (int16_t) f_smeter;` → `i_smeter = (int16_t) floorf(f_smeter + 0.5f); // nearest dB (a plain cast rounds toward zero)`
(`<math.h>` comes in via `extern.h`.)

### B3. `sources/extern.h:40`
`#define VERSION_MINOR 146` → `147` (recv **3.147**).

Check: `git diff --no-index --ignore-cr-at-eol rpi/SDRcore-recv-linux/sources/sdrcore.c mscc-ui/windows-work-tree/SDRcore-recv/sources/sdrcore.c` shows only the two Windows NFM comments (and the cast, if added).

---

## Part C. Build, versions, CHANGELOG
1. Stop MSCC and its servers.
2. **recv:** build `SDRcore-recv/SDRcore.vcxproj` (via `sdrcore-recv.sln`), **MFC|Win32**, v143. The output is `mscc-ui/Release/windows-wpf/mscc-recv.exe`. No new warnings in `sdrcore.c`/`udp_thread.c`. Copy it to `C:\mscc-net9\mscc-recv.exe`.
3. **WPF:** `dotnet build MSCC.sln -c Release` from `mscc-ui/windows-work-tree/mscc-mscc/mscc-new`. `ClientVersion.txt` 10.11.2 → **10.11.3** (same day; use what the build prints), and it copies to `C:\mscc-net9`.
4. Check:
   - `C:\mscc-net9\mscc-recv.exe` matches the Release copy (same hash) and has today's time.
   - `MSCC.Wpf.exe` ProductVersion is 10.11.3.
   - trans 3.146 and ms-sdr 3.182 are unchanged (Oct 8).
   - The recv log reports 3.147 on first start.
5. `installers/windows/CHANGELOG.md`: add a **new section above R10.11.2** (released). Use the same heading style:
   ```
   ## R10.11.3 - 2026-10-11  (`mscc-net9-R10-11-3-install.exe`, title bar 10.11.3)

   ### Changed
   - S-meter: the needle now moves smoothly in fractions of an S unit (6 dB per unit up to S9, 10 dB per unit above) instead of jumping a whole S unit. The digital readout still shows whole units (S9, S9+10) or dBm.
   - S-meter: the receive server measures average power instead of the block peak, smoothed (fast rise, 0.3 s fall) and rounded to the nearest dB. Noise reads steadier and a little lower; a steady carrier reads the same.
   - Receive server 3.147 (Windows).
   ```
6. Update `.mscc-coord/status/windows-new-hp.md` (cmd-073b row: recv 3.147, client 10.11.3, not committed).
7. Packaging is **not** part of this command. Stew builds `mscc-net9-R10-11-3-install.exe` from `C:\mscc-net9` after the smoke test.

---

## Part D. Smoke (NEW-HP; Stew, TG + 2.5 dB step attenuator; same as Linux cmd-073)
1. **−73 dBm:** the needle is on S9 and the text shows S9.
2. **−70 dBm:** the needle is about 0.3 of the way from S9 toward +10, and the text shows S9.
3. **−76 dBm:** the needle is halfway between S8 and S9, and the text shows **S9**.
4. **−77 dBm:** the text shows **S8**.
5. **−63 dBm:** the needle is on +10, and the text shows S9+10.
6. **Steps:** 2.5 dB attenuator steps give small, even needle moves (about 0.4 unit below S9, 0.25 above) and no whole-unit jumps.
7. **Noise and carrier:** with no signal, the noise floor reading is steadier (a little lower than before is expected). A steady carrier reads the same dBm as with recv 3.146 (dBm toggle).
8. **Controls:** HOLD (slow fall) and Peak needle behave as before. The click toggles S ↔ dBm and is still remembered across a restart.
9. **TX:** the TX power face, SWR reading and RESET on a latched fault are unchanged. Back on RX the needle resumes smoothly.
10. **Spectrum:** unaffected (TG −70 still reads −70 ±2), and cmd-068..071 behaviour is unchanged.
11. **Title:** shows 10.11.3.

## Report
Diffs, build warnings, the `C:\mscc-net9` file times/versions/hashes, and smoke 1-11.

STOP. No commit, no push.
