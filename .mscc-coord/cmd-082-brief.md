# cmd-082: Avalonia QRP/AMP CAL last-tuned frequency (Part A) and AGC FAST RELEASE range (Part B)

Host: **ubuntu-stew** (stew-HP). Repo: `/home/stew/Documents/GitHub/mscc-station`.

**Do not start this brief until the cmd-081 commit is done** (Avalonia 1.7.03, slider wheel + QRP CAL TUNE mode). If `git log -1 --oneline` is not that commit, stop and tell Stew.

Then run `git pull --rebase --autostash` and tell Stew what came in (who, when, one line each). HEAD should already include Ron's `87e12aa` (AGC reply). Work from that, on top of the new 1.7.03 commit.

**Avalonia client only.** No WPF (that comes later, on Shack). No server changes, no firmware.
**Do not edit** `rpi/`, `linux/` servers, `Solidus/`, or `installers/rpi/`.
Minimal change. Check the line numbers below against the tree after the cmd-081 commit; they are from stew-HP after Part A+B, before the 081 commit.

**Parts A and B are done and tested on Ubuntu (Ultimus MKII).** Stew said "finished" (t909u). LF/Geminus testing is deferred. The **Finished / commit** section at the end of this file is now in force.

Version: bump Avalonia **1.7.03 → 1.7.04** in `Avalonia/src/MSCC.Avalonia/MSCC.Avalonia.csproj` and `Avalonia/packaging/mscc-avalonia/DEBIAN/control` only. Do not bump anything else.

---

# Part A — QRP CAL and AMP CAL use last-tuned frequency (Option A, client only)

## Why

Today a QRP or AMP band click looks up a fixed client table (`GetCalFrequencyHz`, 20m = 14.150) and, for QRP, sends 0xB6 then 0xA1. ms-sdr 0xA1 retunes to its own table (`Set_freq_for_power_cal`, 20m = 14.010). The freq box stays on 14.150; the radio transmits on 14.010. The two tables also disagree on 2200, 60, 17 and 15. Stew wants the cal carrier to be the last frequency he used on that band (or a default if unused), and the freq box must always equal the actual TX frequency. This matters most on LF/MF, where his antenna passband is narrow: if he is on 136 kHz, cal must be at 136 kHz. If he is on 14.074 and opens QRP CAL, cal must stay on 14.074.

## Design (agreed)

**Option A, client only.** No server change. Ron's Pi 0xA1 still retunes to the server table for a moment; a 0xB6 **after** 0xA1 overwrites `G_tune_freq` and that value is what `Cal_tune_button` / `SetModeRxTx` re-queue on key-up and key-down. Verified the same on linux ms-sdr, Windows ms-sdr-MKII, and `rpi/ms-sdr-linux`. The brief `.mscc-coord/NOTE-FOR-RON-QRP-AMP-CAL-FREQ-2026-11-15.md` (already written, leave untracked) tells Ron; do not edit the Pi tree.

## Frequency picker (use for QRP and AMP)

Add one helper, e.g. `ResolvePowerCalFrequencyHz(int band)`, and use it for both tabs. Order is mandatory:

1. **Current VFO, if it is already on that band.** Map the active VFO (`UseVfoA ? _frequencyHz : _vfoBFrequencyHz`) with `BandNameForFrequency` (8928–8943). If it equals the clicked band (normalize the same way `SelectBandAsync` does), use the current VFO. Do **not** fall through to last-used here: the 500 ms last-used debounce can be stale, and this is the "I'm on 14.074 and I open QRP CAL" case.
2. **Else** `BandLastUsedStore.Load(label, forVfoB: !UseVfoA)` (see `Services/BandLastUsedStore.cs`). Use that freq only if it is in 10_000…60_000_000 **and** `BandNameForFrequency` still maps it to this band (same accept/reject as `SelectBandAsync` 7072–7079).
3. **Else** `TryBandDefaultHz` (8953–8970) — the **existing main-tab band defaults**, not `GetCalFrequencyHz` and not the server `Set_freq_for_power_cal` table.

Main-tab defaults (must match `TryBandDefaultHz` / WPF MAIN band-button Tags): 2200=136.000, 630=474.200, 160=1.800, 80=3.500, 60=5.350, 40=7.000, 30=10.100, 20=14.000, 17=18.100, 15=21.000, 12=24.900, 10=28.000.

Do **not** write last-used from these cal retunes. `ShouldUpdateLastUsed` already returns false for reasons starting with `qrp-cal` / `amp-cal` and while `IsCalibrationOrIqSessionActive()` (6828–6849). Keep that. Use those reason prefixes on `ApplyFrequencyAsync`.

Do **not** change mode on a band click or on tab entry. cmd-081 Part B still saves the mode, sets TUNE, and restores it on stop.

**Stop using `GetCalFrequencyHz` for QRP and AMP** (call sites today: `SelectPowerCalBandAsync` ~4370, `SelectAmpCalBand` ~4769). Leave the function in place only if some other path still needs it (RX IQ fallback around 5208–5210 — do not change RX IQ). **Leave the TX IQ table `GetTxIqFrequencyHz` and all TX IQ band-click code alone.**

## QRP band click — `SelectPowerCalBandAsync` (~4353–4394)

Today: `ApplyFrequencyAsync(GetCalFrequencyHz)` (0xB6) **then** `SetBandPowerBandAsync` (0xA1). That order is the bug: 0xA1 overwrites the radio with the server table.

New sequence, **order mandatory**:

1. If TX is on, keep the existing "turn TX off first" block.
2. Pick `freq = ResolvePowerCalFrequencyHz(band)`.
3. Select the band in the UI (`PowerCalSelectedBand`, lamp) as now.
4. If connected: **`await _radio.SetBandPowerBandAsync(band)` (0xA1) FIRST**, then `await ApplyFrequencyAsync(freq, $"qrp-cal {band}m")` (0xB6). Then the freq box is whatever `ApplyFrequencyAsync` already sets (it must show `freq`, not the server table value).
5. If not connected: set `_frequencyHz` / `UpdateFrequencyUi(freq)` / `BandText` as the current not-connected path does, using `freq` from the picker (not `GetCalFrequencyHz`).

Do not send 0xA1 after 0xB6. Do not skip 0xA1 (the server still needs it to load the per-band power step and 0xB4).

Same-band early return (`if (PowerCalSelectedBand == band) return`) stays for a no-op re-click, but tab entry (below) must be able to run the sequence when the band is first selected.

## AMP band click — `SelectAmpCalBand` (~4755–4796)

0xF9 `CMD_SET_AMPLIFIER_INITIALIZE` does **not** retune (ms-sdr `amplifier.c` 139–151). AMP already transmits whatever 0xB6 last set. It only needs the same picker.

1. Keep the existing "TX off before band change" block.
2. `freq = ResolvePowerCalFrequencyHz(band)`.
3. Select the band in the UI as now.
4. If connected: `ApplyFrequencyAsync(freq, $"amp-cal {band}m")` (0xB6), then the existing `SetAmplifierInitializeAsync` (0xF9) and `SetAmplifierPowerAsync(100)` (0xFA). No 0xA1.
5. If not connected: update the display to `freq`, as now.

## Tab entry — pre-select the current band

Today `PowerCalSelectedBand` / `AmpCalSelectedBand` start at 0, and TUNE / CALIBRATE refuse until a band is clicked (`TogglePowerCalTxAsync` / `TogglePowerCalCalibrateAsync` ~4477 / 4526). Stew's case is: he is already on 14.074, he opens the QRP (or AMP) tab, and cal should happen at 14.074 without jumping.

In `MainTabs_OnSelectionChanged` (`Views/MainWindow.axaml.cs` 110–154):

- Keep the existing leave-QRP `ForceStopPowerCal("leave tab")` and all Freq Cal logic.
- When the **added** tab is `QrpCalTab` or `AmpCalTab`, if the current VFO maps to a ham band the tab supports (HF/LF gating still applies: 2200/630 only when `LfBandsEnabled`, 160–10 only when `HfBandsEnabled`), pre-select that band and run the same sequence as a click (QRP: 0xA1 then 0xB6 of the **current VFO**; AMP: 0xB6 + 0xF9). Prefer a small ViewModel method so the code-behind stays thin, e.g. `PreparePowerCalTab()` / `PrepareAmpCalTab()`.
- If the VFO is not on a ham band (GEN / out of range), leave the band unselected; the existing "Select a band" prompt stays.
- Do not retune to a table frequency on tab entry when the VFO is already in-band.

## What not to do (Part A)

- Do not change `BandLastUsedStore`, MAIN `SelectBandAsync`, or last-used file format.
- Do not change servers, `rpi/`, WPF, or TX IQ.
- Do not send mode TUNE on the band click (Part B of cmd-081 already does that on start).

---

# Part B — AGC FAST RELEASE range, to match WPF (Ron confirmed)

Ron’s reply (commit `87e12aa`, `.mscc-coord/NOTE-FOR-STEW-AGC-AND-PACKAGING-REPLY-2026-11-15.md`): the slider should go **0 to 2000 ms in steps of 10**, the same as WPF. Also raise the Avalonia view-model and settings clamps that today cut off at 1000 to **2000**. Stew is to make the Avalonia change; Ron did not change any Avalonia code.

WPF (leave it alone, for reference): `MainWindow.xaml` 2197–2198, `Minimum="0" Maximum="2000" SmallChange="10"`, wheel attached; ViewModel sends the slider value with no extra clamp.

## What to change (Avalonia only)

1. **Slider** `Views/MainWindow.axaml` ~1633–1635: `Maximum="100"` → **`Maximum="2000"`**, add **`SmallChange="10"`**, and attach **`PointerWheelChanged="Slider_OnPointerWheelChanged"`** (the cmd-081 Part A handler). Do not change Minimum (0). Do not hang it on the style; keep the per-slider attribute, same as the other sliders.
2. **View-model clamps** `ViewModels/MainViewModel.cs` — every `Math.Clamp(..., 0, 1000)` on `AgcFastRelease` becomes **`0, 2000`**:
   - `OnAgcFastReleaseChanged` ~2665 (`SendAgcFastReleaseAsync(Math.Clamp(value, 0, 1000))`)
   - settings load ~7686
   - connect restore ~8042
   If you find another 0–1000 clamp on this property, raise it too. The report handler ~8529 assigns the server value with no clamp; clamp that assignment to 0–2000 as well so a wild report cannot break the slider.
3. **Settings file** `Services/ClientSettingsStore.cs` ~820 (`AGC_FAST_RELEASE`): `Math.Clamp(afr, 0, 1000)` → **`0, 2000`**. Leave the default (`AgcFastRelease = 50` in the store and `_agcFastRelease = 50` in the VM) as it is.
4. Do not add a second opcode path. 0xCB (`SetAgcFastReleaseAsync`) stays as it is. Do not change AGC SLO/MED/FST button code, recv, or ms-sdr.

---

# Version, build, install

Bump to **1.7.04** in the csproj and `DEBIAN/control` only.

Rebuild the amd64 deb from the repo root:

```bash
pwsh -NoProfile -File Avalonia/build-mscc-avalonia-deb.ps1 -Arch amd64
```

Do not build arm64. Do not copy into `installers/rpi/`. Do not `sudo` — Build cannot install the package. Tell Stew to reinstall with:

```bash
sudo apt install --reinstall -y /home/stew/Documents/GitHub/mscc-station/Avalonia/mscc-avalonia_1.7.04_amd64.deb
```

---

# Smoke test (Stew), after the reinstall

Title bar **1.7.04**.

## QRP / AMP frequency (Part A)

For **each band including 2200 and 630** (use a Geminus for LF; skip LF on a Proficio):

1. On MAIN, tune to a known frequency on that band (e.g. 14.074 on 20m, 136 kHz on 2200m). Open **QRP CAL** — the freq box must stay on that frequency, the band should already be selected, and TUNE/CALIBRATE should be usable without clicking the band again. Press TUNE or CALIBRATE: the SA carrier must equal the freq box.
2. Click a **different** band you have used before: box and TX must go to that band's last-used frequency, not 14.150 / 14.010.
3. Click a band you have **not** used this install (or after clearing that band's last-used): box and TX must be the main-tab default (20m = 14.000, 2200m = 136.000, 630m = 474.200, etc.).
4. Repeat 1–3 on **AMP CAL** (AMP path on). Same frequencies; AMP has no 0xA1 retune, but the picker must still be last/default/current-VFO.
5. After cal, go back to MAIN and change band then back: last-used on MAIN must still be what you had **before** cal, not a cal-table frequency.

## Regression

6. **cmd-081 Part B still works:** on USB, open QRP CAL, press CALIBRATE without changing mode: clean TUNE carrier, no mic audio; stop returns USB. Leave the tab while keyed: TX stops and mode returns.
7. **Wheel still works:** one notch moves the image / value live on the IQ and other sliders from cmd-081. VFO wheel unchanged.
8. **AGC FAST RELEASE (Part B):** slider goes 0–2000, readout in ms, one wheel notch = 10 ms, clamped at 0 and 2000. Pick FAST, set 2000, confirm the radio takes it (same feel as SLO is fine; value is sent as 0xCB). Restart the client: a saved value above 1000 (e.g. 1500 or 2000) must come back, not be cut to 1000. SLO/MED/FST button still works.

## Out of scope

WPF (later, on Shack), servers, firmware, `rpi/`, `linux/` servers, `Solidus/`, `installers/rpi/`, TX IQ tables, last-used file format, GitHub Release, commit, push. The Ron note `.mscc-coord/NOTE-FOR-RON-QRP-AMP-CAL-FREQ-2026-11-15.md` is already written; leave it untracked (it goes in with the cmd-082 commit when Stew says "finished").

Parts A and B are complete. See **Finished / commit** at the end.


---

# Finished / commit (Stew said "finished", 2026-11-15)

Stew's preference: **Build commits on this host; Stew pushes.** Do the commit, then stop. **No push. No GitHub Release.** Avalonia 1.7.04 will ride the next GitHub Release with the firmware (and with 1.7.03).

This is an **Avalonia-only** job. Scope the root `CLAUDE.md` "finished" checklist to what applies. Skip: Pi ms-sdr `version.h`, Windows/Pi/Debian installer builds, `installers/rpi/*.deb`, firmware guide, `rpi/CLAUDE.md`, WPF, servers.

Stew will test LF (2200/630) later on a Geminus, after he loads firmware. **Note that in the commit message and the CHANGELOG as "LF not yet tested on Geminus."** Do not treat missing LF test as a reason to hold the commit.

## 1. Pull first

```bash
git pull --rebase --autostash
```

Tell Stew what came in (who, when, one line each).

As of this brief update, stew-HP is at **`f3af9d4`** (cmd-081, Avalonia 1.7.03, already pushed) plus the uncommitted 1.7.04 work. If a newer fetch brings Avalonia edits, stop and report the conflict.

Confirm `git status` shows only the cmd-082 Avalonia files, this brief, the Ron note, and any docs you add below. **Do not add leftover 1.7.03-only files or anything from another job.**

## 2. Docs for the Avalonia 1.7.04 bump

Match what **cmd-081 / `f3af9d4`** touched: `docs/manuals/MSCC-Software-Versions-Addendum.md`, `installers/windows/CHANGELOG.md`, `installers/rpi/INSTALL.md`. That commit did **not** include the Addendum PDF. Avalonia `.deb` files are not committed (`Avalonia/.gitignore` has `*.deb`).

**Addendum** (`docs/manuals/MSCC-Software-Versions-Addendum.md`):

- Change Avalonia **1.7.03 → 1.7.04** in the programs table and in the Pi installer filename (`mscc-avalonia_1.7.04_arm64.deb`).
- Keep the release header at **R11-14-3**. There is no new GitHub Release yet.
- Update the short Ubuntu line so it covers 1.7.04 as well (QRP/AMP last-tuned frequency, auto band on tab entry, AGC FAST RELEASE 0–2000 step 10). Say it is built and tested on Ubuntu (Ultimus MKII) and will be attached at the next GitHub Release, with the firmware. **LF not yet tested on Geminus.** Do not invent an arm64 deb that was not built on this host (cmd-082 built amd64 only).

**Do not rebuild or commit the Addendum PDF.** Stew will have it built later on Shack. Leave `docs/manuals/MSCC-Software-Versions-Addendum.pdf` alone (it will still describe 1.7.03 until Shack regenerates it).

**`installers/windows/CHANGELOG.md`**: add an Avalonia 1.7.04 entry dated 2026-11-15 **above** the 1.7.03 entry. This is **not** a new Windows `R11-…` installer. WPF and the Windows servers are unchanged. Suggested text (plain language, keep the file's style):

- Changed: QRP CAL and AMP CAL now use the last frequency tuned on that band (or the current VFO if you are already there, or the main-tab default if the band has not been used). Opening the tab auto-selects the band you were on. The freq box and the transmit frequency stay the same. Tested on Ubuntu (Ultimus MKII). **LF not yet tested on Geminus.**
- Fixed: AGC FAST RELEASE is 0–2000 ms in steps of 10, like WPF, including the view-model and settings clamps (was 0–100 on the slider, 0–1000 in the clamps). The value is kept after a reload. Mouse wheel on this slider is one notch = 10 ms.
- Known: leaving the AMP CAL tab while still keyed does not yet force-stop AMP TX (QRP leave-tab already does). Do not jump from AMP to QRP while keyed.

**`installers/rpi/INSTALL.md`**: the versions table still has `mscc-avalonia` **1.7.03** (line 16). Update that cell to **1.7.04**, as 1.7.03 did. Do not add version numbers to guides that don't already have them.

**Do not edit:** `installers/windows/INSTALL.md`, `installers/linux/CHANGELOG.md`, `rpi/CLAUDE.md`, `mscc-ui/windows-work-tree/CLAUDE.md`, firmware guide, the Addendum PDF.

## 3. One commit, no push

`git add` **only these paths** (skip any that you did not actually change):

```
Avalonia/packaging/mscc-avalonia/DEBIAN/control
Avalonia/src/MSCC.Avalonia/MSCC.Avalonia.csproj
Avalonia/src/MSCC.Avalonia/Services/ClientSettingsStore.cs
Avalonia/src/MSCC.Avalonia/ViewModels/MainViewModel.cs
Avalonia/src/MSCC.Avalonia/Views/MainWindow.axaml
Avalonia/src/MSCC.Avalonia/Views/MainWindow.axaml.cs
.mscc-coord/cmd-082-brief.md
.mscc-coord/NOTE-FOR-RON-QRP-AMP-CAL-FREQ-2026-11-15.md
docs/manuals/MSCC-Software-Versions-Addendum.md
installers/windows/CHANGELOG.md
installers/rpi/INSTALL.md
```

**Never add:** `docs/manuals/MSCC-Software-Versions-Addendum.pdf`, `Avalonia/mscc-avalonia_1.7.04_amd64.deb` (gitignored), `Avalonia/publish/`, anything under `installers/linux/*.deb` or `installers/rpi/*.deb`, Visual Studio bookkeeping (`.tlog`, `.recipe`, `.iobj`, `.ipdb`, `.lastbuildstate`), or other people's dirty files.

Commit message (Stew / "With Ron" convention):

```
Avalonia 1.7.04: QRP/AMP last-tuned cal freq and AGC 0-2000

- QRP CAL and AMP CAL use the current VFO if already on that band, else
  the client's last-used, else the main-tab default. QRP sends 0xA1 then
  0xB6 (order mandatory) so the freq box matches the TX frequency. AMP
  uses the same picker, then 0xB6 and 0xF9. Opening the tab auto-selects
  the band you were on. Tested on Ubuntu (Ultimus MKII). LF not yet
  tested on Geminus.
- AGC FAST RELEASE is 0-2000 ms, step 10, like WPF (slider, view-model
  and settings clamps). Wheel attached. Value held after a reload
  (0xCB 1090 ms on reconnect).
- Note for Ron: the client now sets the cal frequency after 0xA1; the
  Pi works unchanged. Optional later: 0xA1 could stop retuning
  (.mscc-coord/NOTE-FOR-RON-QRP-AMP-CAL-FREQ-2026-11-15.md).
- Addendum / Windows CHANGELOG / Pi INSTALL: Avalonia 1.7.04. Addendum
  PDF to be rebuilt on Shack. No GitHub Release now; this rides the
  next release with the firmware. WPF unchanged (later on Shack).
- Known: leaving AMP CAL while keyed does not yet force-stop AMP TX
  (QRP leave-tab already does). Do not jump AMP to QRP while keyed.

With Ron
- FYI: QRP/AMP cal frequency is now last-tuned / current VFO, set by
  the client after 0xA1. Pi server can stay as it is. Optional later
  (Option B): 0xA1 stops retuning, which would need the Pi tree too
  (NOTE-FOR-RON-QRP-AMP-CAL-FREQ-2026-11-15.md).
- Known open: AMP CAL leave-tab while keyed does not force-stop AMP TX.
- LF (2200/630) not yet tested on Geminus; Stew will do that later
  after loading firmware.
```

Show `git status`, `git diff --stat` (staged), and the resulting hash. **Do not `git push`.** Do not `gh release`.
