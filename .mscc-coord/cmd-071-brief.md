# cmd-071: Windows SDRcore-recv 3.146: port Ron's Pi spectrum smoothing + LO-point levelling (2026-10-10)

Host: **windows-new-hp**. Repo: `C:\Users\n8vet\OneDrive\Documents\GitHub\mscc-station`, at `f952111` (clean, up to date).
**Windows recv only** (`mscc-ui/windows-work-tree/SDRcore-recv/sources/`). No client code change. The client is rebuilt only to index its version for packaging.
**Do NOT edit `rpi/`, `Solidus/`, or `installers/rpi/`.** Read `rpi/` only as the source of the port. Don't touch `SDRcore-trans`, `ms-sdr-MKII`, the Ubuntu `linux/` servers, MSCC.Core, WPF code or Avalonia.
**STOP BEFORE COMMIT.** Show `git status` and `git diff --stat`, plus the full diff for `panadapter.c`, `dsputils.c`, `extern.h` and `CHANGELOG.md`.

Sources of the port (all read-only):
- Ron's Pi commits `8d7eca4` (smoothing + first LO levelling) and `a8094ee` (after-TX dip fix, the final `dsputils.c`). Take `dsputils.c` from `a8094ee` = HEAD.
- `.mscc-coord/NOTE-FOR-STEW-SPECTRUM-SMOOTHING-2026-10-10.md`.
- `rpi/CLAUDE.md` items 24-27.

Checked on NEW-HP:
- **Windows `panadapter.c`** is the same as the Pi file just before `8d7eca4`. The only differences are three comments with mangled `—`/`≈` characters; it already has the cmd-069 averaging fix and `pausing`/`smoothing`. So the new Pi `panadapter.c` drops in **verbatim**.
- **Windows `dsputils.c`** is the same as that Pi version apart from Windows-only code: the `WINDBG` blocks in `doAGC`, the longer packing comment from cmd-069, and the Si5351 cal code without `cal_mag_scaled`. All of that stays. The Pi changes are pure insertions in `doPanadapter` plus the `DC_BLOCK_A` value, so apply them as the four hunks below.
- `expf`/`sqrt` come from `<math.h>` via `extern.h`. `G_tx_mode`, `G_Pause_Panadapter`, `G_Smoothing` and `G_Panadapter_Blocks` are already declared in the Windows `extern.h`.

## How smoothing reaches the Windows recv (no change needed)
- **ms-sdr-MKII:** reads `PANADAPTER_AVERAGE` from `user_controls.ini` (`user_controls.c:346-352`, no range check, `uint8_t`).
  - It sends it to recv at start (`user_controls.c:587`, `CMD_GET_SET_PANADAPTER_SMOOTHING` 0xDA).
  - It forwards a client 0xDA, stores it and saves it (`main-controller.c:1868`, `user_controls.c:1373-1377`).
- **Windows recv:** `udp_thread.c:1102` `G_Smoothing = t_opcode_data + 2`. That is the same path as on the Pi, so the new `smoothing = G_Smoothing - 2` is correct on Windows.
- **The WPF client never sends 0xDA:** there is no constant in `MSCC.Core/Protocol/Opcodes.cs`. REFRESH is client-only: `MainViewModel.cs:2086-2091` draws 1 frame in N, and the waterfall gets one row per drawn frame. So today the setting comes only from the ini.
  - NEW-HP: `%LocalAppData%\MSCC-NET9\user_controls.ini` has `PANADAPTER_AVERAGE=0`.
  - Installer template: `C:\mscc-net9\init-files\user_controls.ini` also has `0`.
  - With 0 the new code behaves exactly as recv 3.145 does now.
  - Old Multus-era inis on some PCs have `1`. That would now mean a 25 ms running average: practically the same look, harmless.
- **Item 26 (REFRESH becomes a smoothing control): optional, NOT in this command.** It isn't trivial: it needs an opcode constant, a new control/range (0-30), removing the frame dropping, and naming. It belongs in a later WPF brief.
  - For testing, set `PANADAPTER_AVERAGE` in `%LocalAppData%\MSCC-NET9\user_controls.ini` with MSCC and its servers **stopped** (ms-sdr writes the file back), then start again.

## Part 1. `panadapter.c`: running-average smoothing (verbatim from the Pi)
Copy the Pi file over the Windows one byte for byte. Use cmd's `>` so PowerShell doesn't re-encode the file as UTF-16. Run from the repo root:
```
cmd /c "git show HEAD:rpi/SDRcore-recv-linux/sources/panadapter.c > mscc-ui\windows-work-tree\SDRcore-recv\sources\panadapter.c"
git diff --no-index --ignore-cr-at-eol --stat rpi/SDRcore-recv-linux/sources/panadapter.c mscc-ui/windows-work-tree/SDRcore-recv/sources/panadapter.c
```
The second command must print nothing (identical). Then `git diff mscc-ui/windows-work-tree/SDRcore-recv/sources/panadapter.c` must show only the `8d7eca4` change plus the three comment lines whose characters are now correct. Specifically:
- **History:** the `buffer_input[4]` history becomes `prev[MAX_X]` + `float avg[MAX_X]`, and `static int pan_average_primed`.
- **Setting:** `#define PAN_AVG_MAX_SETTING 100`, `PAN_AVG_STEP_SEC 0.025f`, `PAN_BLOCK_SEC (2048.0f / 96000.0f)`. Then `smoothing = G_Smoothing - 2` clamped 0..100, and `alpha = 1 - expf(-(G_Panadapter_Blocks * PAN_BLOCK_SEC) / (smoothing * 0.025))`.
- **0** = (this frame + previous) / 2, as today.
- **1..100** = `avg += (y - avg) * alpha` per display point, rounded, capped at `MAX_Y`.
- **Pausing and clears:** not updated while `pausing`. After `Clear_Panadapter_Average_History()` the first real frame loads `prev`/`avg` directly (no ramp from zero). `pan_average_primed = 1` is set after each non-pausing frame.
- **Removed:** the old 4-frame `memcpy` history shift.

If for any reason the copy is not identical, stop and report; don't hand-merge.

## Part 2. `dsputils.c`: gentle DC blocker + LO-point levelling + after-TX fix (from Pi `a8094ee`)
Line numbers are from the Windows file at `f952111`.

**2a.** Insert between the end of `doAGC` (line 116 `}`) and `void doPanadapter(` (line 118):
```c
/*
 * Levelling of the low-frequency noise hump around the LO point (VFO -12 kHz on the
 * display). The FFT bins within PAN_DC_HALF_BINS of the LO point are scaled down so
 * that their long-term average equals that of the PAN_DC_REF_BINS bins just outside
 * (the quieter side of the two, so a signal on one side does not lift it). Each bin
 * has its own slow average, so the hump goes and the normal noise texture stays: no
 * spike and no hole. Bins are never scaled up. A steady signal inside the slice is
 * levelled too, as it was hidden before. 4096 FFT at 96 kHz = 23.4 Hz per bin.
 *
 * The averages must not learn a burst: the low-frequency thump when the rig returns
 * from transmit drove them far up, and the slice then showed as a dip that took many
 * seconds to fade (Ron, 2026-10-10). So learning stops for PAN_DC_HOLD_FFTS after
 * transmit and during a tuning pause (the scaling learned before stays in use), and
 * one FFT never counts as more than PAN_DC_MAX_STEP times the current average.
 * (Ported from the Pi recv, cmd-071.)
 */
#define PAN_DC_HALF_BINS 16     /* levelled each side of the LO point (375 Hz) */
#define PAN_DC_REF_BINS 16      /* reference bins each side, just outside */
#define PAN_DC_ALPHA 0.01f      /* per FFT (47 a second): about 2 s */
#define PAN_DC_PRIME_FFTS 47    /* first second: plain average of all FFTs so far */
#define PAN_DC_HOLD_FFTS 47     /* no learning for about 1 s after transmit */
#define PAN_DC_MAX_STEP 4.0f    /* one FFT counts as at most this x the average */

```

**2b.** In `doPanadapter`, after `int keep;` (line 144), add:
```c
        // LO point levelling (see PAN_DC_HALF_BINS): slow average power per bin + reference
        static sp_float dc_bin_avg[2 * PAN_DC_HALF_BINS];
        static sp_float dc_ref_avg = 0.0f;
        static int dc_count = 0; // FFTs learned so far, up to PAN_DC_PRIME_FFTS
        static int dc_hold = 0; // FFTs left with learning stopped
        sp_float dc_left, dc_right, dc_ref, dc_pow, dc_alpha;
        int dc_centre, dc_learn;
```

**2c.** Directly before `if (panbuffer.panReady == 1 || G_tx_mode) {` (line 160), add:
```c
        if (G_tx_mode) dc_hold = PAN_DC_HOLD_FFTS; // LO point levelling: no learning right after transmit
```

**2d.** After the second shuffle loop (ends line 199 `}`), before `// 96 kHz` (line 201), add:
```c
        // Level the noise hump around the LO point (see PAN_DC_HALF_BINS).
        // After the shuffle the LO point lies between dbuf[nfft/2 - 1] and dbuf[nfft/2].
        dc_centre = mystate.nfft / 2;
        dc_learn = 1;
        if (dc_hold > 0) {
                dc_hold--;
                dc_learn = 0;
        }
        if (G_Pause_Panadapter) dc_learn = 0;
        if (dc_centre >= PAN_DC_HALF_BINS + PAN_DC_REF_BINS && (dc_learn || dc_count > 0)) {
                // First PAN_DC_PRIME_FFTS: plain average of everything so far, then the slow average
                dc_alpha = (dc_count < PAN_DC_PRIME_FFTS) ? 1.0f / (sp_float) (dc_count + 1) : PAN_DC_ALPHA;

                if (dc_learn) {
                        dc_left = 0.0f;
                        dc_right = 0.0f;
                        for (j = 0; j < PAN_DC_REF_BINS; j++) {
                                k = dc_centre - PAN_DC_HALF_BINS - 1 - j;
                                dc_left += (dbuf[k].real * dbuf[k].real) + (dbuf[k].imag * dbuf[k].imag);
                                k = dc_centre + PAN_DC_HALF_BINS + j;
                                dc_right += (dbuf[k].real * dbuf[k].real) + (dbuf[k].imag * dbuf[k].imag);
                        }
                        dc_ref = ((dc_left < dc_right) ? dc_left : dc_right) / (sp_float) PAN_DC_REF_BINS;
                        if (dc_count >= PAN_DC_PRIME_FFTS && dc_ref > dc_ref_avg * PAN_DC_MAX_STEP)
                                dc_ref = dc_ref_avg * PAN_DC_MAX_STEP;
                        dc_ref_avg += (dc_ref - dc_ref_avg) * dc_alpha;
                }

                for (j = 0; j < 2 * PAN_DC_HALF_BINS; j++) {
                        k = dc_centre - PAN_DC_HALF_BINS + j;
                        if (dc_learn) {
                                dc_pow = (dbuf[k].real * dbuf[k].real) + (dbuf[k].imag * dbuf[k].imag);
                                if (dc_count >= PAN_DC_PRIME_FFTS && dc_pow > dc_bin_avg[j] * PAN_DC_MAX_STEP)
                                        dc_pow = dc_bin_avg[j] * PAN_DC_MAX_STEP;
                                dc_bin_avg[j] += (dc_pow - dc_bin_avg[j]) * dc_alpha;
                        }
                        if (dc_bin_avg[j] > dc_ref_avg && dc_bin_avg[j] > 0.0f) {
                                sp_float scale = sqrt(dc_ref_avg / dc_bin_avg[j]);
                                dbuf[k].real *= scale;
                                dbuf[k].imag *= scale;
                        }
                }
                if (dc_learn && dc_count < PAN_DC_PRIME_FFTS) dc_count++;
        }

```
(If MSVC warns C4244 double→float on the `sqrt` line, make it `(sp_float) sqrt(...)`. That's the only allowed deviation.)

**2e.** DC blocker comment and value (lines 381-390). Replace
```c
 * (doPanadapter runs before the 12 kHz shift). 0.98 at 96 kHz = -3 dB at ~300 Hz
 * (0.9999 = 1.5 Hz removed only the DC, the spur stayed). Replaces the old pixel
 * notch + fill in panadapter.c.
 * Display only: that slice is 12 kHz outside the audio passband.
 */
#define DC_BLOCK_A 0.98f
```
with
```c
 * (doPanadapter runs before the 12 kHz shift). 0.9999 at 96 kHz = -3 dB at 1.5 Hz:
 * it takes out the true DC only. 0.98 (-3 dB at ~300 Hz) also flattened the noise
 * hump but left a hole about 1 kHz wide in the spectrum (Ron, 2026-10-10); the hump
 * is now levelled in doPanadapter instead (PAN_DC_HALF_BINS).
 * Display only: that slice is 12 kHz outside the audio passband.
 */
#define DC_BLOCK_A 0.9999f
```

Check: `git diff --no-index --ignore-cr-at-eol rpi/SDRcore-recv-linux/sources/dsputils.c mscc-ui/windows-work-tree/SDRcore-recv/sources/dsputils.c` now shows **only** the Windows-only differences listed above (WINDBG, the cmd-069 packing comment, cal code, comment characters), plus the "(Ported from the Pi recv, cmd-071.)" line. There should be no difference anywhere in the levelling or the DC blocker.

## Part 3. Version
`sources/extern.h:40`: `#define VERSION_MINOR 145` → `146` (recv **3.146**). Nothing else.

## Part 4. Build + copy
1. Stop MSCC and its servers.
2. Build `mscc-ui/windows-work-tree/SDRcore-recv/SDRcore.vcxproj` (via `sdrcore-recv.sln`), config **MFC|Win32**, toolset v143 (as in cmd-069). The output is `mscc-ui/Release/windows-wpf/mscc-recv.exe` (tracked). No new warnings in `panadapter.c`/`dsputils.c`. Copy it to `C:\mscc-net9\mscc-recv.exe`.
3. **Index the client for packaging** (no client code change): `dotnet build MSCC.sln -c Release` from `mscc-ui/windows-work-tree/mscc-mscc/mscc-new`. `ClientVersion.txt` 10.11.1 → **10.11.2** (same day; use what the build prints), and it copies to `C:\mscc-net9`. The installer name and title bar then match.
4. Check:
   - `C:\mscc-net9\mscc-recv.exe` and `MSCC.Wpf.exe` both have today's time.
   - `MSCC.Wpf.exe` ProductVersion is 10.11.2.
   - trans (3.146) and ms-sdr (3.182) are unchanged (Oct 8 files).
   - recv reports 3.146 (log/version line).
5. `installers/windows/CHANGELOG.md`: add a **new section above R10.11.1** (R10.11.1 is released, so leave it as it is):
   ```
   ## R10.11.2 - 2026-10-11  (`mscc-net9-R10-11-2-install.exe`, title bar 10.11.2)

   ### Fixed
   - Spectrum: no more dark hole (or spike) in the middle of the spectrum and down the waterfall at VFO -12 kHz. The noise there is now levelled to match its surroundings.
   - Spectrum: after a transmission the middle of the spectrum no longer shows a dip that takes many seconds to fade.

   ### Changed
   - Spectrum smoothing (PANADAPTER_AVERAGE in user_controls.ini): 0 looks as before; 1 to 100 give a calmer trace at full frame rate (time constant = value x 0.025 s, e.g. 9 = 0.225 s). Signal levels are unchanged. The waterfall is smoothed the same way.
   - Receive server 3.146 (Windows). Client unchanged apart from the version number.
   ```
   (Use the version the build printed if it isn't 10.11.2.) Use the same plain hyphen-dash style as the existing headings.
6. Update `.mscc-coord/status/windows-new-hp.md` (cmd-071 row: recv 3.146, client 10.11.2 version-only, not committed).
7. Packaging is **not** part of this command. Stew repackages from `C:\mscc-net9` as `mscc-net9-R10-11-2-install.exe` after the smoke test.

## Part 5. Smoke (NEW-HP; Stew, TG + step attenuator)
1. **LO point** (VFO -12 kHz on the display, `PANADAPTER_AVERAGE=0`, antenna or dummy load):
   - No hole, no notch and no DC spike in the trace.
   - No dark or bright stripe down the waterfall.
   - The noise texture there looks like its neighbours.

   Allow about 2 s after start or a band change for the levelling to settle. Check at 1x and 4x zoom. Watch for shoulders just outside ±375 Hz (report them if seen).
2. **After TX:** key TUNE/CW for 2 s and 20 s. On return to RX the middle of the spectrum shows no lasting dip. At most a brief disturbance (under about 1 s), then normal.
3. **Smoothing:**
   - `PANADAPTER_AVERAGE=0` (as shipped) looks like recv 3.145.
   - Stop everything, set 9, start: the trace is clearly calmer at REFRESH 1, the frame rate is unchanged and the waterfall is slightly smoothed.
   - Try 20 (slow, 0.5 s), then set it back to 0 (or leave the value Stew likes and note it).
   - The noise floor may look a little lower and flatter at higher settings: the per-pixel max now picks from calmer bins. That's expected.
4. **Levels unchanged:** a TG at -70 dBm (carrier away from VFO -12 kHz) still reads -70 ±2 with trim 0, both at AVERAGE 0 and at 9. A quick -30/-110 check is also within ±2.
5. **Signal inside the slice:** a steady carrier tuned exactly onto VFO -12 kHz (±375 Hz) is flattened into the floor after about 2 s. This is expected and display only; audio is 12 kHz away. Note it, don't fail it.
6. cmd-068/069/070 still good:
   - No peak drops when tuning at 1x.
   - The S-meter dBm toggle works, and the S-meter reads the same as before.
   - The title shows 10.11.2.
   - The Remote host (Pi 1.0.62, which has the same recv changes) looks the same as local.

## What NOT to touch
`rpi/`, `Solidus/`, `installers/rpi/`, `linux/` (Ubuntu servers), `SDRcore-trans`, `ms-sdr-MKII`, MSCC.Core, WPF/Avalonia code (only the version bump by the build), the panadapter wire format (log/bias/150, `MAX_Y`), the S-meter path, and firmware.

## Report
The diffs, the identity checks (Part 1 empty; Part 2 Windows-only lines only), build warnings, file times/versions in `C:\mscc-net9`, and smoke 1-6.

STOP. No commit, no push.
