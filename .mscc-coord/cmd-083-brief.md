# cmd-083: WPF QRP CAL TUNE mode (Part A), QRP/AMP last-tuned frequency (Part B), AGC confirm (Part C), Addendum PDF (Part D)

Host: **windows-shack** (Shack). Repo: `C:\Users\n8vet\OneDrive\Documents\github\mscc-station`.

HEAD must already be the cmd-082 commit **`784e5fcc`** (`Avalonia 1.7.04: QRP/AMP last-tuned cal freq and AGC 0-2000`), in sync with `origin/main`. This host also has a large **uncommitted cmd-080 / 080b firmware tree** (400+ dirty or untracked files under `radio-psoc-firmware/`, plus untracked `.mscc-coord/cmd-080-brief.md` and `cmd-080b-brief.md`). **That work must stay intact and must never be staged or committed here.**

**Do not** `git pull`, `git rebase`, `git stash`, `git checkout`, `git reset`, or `git clean`. Autostash would touch the firmware tree. If `git fetch` shows you are behind `origin/main`, stop and tell Stew. Work only in the WPF client and the docs listed below.

**WPF client only.** No Avalonia source edits (already done in cmd-081 / cmd-082). No server changes, no firmware.
**Do not edit** `rpi/`, `linux/` servers, `Solidus/`, `radio-psoc-firmware/`, `installers/rpi/`, or anything under `mscc-ui/installer/windows/payload/`.
Minimal change. Check the line numbers below against this tree (`784e5fcc`); they were read on Shack on 2026-11-15.

**Do not commit or push until Stew says "finished".** Then follow **Finished / commit** at the end, with specific paths only.

---

# Version (read this first — do not invent a number by hand)

Ron numbers the WPF client as **month.day.iteration** (`ClientVersion.txt`). The csproj target `IncrementClientVersion` runs `mscc-ui/windows-work-tree/mscc-mscc/mscc-new/src/MSCC.Wpf/bump-client-version.ps1` on every real `MSCC.Wpf` build (not design-time):

- New calendar month or day → iteration resets to **0**.
- Same calendar day → iteration increments.

The file is currently **11.14.3** (built on 14 November). Today is **15 November 2026**, so the **first** Release build of this job will write **11.15.0** automatically, and a second build the same day would become 11.15.1. That is why 11.15.x is the right series; do **not** hand-edit the file to 11.15.1 (or anything else) before the build, and do **not** skip the script.

**Do this:**

1. Leave `ClientVersion.txt` alone until the test build.
2. After the code edits, do **one** `dotnet` Release build of `MSCC.Wpf` (see Build below). Close any running `MSCC.Wpf` first so `C:\mscc-net9` is not locked.
3. Read `ClientVersion.txt` and the title-bar / `MSCC client version →` build message. That is the version for the Addendum and CHANGELOG (expected **11.15.0** if you only built once today). If a prior Debug/Release build today already bumped it, use whatever the file actually says — do not force it back.

**Version spots (what to touch, and what not to):**

| Spot | Action |
|------|--------|
| `mscc-ui/windows-work-tree/mscc-mscc/mscc-new/src/MSCC.Wpf/ClientVersion.txt` | Let the build script write it. Commit the result. |
| `MSCC.Wpf.csproj` `IncrementClientVersion` / `bump-client-version.ps1` | Do **not** edit. They already stamp `Version`, `InformationalVersion`, `AssemblyVersion`, and `FileVersion` (`M.D.I.0`) from the file. There is no separate `AssemblyInfo.cs` to bump. |
| `mscc-ui/installer/windows/MSCC.aip` (`AppVersion=11.14.3`, `SetupFileName=mscc-net9-R11-14-3-install`) | **Leave unchanged.** This job does not build a new Windows installer or GitHub Release. |
| `docs/manuals/MSCC-Software-Versions-Addendum.md` | After the build: WPF title bar cell **11.14.3 → the new number**. Keep the release header at **R11-14-3**. Avalonia stays **1.7.04**. Then rebuild the PDF (Part D). |
| `installers/windows/CHANGELOG.md` | After the build: add a **WPF 11.15.0** (or whatever was written) entry dated 2026-11-15 **above** the Avalonia 1.7.04 entry. This is **not** a new `R11-…` installer. |
| `installers/windows/INSTALL.md` | **Leave unchanged** (still `mscc-net9-R11-14-3-install.exe`). |
| `installers/rpi/INSTALL.md` | **Leave unchanged** (Avalonia is already 1.7.04). |

---

# Part A — port cmd-081 Part B: QRP CAL sets TUNE before 0xAC

## Why

WPF `TogglePowerCalTx` (3478–3501) and `TogglePowerCalCalibrate` (3508–3624) key TX with `SetCalibrationTuneAsync` (0xAC) only. They never set mode TUNE or send 0xA6. If the radio is still in a phone mode, CALIBRATE / TX puts out mic audio, the same bug Avalonia had before 1.7.03. Avalonia was fixed in cmd-081 Part B. Port that behaviour, using the patterns already in this WPF tree.

## What is already here (reuse it)

- AMP CAL start already does TUNE + 0xA6: `ToggleAmpCalTx` 3128–3139 and `ToggleAmpCalCalibrate` 3224–3229 (`SetTunePowerAsync(100)`, `RadioState.ActiveVfo.Mode = RadioMode.TUNE`, `OnPropertyChanged(nameof(ActiveMode))`, `SetModeAsync("TUNE")`, `SetAutoTuneAsync(true)`).
- AMP / QRP both snapshot the full operating state on tab enter: `EnterPowerCalTab` 3664–3677 (`_prePowerCalState = CaptureOperatingStateSnapshot()`), `EnterAmpCalTab` 3732–3750 (`_preAmpCalState`).
- `LeavePowerCalTab` 3682–3726 already sends 0xAC off if still keyed, then restores the snapshot (freq, mode, VFO, tune power) via `RestoreOperatingStateAfterCalTabAsync`. It does **not** send 0xA6 off (QRP never sent 0xA6).
- AMP restore-on-stop is `RestoreModeAfterAmpCalTx` 3079–3094 (uses the enter snapshot; if the saved mode is TUNE it falls back to USB) and `StopAmpCalTuneCarrier` 3096–3104 (0xA6 off, then restore).
- WPF will **not** let you go TX then CALIBRATE without stopping first (3517–3523: "TX ON. SET TX OFF BEFORE CALIBRATE"). So "TX then CALIBRATE must not save TUNE" is already blocked as a click path. Still: **do not recapture `_prePowerCalState` on TX/CALIBRATE start** (that snapshot is the pre-tab mode). Do not add a second save that would store TUNE.

## Required changes

In `ViewModels/MainViewModel.cs`:

1. **Start** of `TogglePowerCalTx` (when turning on) and `TogglePowerCalCalibrate` (start path, after the antenna warning, around 3530–3546): mirror AMP. Save nothing new if you are using `_prePowerCalState`. Set mode TUNE the same way AMP does (do **not** assign `ActiveMode = …`, because the `ActiveMode` setter at 1955 calls `SaveLastUsedForCurrentBand()`). Then `SetAutoTuneAsync(true)` (0xA6), **then** 0xAC on (and keep the existing 0xA2 0 on CALIBRATE start). Log it.
2. **Stop** of TX (TogglePowerCalTx off, currently only 0xAC off at 3497–3500) and **stop** of CALIBRATE (3552–3570, keep the Yes/No save dialog): 0xAC off, then `SetAutoTuneAsync(false)`, then restore the mode from `_prePowerCalState` (same idea as `RestoreModeAfterAmpCalTx`; if the snapshot mode is TUNE, fall back to USB). Stay on the tab; do not run the full leave-tab freq restore here.
3. **Leave tab** (`LeavePowerCalTab` 3695–3700): after 0xAC off, also 0xA6 off, then the existing snapshot restore (which already puts the mode back).
4. **Disconnect and app close:** today `StopRadioService` (6122) and `MainWindow_Closing` / `Dispose` (MainWindow.xaml.cs 1099, VM 7138) do **not** call `LeavePowerCalTab`. If Stew disconnects or closes while keyed, 0xAC can stay on. Before `_radioService.Stop()`, if the QRP tab is active or TX/CALIBRATE is on, do the same stop as leave-tab (0xAC off, 0xA6 off, restore). Hook this in `StopRadioService` (commands must go out while still connected) **and** in `MainWindow_Closing` before `Dispose` (Closing does not call `StopRadioService`; it only `Dispose()`s, and Dispose does not leave QRP).

Keep the antenna warning, the mutual exclusion with CALIBRATE, and the 0xA2 / accept-dialog behaviour. No server changes.

---

# Part B — port cmd-082 Part A: QRP/AMP cal frequency picker

## Why

`SelectPowerCalBand` (3367–3407) still uses `GetAmpCalFrequencyHz` (2577–2592; 20m = 14.150), sets the freq box, sends 0xB6 (`SetFrequencyAsync`), **then** 0xA1 (`SetBandPowerBandAsync`). ms-sdr 0xA1 retunes to its own table (20m = 14.010), so the box and the TX frequency disagree. `SelectAmpCalBand` (3012–3070) uses the same table and sends 0xB6 + 0xF9 + 0xFA 100 (AMP 0xF9 does not retune; the mismatch there is only the client table vs last-tuned). Avalonia 1.7.04 already uses last-tuned / current VFO / main-tab default, with 0xA1 **then** 0xB6 for QRP.

## Frequency picker (use for QRP and AMP)

Add one helper, e.g. `ResolvePowerCalFrequencyHz(int band)`, used by both tabs. Order is mandatory:

1. **Current VFO, if it is already on that band.** Use `RadioState.ActiveVfo.FrequencyHz` and `GetBandNameForFrequency` (6218–6243). If that name equals `{band}m` (e.g. 20 → `20m`), use the current VFO. Do **not** fall through to last-used here: this is the "I'm on 14.074 and I open QRP CAL" case. Works for VFO B because `ActiveVfo` is whichever is active (`UseVfoBLastUsedFile` at 6185).
2. **Else** last-used for that band and the active VFO: `SpectrumWaterfallSettings.LoadLastUsedForBand("{band}m", UseVfoBLastUsedFile)` (same load as RX IQ 4464–4465). Use it only if the freq is in a sane range (10_000…60_000_000) **and** `GetBandNameForFrequency` still maps it to `{band}m`. Do **not** call `LoadLastUsedForBand` on the view-model (6318) — that helper tunes the radio and applies mode/filters.
3. **Else** the **main-tab BandButton Tag default**, not `GetAmpCalFrequencyHz` and not the server cal table. QRP/AMP band buttons have no Tag (only `CommandParameter` = meters, XAML 1386 / 1529). Read the defaults from the MAIN band buttons in `MainWindow.xaml` 625–647:

   2200=136000, 630=474200, 160=1800000, 80=3500000, 60=5350000, 40=7000000, 30=10100000, 20=14000000, 17=18100000, 15=21000000, 12=24900000, 10=28000000.

   Put them in a small helper (e.g. `TryMainBandDefaultHz(int band)`). Do not parse XAML at runtime.

## QRP `SelectPowerCalBand`

Keep the "no band change while TX/calibrating" guard and the UI highlight.

- Replace `GetAmpCalFrequencyHz` with `ResolvePowerCalFrequencyHz`.
- If connected, **await** `SetBandPowerBandAsync(band)` (0xA1) **FIRST**, then **await** `SetFrequencyAsync(calFreq)` (0xB6). Order is mandatory. Do not fire-and-forget both (`_ =`) — 0xA1 must complete before 0xB6. A private async helper plus `[RelayCommand]` on a Task-returning method is fine.
- Set the freq box (`RadioState.ActiveVfo.FrequencyHz`) to that **same** `calFreq`. `RadioState.CurrentBand` / `StoreBandForActiveVfo` may stay, as now; leave-tab restore puts the pre-tab band back.
- The early `if (PowerCalSelectedBand == band) return` (3388–3389) must not block tab entry. Add a `force` flag (tab entry uses `force: true` so a re-entry on the same band still sends 0xA1 then 0xB6). Clicks on the already-selected band may still no-op.
- Do **not** change mode on a band click.

## AMP `SelectAmpCalBand`

Same picker. Then 0xB6 with the picked freq, then the existing 0xF9 (`SetAmplifierInitializeAsync`) and 0xFA 100 (`SetAmplifierPowerAsync(100)`), and the slider reset to −99. Await 0xB6 before 0xF9 if that is easy; 0xF9 does not retune, but the freq box and 0xB6 must be the picked value. Same `force` behaviour for tab entry. No mode change. No 0xA1 on AMP.

## Tab entry: pre-select the current band

`MainWindow.xaml.cs` `MainTabControl_SelectionChanged` already calls `EnterPowerCalTab` (722) and `EnterAmpCalTab` (735). Extend those VM methods:

- **Snapshot first** (already done at 3669 / 3737), then TUNE_POWER 100 (already), **then** auto-select. If you auto-select before the snapshot, leave-tab would restore the cal frequency.
- If the current VFO maps to a supported QRP/AMP cal band (10, 12, 15, 17, 20, 30, 40, 60, 80, 160, 630, 2200 — same as `RadioCalBitForBand` 3269–3273), call `SelectPowerCalBand` / `SelectAmpCalBand` with `force: true` so the band is already highlighted and 0xA1/0xB6 (or 0xB6/0xF9) have been sent. TUNE/CALIBRATE should work without another click.
- If the VFO is GEN / WWV / out of band (GetBandNameForFrequency is `gen` or `?`), do **not** auto-select; the user clicks a band.
- Do not change mode on tab entry (beyond what enter already does: snapshot, TUNE_POWER 100; AMP also forces TX/tune off).

## Last-used write suppression

`SaveLastUsedForCurrentBand` (6254) already honours `SuppressLastUsedSave`. Leave-tab restore already sets that flag (3809–3847). **It is missing during cal itself:** `SelectPowerCalBand` / `SelectAmpCalBand` currently assign `FrequencyHz` and send 0xB6 without suppression. If you route 0xB6 through `TuneToFrequency` (4763–4787), that method **will** call `SaveLastUsedForCurrentBand()` and can overwrite last-used with a default or cal freq.

**Add the guard.** The simplest correct fix: at the top of `SaveLastUsedForCurrentBand`, return if `_powerCalTabActive || _ampCalTabActive` (and keep `SuppressLastUsedSave`). Also wrap the band-select freq apply in `SuppressLastUsedSave` if you call `TuneToFrequency`. After cal, MAIN last-used must still be what it was **before** the tab, not 14.150 / 14.000.

## Stop using `GetAmpCalFrequencyHz` for QRP/AMP

After this, only the RX IQ START fallback at 4473 should still call `GetAmpCalFrequencyHz`. **Leave that, and leave `GetTxIqFrequencyHz` (2599), completely alone.** Do not delete the helper if RX IQ still uses it. You may add a comment that QRP/AMP no longer use it.

## Edge cases

- Tab entry while keyed: first entry cannot be keyed yet. Re-entry is ignored (`if (_powerCalTabActive) return`). Band clicks while keyed stay blocked.
- Part A snapshot vs Part B auto-select: snapshot **before** auto-select, as above. Do not recapture after the freq change.
- VFO B: picker and last-used must use the active VFO / `UseVfoBLastUsedFile`.
- 2200 / 630: same picker and Tag defaults (136.000 / 474.200). Stew will test LF later on a Geminus; still implement them. **LF not yet tested on Geminus** in the CHANGELOG / commit.
- 60m is a cal band here (Tag 5.350). Auto-select it if the VFO is in 5.330–5.405.

---

# Part C — WPF AGC FAST RELEASE: confirm only, no change

Already correct, same as Ron's 0–2000 / step 10 answer:

- `MainWindow.xaml` 2197–2198: `Minimum="0" Maximum="2000" SmallChange="10"`, `PreviewMouseWheel="Slider_PreviewMouseWheel"` already attached.
- `OnAgcFastReleaseChanged` 5475 sends 0xCB with no 0–1000 clamp. The report handler 6495–6497 assigns the server value. Leave all of this alone.

In the job report, quote these lines and say **Part C PASS (no code change)**.

---

# AMP leave-tab-while-keyed: report only, do not "fix" as a new design

**WPF does not have Avalonia's AMP leave-tab gap.** `LeaveAmpCalTab` 3756–3798 already clears `AmpCalTxOn` / `AmpCalCalibrating`, sends `SetTransmitAsync(false)` and `SetAutoTuneAsync(false)`, then restores the snapshot. `MainTabControl_SelectionChanged` 737–740 already calls it when leaving the tab.

**What WPF does have:** disconnect and app close do not call `LeaveAmpCalTab` (same as QRP, above). If you are hooking `StopRadioService` and `MainWindow_Closing` for Part A, also call `LeaveAmpCalTab` when the AMP tab is still active (or AMP TX is on). That is using the existing leave path, not a new AMP leave-tab behaviour. Do not invent a second AMP stop design. Mention the close/disconnect gap, and that leave-tab is already OK, in the job report and in the CHANGELOG Known line only if it is still true after the hook.

Do not jump from AMP to QRP while keyed until that close/disconnect hook is in. After the hook, leave-tab and close should both stop AMP TX.

---

# Build (WPF Release on Shack)

Solution: `mscc-ui/windows-work-tree/mscc-mscc/mscc-new/MSCC.sln`.
Project: `mscc-ui/windows-work-tree/mscc-mscc/mscc-new/src/MSCC.Wpf/MSCC.Wpf.csproj`.

Close any running MSCC first, then from the repo root (one Release build only, so the version stays at the first bump of the day):

```powershell
dotnet build "mscc-ui\windows-work-tree\mscc-mscc\mscc-new\src\MSCC.Wpf\MSCC.Wpf.csproj" -c Release
```

The csproj target `CopyToMsccNet9` copies `MSCC.Wpf.exe` and dependencies to **`C:\mscc-net9`**, leaving the servers in that folder alone. **Stew can smoke-test by running `C:\mscc-net9\MSCC.Wpf.exe`.** A new Actual Installer run is **not** needed for this test. Do not run `actinst.exe`. Do not edit `MSCC.aip`. Do not copy into `installers/windows/`.

Report the build's version line, any errors or warnings that look new, and the write time of `C:\mscc-net9\MSCC.Wpf.exe`.

---

# Smoke test (Stew), after the Release copy to `C:\mscc-net9`

Title bar must show the new client version (expected **11.15.0** if there was one Release build on 15 Nov).

## QRP CAL TUNE (Part A)

1. On MAIN, set USB (or LSB). Open **QRP CAL** (do not change mode). Press CALIBRATE or TX/TUNE: clean TUNE carrier, **no mic audio**. Stop: mode returns to USB (or whatever you had).
2. Leave the tab while keyed: TX stops, mode returns, you are back on the pre-tab frequency.
3. Key TUNE, then Disconnect (and, separately, close the app while keyed if you are willing): TX must stop. No leftover TUNE after you connect again.

## QRP / AMP frequency (Part B)

For each band you can use on this radio, including 2200 and 630 if a Geminus is on the bench (otherwise skip LF and note it):

4. On MAIN, tune to a known frequency on that band (e.g. 14.074 on 20m, 136 kHz on 2200m). Open **QRP CAL** — the freq box must stay on that frequency, the band should already be selected, and TUNE/CALIBRATE should work without clicking the band again. The SA carrier must equal the freq box.
5. Click a **different** band you have used before: box and TX must go to that band's last-used frequency, not 14.150 / 14.010.
6. Click a band you have **not** used (or after clearing that band's last-used): box and TX must be the main-tab Tag default (20m = 14.000, 2200m = 136.000, 630m = 474.200, etc.).
7. Repeat 4–6 on **AMP CAL** (AMP path on). Same frequencies; AMP has no 0xA1 retune, but the picker must still be current-VFO / last-used / default.
8. After cal, go back to MAIN and change band then back: last-used on MAIN must still be what you had **before** cal, not a cal-table or default frequency. Confirm with VFO B as well on at least one band.
9. If the VFO is on GEN / out of band and you open QRP CAL, no band should auto-select.

## AGC (Part C, confirm unchanged)

10. AGC FAST RELEASE still 0–2000, readout in ms, one wheel notch = 10. A saved value above 1000 still comes back after a reload.

## Out of scope

Avalonia source, servers, firmware, `rpi/`, `linux/` servers, `Solidus/`, TX IQ tables, last-used file format, a new Windows installer, GitHub Release, commit, push. The cmd-080 firmware tree must stay dirty and unstaged.

---

# Part D — rebuild the Addendum PDF

Do this **after** the WPF version is known (after the Release build) and after you have edited `docs/manuals/MSCC-Software-Versions-Addendum.md`:

- Release header stays **R11-14-3**.
- Programs table: WPF title bar → the new number (expected 11.15.0); Avalonia stays 1.7.04 (including the Pi installer filename already at `mscc-avalonia_1.7.04_arm64.deb`).
- Add a short Windows line that the WPF client now has the same QRP TUNE-before-0xAC fix and the same QRP/AMP last-tuned frequency / auto band-on-tab-entry as Avalonia 1.7.04. It is a client-only drop to `C:\mscc-net9` for now; it will ride the next GitHub Release, with the firmware. **LF not yet tested on Geminus.** Do not invent a new `mscc-net9-R11-…-install.exe`.

Then, from the repo root (Shack has Microsoft Edge; needs the Python `markdown` module — `python -m pip install --user markdown` if missing):

```powershell
python docs\manuals\make-addendum-pdf.py
```

Keep the PDF to **2 pages** (root `CLAUDE.md`). Commit both the `.md` and the `.pdf` this time (cmd-082 left the PDF for Shack). Do not rebuild the firmware guide.

Also add the WPF CHANGELOG entry (Version section above). Do not edit `installers/windows/INSTALL.md`.

---

# Finished / commit (only when Stew says "finished")

Stew's preference: **Build commits on this host; Stew pushes.** Do the commit, then stop. **No push. No GitHub Release.** This WPF build will ride the next GitHub Release with the firmware and Avalonia 1.7.03 / 1.7.04.

**Scope the root `CLAUDE.md` "finished" checklist.** Skip: Pi `version.h`, Windows/Pi/Debian installer builds, `actinst`, `MSCC.aip`, `installers/rpi/*.deb`, firmware guide, `rpi/CLAUDE.md`. Do **Part D** (Addendum `.md` + PDF) and the Windows CHANGELOG entry. Do **not** `git pull --rebase --autostash` on this host while the cmd-080 firmware tree is dirty.

Confirm `git status` shows the WPF / docs files below **plus** the existing cmd-080 firmware dirt and the untracked cmd-080 briefs. Those firmware files must stay unstaged.

`git add` **only these paths** (skip any you did not actually change):

```
mscc-ui/windows-work-tree/mscc-mscc/mscc-new/src/MSCC.Wpf/ClientVersion.txt
mscc-ui/windows-work-tree/mscc-mscc/mscc-new/src/MSCC.Wpf/ViewModels/MainViewModel.cs
mscc-ui/windows-work-tree/mscc-mscc/mscc-new/src/MSCC.Wpf/MainWindow.xaml.cs
.mscc-coord/cmd-083-brief.md
docs/manuals/MSCC-Software-Versions-Addendum.md
docs/manuals/MSCC-Software-Versions-Addendum.pdf
installers/windows/CHANGELOG.md
```

If you had to touch `MainWindow.xaml` (you should not, unless a typo forces it), add that path too. `ClientVersion.txt` must be the version the Release build wrote.

**Never add:**

- Anything under `radio-psoc-firmware/`
- `.mscc-coord/cmd-080-brief.md`, `.mscc-coord/cmd-080b-brief.md`
- `MSCC.aip`, `installers/windows/*.exe`, `installers/windows/INSTALL.md`
- Visual Studio bookkeeping under `mscc-ui/windows-work-tree` (`.tlog`, `.recipe`, `.iobj`, `.ipdb`, `.lastbuildstate`, `.vs/`, `*.user`)
- `bin/`, `obj/`, Avalonia trees, `rpi/`, `linux/`, `Solidus/`
- `git add -A` or `git add .`

Commit message (replace 11.15.0 if the file is a later iteration). Stew / "With Ron" convention:

```
WPF 11.15.0: QRP CAL TUNE mode and last-tuned cal freq

- QRP CAL CALIBRATE and TX/TUNE now set mode TUNE and 0xA6 before
  0xAC, and restore the previous mode on stop, leave tab, disconnect
  or close (same idea as AMP CAL / Avalonia 1.7.03).
- QRP CAL and AMP CAL use the current VFO if already on that band,
  else the client's last-used, else the main-tab button default.
  QRP sends 0xA1 then 0xB6 so the freq box matches the TX frequency.
  AMP uses the same picker, then 0xB6, 0xF9 and 0xFA. Opening the
  tab auto-selects the band you were on. Last-used is not overwritten
  during cal. LF not yet tested on Geminus.
- AGC FAST RELEASE was already 0-2000 ms, step 10; unchanged.
- Addendum (md + PDF) and Windows CHANGELOG: WPF 11.15.0. No new
  Windows installer; Stew can run C:\mscc-net9\MSCC.Wpf.exe. No
  GitHub Release now; this rides the next release with the firmware.

With Ron
- WPF now matches Avalonia 1.7.04 on QRP TUNE-before-0xAC and on
  last-tuned / current-VFO cal frequency (0xA1 then 0xB6). Pi server
  can stay as it is.
- LF (2200/630) not yet tested on Geminus; Stew will do that later
  after loading firmware.
```

Show `git status`, `git diff --stat` (staged), and the resulting hash. **Do not `git push`.** Do not `gh release`. Confirm `radio-psoc-firmware/` is still dirty and unstaged.

---

# Paste-ready prompt for Build

Do this on **Shack** only. Repo: `C:\Users\n8vet\OneDrive\Documents\github\mscc-station`. Read `.mscc-coord/cmd-083-brief.md` and follow it. WPF client only. Do not pull, stash, checkout, reset, or clean — the uncommitted cmd-080 firmware tree must stay untouched. Do not commit or push until Stew says finished. One Release build of MSCC.Wpf after the edits; let `bump-client-version.ps1` write ClientVersion.txt (expected 11.15.0 on the first build today). Stew will smoke-test `C:\mscc-net9\MSCC.Wpf.exe`. Then do Part D (Addendum md + PDF) once the title-bar version is known.

---

## Finished — commit (Build does this)

Stew said **"finished"** (t913u, 2026-11-15). This section is now in force. Do **only** the commit steps below. Do not rebuild, do not edit other files, do not start new work.

Scope the root `CLAUDE.md` "finished" checklist: skip Pi `version.h`, Windows/Pi/Debian installer builds, `actinst`, `MSCC.aip`, `gh release`, and a push. The Addendum (md + PDF) and Windows CHANGELOG are already updated. **Stew pushes.** You do not `git push`.

### 1. Pull first

```
git pull --rebase --autostash
```

Tell Stew what came in (who, when, one line each). If the rebase **conflicts**, stop and tell Stew. Do not resolve a messy conflict on your own.

`--autostash` is the only stash allowed, and only as part of that pull. Do **not** run a separate `git stash`, `git stash drop`, `git stash pop`, `git reset`, `git checkout`, or `git clean`. The uncommitted cmd-080 firmware tree must stay intact. After a successful pull, confirm the cmd-083 WPF/docs files and the cmd-080 firmware dirt are still in the working tree (autostash should have put them back).

### 2. Stage only these paths

`git add` **only** these paths (verified on Shack from `git status` after the cmd-083 work):

```
mscc-ui/windows-work-tree/mscc-mscc/mscc-new/src/MSCC.Wpf/ViewModels/MainViewModel.cs
mscc-ui/windows-work-tree/mscc-mscc/mscc-new/src/MSCC.Wpf/MainWindow.xaml.cs
mscc-ui/windows-work-tree/mscc-mscc/mscc-new/src/MSCC.Wpf/ClientVersion.txt
docs/manuals/MSCC-Software-Versions-Addendum.md
docs/manuals/MSCC-Software-Versions-Addendum.pdf
installers/windows/CHANGELOG.md
.mscc-coord/cmd-083-brief.md
```

Never `git add -A` or `git add .`.

Never stage:

- anything under `radio-psoc-firmware/`
- `.mscc-coord/cmd-080-brief.md` or `.mscc-coord/cmd-080b-brief.md`
- Visual Studio bookkeeping (`.vs/`, `*.user`, `.tlog`, `.recipe`, `.iobj`, `.ipdb`, `.lastbuildstate`)
- `bin/`, `obj/`, publish output
- `.deb` files, `installers/windows/*.exe`, `MSCC.aip`

Do not reset or stash the cmd-080 work.

### 3. Confirm the index

```
git diff --cached --name-only
```

It must show **exactly** the seven paths listed above, and nothing else. If anything else is staged, unstage only the extra path (`git restore --staged <path>`) and check again. If a listed path is missing, stop and tell Stew.

### 4. Commit (do not push)

One commit. Message style from root `CLAUDE.md`: say what changed, then end with the open list for the other person (Stew's commits use a **With Ron** trailer).

```
cmd-083: WPF 11.15.0 QRP CAL TUNE mode + restore, QRP/AMP cal frequency picker (current/last-tuned band freq), Addendum PDF

- QRP CAL CALIBRATE and TX/TUNE now set mode TUNE and 0xA6 before
  0xAC, and restore the previous mode on stop, leave tab, disconnect
  or close (same idea as AMP CAL / Avalonia 1.7.03).
- QRP CAL and AMP CAL use the current VFO if already on that band,
  else the client's last-used, else the main-tab button default.
  QRP sends 0xA1 then 0xB6 so the freq box matches the TX frequency.
  AMP uses the same picker, then 0xB6, 0xF9 and 0xFA. Opening the
  tab auto-selects the band you were on. Last-used is not overwritten
  during cal. LF not yet tested on Geminus.
- AGC FAST RELEASE was already 0-2000 ms, step 10; unchanged.
- Addendum (md + PDF) and Windows CHANGELOG: WPF 11.15.0. No new
  Windows installer; Stew can run C:\mscc-net9\MSCC.Wpf.exe. No
  GitHub Release now; this rides the next release with the firmware.

With Ron
- WPF now matches Avalonia 1.7.04 on QRP TUNE-before-0xAC and on
  last-tuned / current-VFO cal frequency (0xA1 then 0xB6). Pi server
  can stay as it is.
- LF (2200/630) not yet tested on Geminus; Stew will do that later
  after loading firmware.
```

**Do not `git push`.** Stew pushes. Do not `gh release`.

### 5. Report

Report the commit hash, then `git status --short`. The cmd-080 firmware files should still show as dirty or untracked, and the cmd-080 / cmd-080b briefs should still be untracked. Confirm nothing from `radio-psoc-firmware/` was committed.
