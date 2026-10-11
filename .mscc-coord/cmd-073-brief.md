# cmd-073: Ubuntu: smooth S-meter needle (Avalonia 0.6.75) + Pi average-power S-meter (recv 3.145), mscc 1.0.53

Host: **stew-HP** (Ubuntu, x86_64). Repo: `/home/stew/Documents/GitHub/mscc-station`, at `4d8a583` (cmd-072, clean, 1 commit ahead of origin until Stew pushes).
Scope:
- **Avalonia** (`mscc-ui/Avalonia-Migration/src/MSCC.Avalonia/`): needle only.
- **Ubuntu recv** (`linux/SDRcore-recv-linux/sources/`): S-meter only.
- Versions, the two amd64 debs, CHANGELOG, status.

**Do NOT edit `rpi/`, `Solidus/`, or `installers/rpi/`** (Ron's; read `rpi/` only).
Don't touch: WPF, the Windows recv, MSCC.Core, `SDRcore-trans-linux`, `ms-sdr-linux` sources (only the auto-bumped `version.h`), the spectrum code, the 0xD4 protocol, firmware. Avalonia arm64/Pi packaging is out of scope.
Stew: "Lets not go too crazy with the code changes." Small changes, no refactors.
**STOP BEFORE COMMIT.** Show `git status`, `git diff --stat`, and the full diff of each source file.
Line numbers are from `4d8a583`. Check them before you edit.

---

## Part A. Avalonia: smooth S-meter needle (0.6.75)

**Today:**
- `ViewModels/MainViewModel.cs:1709-1715` `ApplySmeterSample` runs `DbToSmeter(dbm)` (1680-1700), which returns a whole S unit, and puts it into `SMeter`.
  - `SMeter` is already a `double` (line 319).
  - `AnalogSMeterControl.Value` is already a `double` (`StyledProperty<double>`, `Controls/AnalogSMeterControl.axaml.cs:24`).
- So the needle jumps 6 dB (one S unit) at a time.

**Meter scale** (`AnalogSMeterControl.axaml.cs`, `BuildFace` tick table and `UnitToAngle`):
- Units 0..15, linear in angle (`MaxSUnit = 15`).
- 1..9 = S1..S9.
- **Above S9, one unit = 10 dB:** unit 10 = +10, 12 = +30, 15 = +60 (labels +10/+30/+60).

**The needle's S value is therefore:**
- S-units = 9 + (dBm + 73) / 6 at or below S9 (S9 = -73 dBm).
- 9 + (dBm + 73) / 10 above S9.
- Clamped 0..15.
- Examples: -73 = 9.0; -76 = 8.5; -70 = **9.3**; -63 = 10.0; -43 = 12.0.

HOLD (fall 0.12 units / 30 ms) and Peak (`ApplyRawSample` / `BallisticsTick`) already work in doubles: no change.

### A1. `ViewModels/MainViewModel.cs`: double S value for the needle
Directly after `DbToSmeter` (after line 1700), add:
```csharp
    /// <summary>
    /// cmd-073: smooth needle. dBm → fractional S units on the meter's own scale:
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
In `ApplySmeterSample` (1709-1715), replace `SMeter = units;` with:
```csharp
        SMeter = DbToSmeterNeedle(dbm); // cmd-073: smooth needle (was the whole S unit)
```
Keep `int units = DbToSmeter(dbm);` and `SmeterText = FormatSmeterReading(units);` as they are. Change the `SMeter` summary at line 318 to `/// <summary>S-meter needle, fractional S units 0–15 (cmd-073). Drives analog face.</summary>`.

### A2. `Controls/AnalogSMeterControl.axaml.cs`: whole-unit readout from the double
Line 518 uses `(int)Math.Round(main)`. With a fractional needle, `Math.Round` (banker's rounding) gives the wrong text at x.5, and above S9 it must round the 10 dB units. Replace line 518 with:
```csharp
            ReadingText.Text = ShowDbm ? $"{Dbm} dBm" : FormatSmeterReading(ReadoutUnits(main));
```
and add next to `FormatSmeterReading` (529):
```csharp
    /// <summary>
    /// cmd-073: whole S unit for the readout from the fractional needle. Nearest unit, a tie
    /// goes up (same as DbToSmeter): −76…−73 = S9, −77 = S8; above S9 nearest 10 dB (−68 = S9+10).
    /// </summary>
    private static int ReadoutUnits(double main)
    {
        int u = (int)Math.Floor(main + 0.5);
        if (main > 0.0 && u < 1) u = 1;
        return Math.Clamp(u, 0, (int)MaxSUnit);
    }
```
The readout still follows the displayed needle, so HOLD acts on the text as it does today. The dBm toggle (cmd-072) is unchanged.

Small differences from `DbToSmeter`, all acceptable; note them in the report:
- The old int path capped the text at S9+30 (unit 12, a Kenwood CAT "firmware limit" convention). Text and needle now go up to S9+60, matching the face.
- At or below about -127 dBm the needle rests at 0, and the text shows S0 only when the needle is at 0.

### A3. Version 0.6.74 → 0.6.75
- `MSCC.Avalonia.csproj:8`: `<Version>0.6.75</Version>`.
- `ViewModels/MainViewModel.cs:498`: `_clientVersionText = "0.6.75"`.
- `ViewModels/MainViewModel.cs:188`: `AppendLog("MSCC Avalonia 0.6.75 — smooth S-meter needle.");`.
- `mscc-ui/Avalonia-Migration/packaging/mscc-ui/DEBIAN/control:2`: `Version: 0.6.75`.
- Check: `grep -rn "0\.6\.74" mscc-ui/Avalonia-Migration --include=*.cs --include=*.csproj --include=*.axaml --include=control` returns nothing.

---

## Part B. Ubuntu recv: Pi average-power S-meter (Ron `48a04b6`) → 3.145

**What was checked on stew-HP:**
- `linux/.../sdrcore.c` is **byte-identical** to the Pi file before the change (`git show 48a04b6^:rpi/SDRcore-recv-linux/sources/sdrcore.c`).
- The Pi `sdrcore.c` at HEAD equals `48a04b6` (no later Pi change). So **copy the Pi file verbatim**.
- `linux/.../udp_thread.c` differs from the Pi's pre-change file in Ubuntu/Pi-only calibration code (float `magnitude_*` on the Pi). So **don't copy it; apply the one-line hunk.** The `Send_smeter_thread` lines (Ubuntu 455-456) match the Pi's pre-change lines exactly.
- `floorf`/`log10` come from `<math.h>` via `extern.h:23`. `mystate.nfft`, `filtertaps` and `samplerate` are in `sdrcore.h` (87, 88, 93).

**What it does (Ron, Pi mscc 1.0.58):**
- The S-meter reports each ~21 ms block's **average power** (`10*log10(powaccum/N) - 20`), not the block's peak sample (`20*log10(peakmag) - 20`).
- It is smoothed in dB: fast rise (`METER_RISE_ALPHA 0.5`, about 2 blocks), 0.3 s fall (`METER_FALL_SEC`), with `METER_MIN_POWER 1e-20` so an all-zero block doesn't hit log10(0).
- `udp_thread.c` then rounds to the nearest dB instead of truncating toward zero.
- **Effect:**
  - A steady carrier reads the same: its complex magnitude is constant, so mean power = peak² and the -20 constant is unchanged.
  - Noise reads lower and steadier. Ron said "slightly"; the expectation is up to about one S unit, depending on the block peak-to-average. Ron on the Pi: "steadier, floor slightly lower".

### B1. `sdrcore.c`: verbatim from the Pi
```
cp rpi/SDRcore-recv-linux/sources/sdrcore.c linux/SDRcore-recv-linux/sources/sdrcore.c
cmp rpi/SDRcore-recv-linux/sources/sdrcore.c linux/SDRcore-recv-linux/sources/sdrcore.c && echo IDENTICAL
git diff --stat linux/SDRcore-recv-linux/sources/sdrcore.c     # 28 insertions, 3 deletions
```
`git diff` must equal the `48a04b6` sdrcore.c hunks (`git show 48a04b6 -- rpi/SDRcore-recv-linux/sources/sdrcore.c`), namely:
- the `METER_*` defines plus the `meter_db` / `meter_started` statics after line 27;
- `peakmag` → `powaccum`, `blockpow`, `blockdb`, `blocksec` (line 100);
- `powaccum += mag * mag;` instead of the peak capture (157);
- the averaged/smoothed `mystate.peakRxSignalDbm = meter_db;` block instead of line 253.

If the copy shows anything else, stop and report.

### B2. `udp_thread.c`: one line (456)
Replace
```c
        i_smeter = (int16_t) f_smeter;
```
with
```c
        i_smeter = (int16_t) floorf(f_smeter + 0.5f); // nearest dB (a plain cast rounds toward zero)
```
Nothing else in that file.

### B3. Version and build
1. `linux/SDRcore-recv-linux/sources/extern.h:41`: `VERSION_MINOR 144` → `145` (recv **3.145**).
2. `linux/mscc-deb/packaging/DEBIAN/control:2`: `Version: 1.0.52` → `1.0.53`.
3. Stop MSCC. Run `./linux-build/build-mscc-deb-amd64.sh`.
   - ms-sdr `version.h` auto-bumps from 3.179 to 3.180. Report the final number and **don't rebuild afterwards**: each build bumps it again.
   - No new gcc warnings in `sdrcore.c` / `udp_thread.c`.
   - Report recv 3.145, trans 3.142 (unchanged) and ms-sdr.
4. Avalonia: `rm -rf mscc-ui/Avalonia-Migration/publish/linux-x64-sc && ./linux-build/mscc-ui-x64.sh && ./linux-build/build-mscc-ui-deb-amd64.sh`. No new warnings in the changed files.
5. `ls installers/linux`: only `mscc_1.0.53_amd64.deb` and `mscc-ui_0.6.75_amd64.deb`; 1.0.52 and 0.6.74 removed by `drop-installers.sh`. Run `dpkg-deb -I` on both (versions correct).
6. Install for the smoke test: `sudo apt install ./installers/linux/mscc_1.0.53_amd64.deb ./installers/linux/mscc-ui_0.6.75_amd64.deb`.

---

## Part C. CHANGELOG, status
1. `installers/linux/CHANGELOG.md`:
   - "Currently shipped" table: mscc **1.0.53**, mscc-ui **0.6.75**.
   - Mark the 1.0.52 heading `(superseded)`.
   - Add above it:
```
## mscc 1.0.53 / mscc-ui 0.6.75 — 2026-10-11

### Changed
- S-meter needle moves smoothly with the signal (it used to jump one S unit, 6 dB, at a time). The reading still shows whole S units (S9+10), or dBm when clicked.
- S-meter reads the average signal power instead of the peak: noise reads steadier and about one S unit lower; a steady carrier reads the same. Rounded to the nearest dB.
- Receive server 3.145 (Ubuntu). Install mscc and mscc-ui together.
```
2. `.mscc-coord/status/ubuntu-stew.md`:
   - Header: Build/Last command id `cmd-073`, State `running`, Updated `2026-10-11`.
   - Set the cmd-072 row to `done` (commit 4d8a583) if Stew confirms the cmd-072 smoke passed; otherwise leave it.
   - New top ACK row: `Avalonia 0.6.75 smooth S-meter needle (fractional S units, 6 dB/unit to S9, 10 dB/unit above; readout whole units). recv 3.145 Pi 48a04b6 average-power S-meter (sdrcore.c Pi verbatim, udp_thread.c rounding). mscc 1.0.53 ms-sdr <N>. Not committed. No rpi. No Solidus. No push.`

---

## Part D. Smoke (stew-HP, Stew with TG + step attenuator)
CW carrier inside the RX filter. HOLD off for steps 1-3 (so the needle follows directly).
1. **-73 dBm:** the needle sits exactly on the **S9** tick; readout S9 / "-73 dBm".
2. **-70 dBm:** the needle is **0.3 of the way from S9 to +10** (above S9 one unit is 10 dB on this face, so +3 dB = 0.3 unit). Readout S9 / "-70 dBm" (±1).
   - -76 dBm: the needle is **half way between S8 and S9**; readout S9.
   - -77 dBm: readout S8.
   - -63 dBm: needle on +10, readout S9+10.
3. **1 dB steps** from -80 to -66: the needle moves a small, even step each time (about 1/6 unit below S9 and 1/10 above); no 6 dB jumps.
4. **Noise floor** (antenna or dummy load, no TG): steadier than 1.0.52 and lower (expect up to about one S unit). Note the dBm before and after if you can.
5. **Steady carrier unchanged:** TG at -70 dBm reads -70 ±1 in dBm mode; -30/-50/-90/-110 each within ±1-2 of what 1.0.52 read (spot check).
6. **HOLD / Peak:** HOLD on: the needle falls slowly and smoothly after a signal drops. Peak on: the orange needle holds ~2 s, then falls. Both as before, just without steps.
7. **dBm toggle:** a click switches S ↔ dBm and is kept after a restart (cmd-072).
8. **TX:** key TUNE; the meter behaves as before.
9. **Spectrum unaffected:** the TG at -70 still reads -70 ±2 on the spectrum. The LO point and smoothing are as in cmd-072.
10. The title shows **MSCC: 0.6.75**; recv 3.145.
11. **Remote to the Pi (1.0.62, which already has the average-power S-meter):** the needle is smooth there too, and the floor is similar to local.

## Notes for cmd-073b (NEW-HP, WPF + Windows recv; not part of this command)
- **WPF needle** (`mscc-ui/windows-work-tree/mscc-mscc/mscc-new/src/MSCC.Wpf/`):
  - `ViewModels/MainViewModel.cs:432` `Db_to_Smeter(int)` (int); `:52` `private int _sMeter;`; `:5852` `SMeter = Db_to_Smeter(dbm);`.
  - `MainWindow.xaml:494` `Value="{Binding SMeter}"`.
  - `Controls/AnalogSMeterControl.xaml.cs`:
    - `ValueProperty` is registered as **`typeof(int)`** (lines 19-22) and needs to become `double`, or a separate double property.
    - `UnitToAngle(double)` is at 948 (an int overload at 955), `MaxSUnit 15` at 176, and the S-mode clamp at 408 / 446.
  - The digital readout uses `(int)Math.Round(main)` (`UpdateDigitalReadout`, line 901). Use the same `ReadoutUnits` floor(+0.5) rule as A2.
  - Check the WPF face's above-S9 mapping (expected 10 dB per unit, as in Avalonia) and the PowerMode (FWD/SWR) path, which shares `Value`.
- **Windows recv** (`mscc-ui/windows-work-tree/SDRcore-recv/sources/`, now 3.146):
  - `sdrcore.c` equals the Pi's pre-`48a04b6` file except two extra NFM comment lines (~185/191). So apply the `48a04b6` hunks rather than copying: statics after line 27, `peakmag` at 100, peak capture at 157, `peakRxSignalDbm` at 255.
  - `udp_thread.c:453` `i_smeter = (int16_t) f_smeter;` → `floorf(f_smeter + 0.5f)`.
  - recv 3.146 → 3.147.

## What NOT to touch
`rpi/`, `Solidus/`, `installers/rpi/`, WPF, the Windows recv, MSCC.Core, `SDRcore-trans-linux`, `ms-sdr-linux` sources, the spectrum code (`panadapter.c`, `dsputils.c`, Avalonia spectrum files), the 0xD4 wire format, Avalonia arm64 packaging, firmware.

## Report
- The B1 `cmp` (IDENTICAL) and its diff.
- The B2 diff and the Part A diffs.
- Build warnings; recv/trans/ms-sdr versions; deb names and `ls installers/linux`.
- The CHANGELOG diff; smoke 1-11 (table: TG dBm / readout / needle position; floor before/after).

STOP. No commit, no push.
