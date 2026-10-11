# cmd-069: spectrum dB slope 1:1 (true dB) — Windows recv averaging fix + client RawYToDb; one-time dB CAL / view reset

Host: **windows-new-hp**. Repo: `C:\Users\n8vet\OneDrive\Documents\GitHub\mscc-station`.
**cmd-068 is built and smoke-passed but NOT committed. Leave its changes in place** (SpectrumDisplayControl.xaml.cs, SpectrumWaterfallSettings.cs, PanadapterControlsWindow.xaml, ClientVersion.txt, CHANGELOG). cmd-069 goes on top of them in the same uncommitted tree.
**Do NOT touch `rpi/`, `Solidus/`, or `installers/rpi/`** (the rpi copies below are reference only). Stew: "Lets not go too crazy with the code changes." Keep each change small, with no refactors.
**STOP BEFORE COMMIT.** Show `git status` and `git diff --stat`, plus the full diff for each file.
Line numbers are from `b9df610` **plus the uncommitted cmd-068 edits** (2026-10-09). Check them before you edit.

## Background (read-only findings)
Stew's TG test: dB CAL set at one level, then adding or removing attenuation reads off by more than 10 dB, and the error grows the further you go from the cal point. That is a slope error. There are two causes on Windows:
1. **Server, Windows recv only: frame averaging has a 1.5x gain.** `SDRcore-recv/sources/panadapter.c:200-213` adds all G_Smoothing history frames **plus** the current frame, then divides by G_Smoothing. With smoothing 2 (PANADAPTER_AVERAGE 0) that is (Y+Y+Y)/2 = 1.5·Y. Y carries the +40·150 bias, so the trace reads 15·log10|X| + 20 instead of 10·log10|X|.
   The sum is also uint16 (it wraps at smoothing 4), and smoothing above 4 indexes past the 4-frame history. **The Pi and Ubuntu recv already have the fix** (`rpi/SDRcore-recv-linux/sources/panadapter.c`; `linux/SDRcore-recv-linux/sources/panadapter.c` is identical to it).
2. **Client, all servers: half-dB scale.** The server packs Y = (10·log10|X| + 40)·150, where |X| is an FFT **magnitude** (`dsputils.c:226-229`). The client inverts that exactly (`UdpRadioService.cs` `RawYToDb`, Y/150 − 40), so the trace is in half-dB: a real 20 dB step moves it 10 dB. One dB CAL offset can only be right at one level.
   - `(10·log10|X| + 40)·150` is the same number as `(20·log10|X| + 80)·75`, so the wire data is fine and only the client reading changes.
   - **This is the fix Ron proposed** (`.mscc-coord/NOTE-FOR-STEW-SPECTRUM-DB-SCALE-2026-10-07.md`) and **already tried in his private test build** (rpi/CLAUDE.md, 2026-10-09: scale 75, bias 80, clamp 140). Pi result: the trace moved 25 dB between the generator's S9 and S4 settings (about 12 on the old client), the noise floor stayed put, and the trim moved everything dB for dB.
   - His trial centre of −86.9 read the S9 carrier about 12 dB low (S9 at about −85). Ron: "a centre near −75 to −80 would put S9 at −73." The S-meter needle can't settle it any closer (it shows S9 anywhere from −78 to −73).
   - Ron's view is that the spectrum should mirror the S-meter with one fixed constant. This brief sets the centre to **−75** and asks Stew for one TG calibration to confirm it.

After both fixes, Windows Y is the same as the Pi's (same DSP since cmd-064; at 3200 the Pi's max-over-bins changes nothing), so **one client change gives true dB against the Windows, Pi and Ubuntu servers**. No Pi/Ubuntu server change.

## Part 1. Windows SDRcore-recv (`mscc-ui/windows-work-tree/SDRcore-recv/sources/`)

### 1a. `panadapter.c`: averaging gain 1x (port from the Pi)
1. At the top of `Panadapter_thread`, after `int segments = 2;` (line 67), add:
   ```c
    int smoothing = 2;
   ```
2. After `if (segments > MAX_PAN_SEGMENTS) segments = MAX_PAN_SEGMENTS;` (line 177), add:
   ```c

            /* History holds 4 frames per segment: keep smoothing in 1..4. */
            smoothing = G_Smoothing;
            if (smoothing < 1) smoothing = 1;
            if (smoothing > 4) smoothing = 4;
   ```
3. Replace lines 200-213, from the first `for (i = 0; i < MAX_X; i++) {` that zeroes `output_buffer` through the closing `}` of the loop that divides by `G_Smoothing`:
   ```c
                for (i = 0; i < MAX_X; i++) {
                    panadaper_average[sequence].avg_buffer_output.output_buffer[i] = 0;
                }
                for (average_count = 0; average_count < G_Smoothing; average_count++) {
                    ...
                }
                for (i = 0; i < MAX_X; i++) {
                    panadaper_average[sequence].avg_buffer_output.output_buffer[i] =
                        (panadaper_average[sequence].avg_buffer_output.output_buffer[i] +
                            panbuffer_temp.Y[i]) / G_Smoothing;
                }
   ```
   with (exact Pi code):
   ```c
                /*
                 * Average the current frame with the (smoothing - 1) newest history frames.
                 * History [smoothing-1] is the newest. Sum in 32 bits: up to 4 x MAX_Y
                 * overflowed the uint16 output and the old sum used one frame too many.
                 */
                for (i = 0; i < MAX_X; i++) {
                    uint32_t sum = panbuffer_temp.Y[i];
                    for (average_count = 1; average_count < smoothing; average_count++)
                        sum += panadaper_average[sequence].buffer_input[average_count].avg_buffer_input[i];
                    sum /= (uint32_t)smoothing;
                    if (sum > MAX_Y)
                        sum = MAX_Y;
                    panadaper_average[sequence].avg_buffer_output.output_buffer[i] = (uint16_t)sum;
                }
   ```
4. Line 242, the history shift: `for (average_count = 0; average_count < (G_Smoothing - 1); average_count++) {` becomes
   `for (average_count = 0; average_count < (smoothing - 1); average_count++) {`. Leave the `memcpy` lines as they are.
   (The following `memcpy(... buffer_input[average_count] ..., panbuffer_temp.Y, ...)` now stores the newest frame at `[smoothing-1]`, as on the Pi.)
5. Check: `git diff --no-index mscc-ui/windows-work-tree/SDRcore-recv/sources/panadapter.c rpi/SDRcore-recv-linux/sources/panadapter.c` must be empty apart from CRLF (`git diff --no-index --ignore-cr-at-eol` or `-w`). Read only; do not write rpi/.

### 1b. `dsputils.c` `doPanadapter`: max over bins (port from the Pi, chosen: ~15 lines)
It is harmless at 3200 (0.96 bins per point, so one bin each). It keeps parity with the Pi, and it fixes 800/1600 for clients that still ask for them (Avalonia and older WPF on a Windows host).
1. Replace lines 207-213:
   ```c
        for (j = 0; j < pixels; j++) {
                // Guard FFT index (fstep can nudge bpos to nfft)
                if (bpos < 0) bpos = 0;
                if (bpos >= mystate.nfft) bpos = mystate.nfft - 1;

                // calculate magnitude of bin
                mag = sqrt((dbuf[bpos].real * dbuf[bpos].real) + (dbuf[bpos].imag * dbuf[bpos].imag));
   ```
   with (exact Pi code):
   ```c
        for (j = 0; j < pixels; j++) {
                int bend;
                sp_float m2, best = 0.0f;

                // Guard FFT index (fstep can nudge bpos to nfft)
                if (bpos < 0) bpos = 0;
                if (bpos >= mystate.nfft) bpos = mystate.nfft - 1;

                // Largest bin of ALL the FFT bins this display point covers
                // (3.84 bins at 800 points, 1.92 at 1600, 1 at 3200). Taking only
                // the first bin made a steady carrier vanish whenever it sat
                // between two sampled bins (signal generator test, 2026-10-05).
                fpos += fstep;
                bend = (int) fpos;
                if (bend <= bpos) bend = bpos + 1;
                if (bend > mystate.nfft) bend = mystate.nfft;
                for (k = bpos; k < bend; k++) {
                        m2 = (dbuf[k].real * dbuf[k].real) + (dbuf[k].imag * dbuf[k].imag);
                        if (m2 > best) best = m2;
                }
                mag = sqrt(best);
   ```
2. Near the end of the same loop (lines 243-244), delete the `fpos += fstep;` line. Keep `bpos = (int) fpos;`. `fpos` now advances at the top of the loop.
3. Leave the log conversion (`10.0f * log10(mag)`, `+40`, `*150`), MAX_Y and maxmag **unchanged**: the wire format stays the same for every client.
   Optional: comment-only edits to the "10*log10 ... bias 40" lines saying the client reads it as `(20*log10|X| + 80) * 75`.
4. `k` is already declared (`int j, k, bpos = 0;`, line 120).

**Not in this command:** the Pi's S-meter average-power change (`rpi` sdrcore.c, mscc 1.0.58). Later option; the Windows S-meter stays as it is (peak).

### 1c. Version
`extern.h:40` `#define VERSION_MINOR 144` → `145` (recv 3.145). `include/extern.h` has no version; leave it.

## Part 2. WPF client (`mscc-ui/windows-work-tree/mscc-mscc/mscc-new/src/`)

### 2a. `MSCC.Core/Services/UdpRadioService.cs` `RawYToDb` (lines 2412-2428)
Replace the summary and method with:
```csharp
    /// <summary>
    /// Invert sdrcore panadapter Y → relative dB (true dB, cmd-069).
    /// Server (dsputils.c): Y = max(0, (10*log10(|FFT|) + 40) * 150), then MAX_Y (16000).
    /// |FFT| is a magnitude, so 10*log10 is half-dB. The same Y equals (20*log10(|FFT|) + 80) * 75:
    /// reading it with scale 75 / bias 80 gives 20*log10(|FFT|), so a 20 dB step moves the trace 20 dB.
    /// Wire data unchanged: works with every sdrcore-recv (Windows, Pi, Ubuntu).
    /// Client SPECTRUM_DB_OFFSET (dB CAL) then sets absolute labels.
    /// </summary>
    private static float RawYToDb(ushort y)
    {
        const float scale = 75f;
        const float bias = 80f;
        float relDb = (y / scale) - bias;
        if (relDb < -200f) relDb = -200f;
        if (relDb > 140f) relDb = 140f;   // Y 16000 → +133
        return relDb;
    }
```
Its only caller is `FlushPanFrame` (line 2383). The `sMeter` fill at 2393 (`(peak + 140f) / 10f`) is unused for the face S-meter (comment at 2391); leave it.
**Note: MSCC.Core is also referenced by Avalonia** (`MSCC.Avalonia.csproj:35`); see "Avalonia" below.

### 2b. `MSCC.Wpf/Controls/SpectrumColorSettings.cs`: new centre and view defaults
How the defaults were picked: for the same signal, new display dB = 2·(old display dB − old offset) + new offset.
With the old offset −91.3 and the new −75, a threshold T maps to about 2·T + 108. Ron's Pi floor (about −118 old / −143 at −86.9) lands near **−131** with −75.
- Line 61: `public const float SpectrumDbCalCenter = -75f;`
  - Summary line 60: `/// <summary>Centre cal (slider "0") for the true-dB scale (cmd-069): Pi test S9 carrier ≈ −73 dBm. ~−75 absolute.</summary>`
  - Comments at 55 (−91.3 → −75) and 66-67 (`// −95`, `// −55`).
- Line 87: `SpectrumGridMin { get; private set; } = -145f;` (line 84 summary: floor about −131, min −145).
- Line 96: `minDb = maxDb - 125f; // e.g. −20 → −145`.
- Lines 202-206: `WaterfallHighDb = -30f; WaterfallLowDb = -140f; WaterfallHighDefault = -30f; WaterfallLowDefault = -140f;` and the summary at about line 200 (Low −140, High −30).
- Grid max stays −20 (S9+40 = −33 fits).

### 2c. `MSCC.Wpf/SpectrumWaterfallSettings.cs`: defaults + one-time reset of old-scale settings
1. Line 37: `SpectrumGridMin { get; set; } = -145f;`. Update the comment at line 29 (−91.3 → −75).
2. Ship constants, lines 146-160:
   ```csharp
    public const float ShipHfHighDb = -30f;   // cmd-069 true-dB scale (was −44 on the half scale)
    public const float ShipHfLowDb = -105f;   // was −106
    ...
    public const float ShipHfGridMin = -145f; // was −125
    ...
    public const float ShipLfHighDb = -25f;   // was −40
    public const float ShipLfLowDb = -95f;    // was −100
    ...
    public const float ShipLfGridMin = -145f; // was −125
   ```
   Leave Gain, Zero, Baseline and GridMax as they are.
3. Load repair, lines 709-717: change the three `-125f` to `-145f` (keep the `> -120f` test as it is).
4. **One-time reset** (old dB CAL, grid floor and waterfall levels were set for the half scale and would now be far off).
   Insert after `if (LastLfFreq <= 0) LastLfFreq = DefaultLastLfFreq;` (line 958) and before `LoadWaterfallBankToLive(RadioModelIsGeminus);` (line 961):
   ```csharp

                // cmd-069: spectrum is now true dB (was half-dB). dB CAL, grid floor and waterfall
                // levels saved for the old scale are reset once to the new defaults.
                if (!fileLines.Any(l => LineMatchesKey(l, "SPECTRUM_DB_LAW")))
                {
                    SpectrumDbOffset = SpectrumColorSettings.SpectrumDbCalCenter;
                    SpectrumHfDbOffset = SpectrumColorSettings.SpectrumDbCalCenter;
                    SpectrumLfDbOffset = SpectrumColorSettings.SpectrumDbCalCenter;
                    SpectrumGridMin = ShipHfGridMin;
                    SpectrumHfGridMin = ShipHfGridMin;
                    SpectrumLfGridMin = ShipLfGridMin;
                    WaterfallHighDb = SpectrumColorSettings.WaterfallHighDefault;
                    WaterfallLowDb = SpectrumColorSettings.WaterfallLowDefault;
                    WaterfallHfHighDb = ShipHfHighDb;
                    WaterfallHfLowDb = ShipHfLowDb;
                    WaterfallLfHighDb = ShipLfHighDb;
                    WaterfallLfLowDb = ShipLfLowDb;
                }
   ```
5. `Save()`: right after the `UpdateOrAdd(lines, "SPECTRUM_DB_OFFSET", ...)` statement (about line 1026-1027) add:
   `UpdateOrAdd(lines, "SPECTRUM_DB_LAW", "20");`
   `Load` already calls `Save()` at the end, so the marker is written on the first run, and a brand-new ini gets it with the new defaults (no reset needed).
   Keep the existing key names (SPECTRUM_DB_OFFSET, _HF, _LF, GRID_*, WATERFALL_*). Ron's private build used other key names; that build isn't in the repo, so don't copy them.

### 2d. Text only
- `PanadapterControlsWindow.xaml:101` and `:108` tooltips: "−91.3" → "−75".
- `:112` tooltip: "Defaults −20…−145 so HF grass (~−131) fills the lower pane."
- `:231` tooltip: "Default −30."
- `PanadapterControlsWindow.xaml.cs:92` and `:477` comments: −91.3 → −75.
- `MainViewModel.cs:6257` comment: "Grass ~−131 needs min ≈ −145."

## Part 3. Versions, CHANGELOG, build
1. **recv 3.145** (1c). trans and ms-sdr are not rebuilt (trans 3.146, ms-sdr 3.182).
2. **Client:** don't hand-edit `ClientVersion.txt` (10.9.0 from cmd-068). The build bumps it, the same day goes to **10.9.1**. Use the number the build prints.
3. **`installers/windows/CHANGELOG.md`:** keep **one** section for cmd-068 + cmd-069 under the final version. Rename the existing `## R10.9.0 — 2026-10-09 (...R10-9-0..., title bar 10.9.0)` heading to the final number (e.g. `## R10.9.1 — 2026-10-09  (\`mscc-net9-R10-9-1-install.exe\`, title bar 10.9.1)`). Keep its bullets and add:
   - Under **Fixed**: `- Spectrum dB scale now matches real dB: a 20 dB change in signal moves the trace 20 dB (it moved only about 10 to 15 dB before), so one dB CAL holds at every level. The receive server's spectrum averaging no longer adds gain.`
   - Under **Changed**: `- On first start, dB CAL, the spectrum GRID MIN and the waterfall LOW/HIGH are reset once to new defaults (centre −75, GRID MIN −145, waterfall −140…−30), because the old values were set for the old scale. Signals now stand about twice as far above the noise floor, and the floor reads lower (about −130). Set dB CAL once with a known signal if you need exact levels.`
   - Under **Changed**: `- Receive server 3.145 (Windows). Install the client and the servers together (this installer does both).`
4. **Build order (Windows):**
   1. Stop MSCC and its servers.
   2. Build `mscc-ui/windows-work-tree/SDRcore-recv/SDRcore.vcxproj` (via `sdrcore-recv.sln`), config **MFC|Win32**, toolset v143. The output lands at `mscc-ui/Release/windows-wpf/mscc-recv.exe` (tracked).
      No new warnings in the changed files. Copy it to `C:\mscc-net9\mscc-recv.exe`.
   3. Then the WPF client: `dotnet build MSCC.sln -c Release` from `mscc-ui/windows-work-tree/mscc-mscc/mscc-new`. This bumps the version and runs `CopyToMsccNet9`.
   4. Check that `C:\mscc-net9\MSCC.Wpf.exe` ProductVersion = the new M.D.I with today's time, and that `mscc-recv.exe` has today's time.
   5. In the recv log after Start, the version reported to ms-sdr is 3.145 (ms-sdr log / client log "recv" version line, if shown).
5. Update `.mscc-coord/status/windows-new-hp.md` (cmd-069 row: recv 3.145, client M.D.I, not committed).

## Part 4. Smoke (NEW-HP, TG + step attenuator; Stew)
Use a CW carrier tuned a few hundred Hz off the tuning marker so the spike tip is visible. At 1x zoom, read the white trace tip. A carrier between FFT bins can read up to about 1.7 dB low; nudge the VFO a few Hz for the highest reading.
1. **First start:** the S/W window shows dB CAL trim 0, GRID MIN −145, waterfall LOW −140 / HIGH −30 (Proficio bank; Geminus LF −95/−25). `MSCC_Client.ini` now has `SPECTRUM_DB_LAW=20;`.
   Close and reopen MSCC: settings you change afterwards are kept (no second reset).
2. **Calibrate once:** TG at **−70 dBm** at the antenna jack (TG level + attenuator). Move dB CAL so the peak reads −70. **Write down the trim value** (it tells us whether the −75 centre is right).
3. **Step test, without touching dB CAL:** −30, −50, −70, −90, −110 dBm. Every peak reads within **±2 dB** of the set level (−110 should still be well above the floor, which sits around −130). Record the five readings.
   Before the fix, ±10 dB or worse away from the cal point.
4. Noise floor: no TG, antenna or dummy load. The floor stays at the same place at every step (it doesn't move with the TG level). Record it.
5. **Waterfall:** looks normal. The floor is dark, FT8/CW signals are clearly coloured, and nothing is washed out or all black. If it needs LOW/HIGH changes, note the values you like (for new defaults).
6. **S-meter unchanged:** same readings as before cmd-069 for the same TG levels (it is not touched).
7. cmd-068 still good: no peak drops when tuning at 1x/2x; the resolution selector is still gone; the title shows the new version.
8. **Remote host, if possible:** WPF 10.9.x connected to the Pi host (Pi servers unchanged). The same TG step reads within ±2 dB of the set levels with the same dB CAL (Pi Y = Windows Y now). If there's no TG at the Pi, a 20 dB attenuator step on an off-air carrier should move the trace 20 dB.
9. PANADAPTER AVERAGE / smoothing set higher (if the control exists): the levels don't change, and the trace only gets smoother (gain is now 1x at every setting).

## What NOT to touch
`rpi/`, `Solidus/`, `installers/rpi/`, the Pi/Ubuntu servers, `SDRcore-trans`, `ms-sdr-MKII`, the panadapter wire format (`dsputils.c` log/bias/150 and MAX_Y), the S-meter path, firmware, and Avalonia.

## Avalonia (later stew-HP brief, not here)
Avalonia builds the **same** `MSCC.Core` (`MSCC.Avalonia.csproj:35`), so it picks up the new `RawYToDb` on its next build. Until its own defaults are changed, it would show true dB with the old −91.3 centre, so the floor would sit around −147, below its −125 grid.
**Do not publish an Avalonia build from this tree until the follow-up is done.**

## Report
Full diffs, recv 3.145 build output path and time, client M.D.I, `C:\mscc-net9` exe times, and smoke results 1-9 (TG table: set level / reading; dB CAL trim used; floor).

STOP. No commit, no push.
