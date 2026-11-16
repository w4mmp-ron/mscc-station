# cmd-085 — Avalonia 1.7.05 small fixes (CPU, AGC text, CMP in Digital, TX IQ wheel)

Host: **ubuntu-stew** (stew-HP). Repo: `/home/stew/Documents/GitHub/mscc-station`.

Stew asked for this brief on 2026-11-16 (t952u). **Avalonia client only.** No WPF, no server, no firmware. Minimal change. No extra refactors.

**Do not edit** `rpi/`, `Solidus/`, or `installers/rpi/` (Ron's). Do not edit `linux/` servers. Do not `git add -A`. Do not push. Do not amend a previous commit. Do not create a GitHub Release. Do not `sudo`. Build the amd64 `.deb` for Stew to install and smoke; do not build arm64. Commit only when Stew says **"finished"**. Stop after the smoke build and wait for him.

Before editing, run `git pull --rebase --autostash` and tell Stew what came in (who, when, one line each). Starting HEAD on stew-HP when this brief was written: **`298b6e18`** (`R11-16-0: Windows installer for the 11-16 firmware release`). The working tree was clean except this brief and an `AGENTS.md` current-work pointer (both uncommitted; they ride the finished commit). If pull conflicts, or if the tree has other dirty files, **stop and report**.

Version: bump Avalonia **1.7.04 → 1.7.05** in `Avalonia/src/MSCC.Avalonia/MSCC.Avalonia.csproj` and `Avalonia/packaging/mscc-avalonia/DEBIAN/control` only. Do not bump servers or WPF. The title bar reads the csproj version.

Line numbers below are from `298b6e18` on stew-HP. Recheck them before you edit.

---

# 1. Avalonia packages 12.1.1 → 12.1.6 (idle CPU)

Ron's note: `.mscc-coord/NOTE-FOR-STEW-AVALONIA-12.1.6-CPU-2026-11-15.md` (commit `525e586`). He already tested this on a Pi 5 with 1.7.04 source and only the package version changed.

Cause: a bug in Avalonia 12.1.1's X11 event loop (fixed upstream in 12.1.5, [PR #22453](https://github.com/AvaloniaUI/Avalonia/pull/22453)). On the Pi the client used 50–60 % of one core even when not connected. With 12.1.6: idle 0.2–0.3 %, connected about 33 % (real drawing work). Same X11 code on Ubuntu, so this helps stew-HP too.

In `Avalonia/src/MSCC.Avalonia/MSCC.Avalonia.csproj`, change only these four package refs (lines 21–24) from `12.1.1` to `12.1.6`:

```xml
<PackageReference Include="Avalonia" Version="12.1.6" />
<PackageReference Include="Avalonia.Desktop" Version="12.1.6" />
<PackageReference Include="Avalonia.Themes.Fluent" Version="12.1.6" />
<PackageReference Include="Avalonia.Fonts.Inter" Version="12.1.6" />
```

Leave `AvaloniaUI.DiagnosticsSupport`, `CommunityToolkit.Mvvm`, and everything else alone. No code changes for this part.

---

# 2. AGC button shows SLO after every start

Ron's note: `.mscc-coord/NOTE-FOR-STEW-AVALONIA-AGC-TEXT-AND-CMP-DIGITAL-2026-11-15.md` (commit `89a6353`). Line numbers are still current at `298b6e18`.

AGC itself is restored correctly; only the button text is wrong. With `AGC_LEVEL=2` saved (FST): the client sends 0x87 = 2, the server answers 2, the radio is in FST, but the button says SLO. The first press then goes to level 0 (still showing SLO), and it takes three presses to get back to FST.

In `Avalonia/src/MSCC.Avalonia/ViewModels/MainViewModel.cs`, constructor:

- Line 199: `LoadClientSettings()` sets `AgcLevel` (line 7750: `AgcLevel = Math.Clamp(s.AgcLevel, 0, 2)`). `OnAgcLevelChanged` (2647–2655) already sets `AgcButtonText` to `SLO` / `MED` / `FST`.
- Line 212: `AgcButtonText = "SLO";` overwrites that, unconditionally.
- When the server later reports the same level, `AgcLevel` does not change, so `OnAgcLevelChanged` does not run and the text stays SLO.

**Fix:** delete line 212 only (`AgcButtonText = "SLO";`). Do not replace it with another assignment. The property already defaults to `"SLO"` (line 707: `_agcButtonText = "SLO"`; `_agcLevel` defaults to 0), so a missing ini still shows SLO.

---

# 3. CMP stays clickable in Digital

Same Ron note. WPF greys the CMP button in Digital; Avalonia turns compression off but leaves the button clickable, so CMP can be turned back on.

WPF reference (do not edit WPF): `mscc-ui/windows-work-tree/mscc-mscc/mscc-new/src/MSCC.Wpf/MainWindow.xaml` 2123–2138 — the CMP button has a `DataTrigger` on `IsDigitalAudio` that sets `IsEnabled=False`.

Avalonia now:

- Button: `Avalonia/src/MSCC.Avalonia/Views/MainWindow.axaml` 1603–1606 — `Command="{Binding ToggleCompressionCommand}"` only. No Digital enable binding.
- Command: `MainViewModel.cs` 2632–2633:

```csharp
[RelayCommand(CanExecute = nameof(CanOperate))]
private void ToggleCompression() => CompressionOn = !CompressionOn;
```

- `OnIsDigitalAudioChanged` (2883–2913) already forces `CompressionOn = false` when entering Digital, and restores it for Phones. It does not refresh the command's CanExecute.
- `NotifyOperateCommands` (1391–1415) already calls `ToggleCompressionCommand.NotifyCanExecuteChanged()` when connect/busy changes.

**Fix (as Ron suggested, nothing more):**

1. Add `CanToggleCompression()` next to `CanOperate()` (line 1417):

```csharp
private bool CanToggleCompression() => CanOperate() && !IsDigitalAudio;
```

2. Change the command attribute at 2632 to `CanExecute = nameof(CanToggleCompression)`. Leave the method body as it is.
3. In `OnIsDigitalAudioChanged`, call `ToggleCompressionCommand.NotifyCanExecuteChanged()` with the other property notifications at the top of the method (2885–2888), **before** the `_suppressAudioSend` early return, so the button greys even when a settings load suppresses the send.

Do not add an axaml style or DataTrigger unless the command change is not enough to grey the button. Do not change the CMP level slider, and do not change how compression is forced off or restored.

---

# 4. TX IQ OFFSET and PHASE mouse wheel — verify first

Stew still wants one notch = one step, matching WPF. Check this during smoke. **Do not re-wire what 1.7.03 already did** unless the notch still jumps more than one step.

Already in the tree from `f3af9d4a` (Avalonia 1.7.03):

- Handler: `Avalonia/src/MSCC.Avalonia/Views/MainWindow.axaml.cs` 316–329, `Slider_OnPointerWheelChanged` — one notch = `SmallChange` (or 1), then `e.Handled = true`. Same idea as WPF `Slider_PreviewMouseWheel` in `mscc-ui/windows-work-tree/mscc-mscc/mscc-new/src/MSCC.Wpf/MainWindow.xaml.cs` 856–864.
- TX IQ OFFSET: `MainWindow.axaml` 1200–1202 — `Minimum="-200" Maximum="200" SmallChange="1" PointerWheelChanged="Slider_OnPointerWheelChanged"`, bound to `TxIqOffset`. Matches WPF `MainWindow.xaml` 1845–1850.
- TX IQ PHASE: `MainWindow.axaml` 1214–1218 — `Minimum="-100" Maximum="100" SmallChange="1" LargeChange="5" IsSnapToTickEnabled="True" TickFrequency="1"` plus the same wheel handler, bound to `TxIqPhase` (integer ticks; the readout is `TxIqPhaseText`, 0.05° per tick, ±5°). Matches WPF `MainWindow.xaml` 1865–1871.

Those sliders are `IsEnabled="{Binding TxIqTxOn}"`, so they only move when TX is on (AMP off, TX IQ tab).

If smoke shows one notch already equals 1 OFFSET unit and 1 PHASE tick (0.05°), leave this part alone and say so.

If a notch still jumps more than one step, fix only the handler (likely Avalonia Slider's own wheel running before the bubbling handler; WPF uses tunneling `PreviewMouseWheel`). Keep it one step per notch, do not add a second handler in axaml, and do not change the ranges or tick settings.

---

# Version, docs, build, smoke

## Version

Bump **1.7.04 → 1.7.05** in:

- `Avalonia/src/MSCC.Avalonia/MSCC.Avalonia.csproj` (`<Version>`)
- `Avalonia/packaging/mscc-avalonia/DEBIAN/control` (`Version:`)

## Docs (do these with the code, before Stew smokes)

Match cmd-082 / 1.7.04:

**Addendum** (`docs/manuals/MSCC-Software-Versions-Addendum.md` only):

- Change Avalonia **1.7.04 → 1.7.05** in the programs table and in the Pi installer filename (`mscc-avalonia_1.7.05_arm64.deb`).
- Update the short Ubuntu paragraph so it covers 1.7.05 (Avalonia packages 12.1.6 for the idle-CPU fix, AGC button text correct at start, CMP greyed in Digital). Say it is built and tested on Ubuntu, and will be attached at the next GitHub Release. Do not invent an arm64 `.deb` that was not built on this host. Do not rewrite the firmware or Windows-installer rows (out of scope).

**Do not rebuild or commit the Addendum PDF.** Leave `docs/manuals/MSCC-Software-Versions-Addendum.pdf` alone (Stew rebuilds that on Shack).

**`installers/windows/CHANGELOG.md`:** add an Avalonia 1.7.05 entry dated 2026-11-16 **above** the current top entries. This is **not** a new Windows `R11-…` installer. WPF and the Windows servers are unchanged. Plain language, keep the file's style. Suggested:

```
## Avalonia 1.7.05 — 2026-11-16  (not a new Windows installer; WPF and the Windows servers are unchanged)

### Fixed
- Avalonia packages 12.1.1 → 12.1.6. The 12.1.1 X11 event loop kept the client busy while idle (about half a core on the Pi even when not connected). Ron measured idle 0.2–0.3 % on 12.1.6.
- The AGC button no longer shows SLO at every start. It now keeps SLO / MED / FST from the saved level.
- CMP is greyed and not clickable in Digital, like WPF. It still turns off when you enter Digital, and comes back for Phones.

### Tested
- Built and smoked on Ubuntu amd64. Pi arm64 package not built in this step.
```

**Do not edit** `installers/rpi/INSTALL.md` (Ron's folder). The versions table there still says Avalonia 1.7.04; that is in the With Ron list.

Do not add this `.deb` to `installers/linux/` and do not edit `installers/linux/CHANGELOG.md` in this job (1.7.03 / 1.7.04 also left those alone). The `.deb` stays in `Avalonia/` for Stew to install, and rides the next GitHub Release.

`AGENTS.md` current-work already points at this cmd (written with this brief, uncommitted). Include that file in the finished commit. Do not rewrite the rest of `AGENTS.md`.

## Build (amd64 Ubuntu client, stew-HP)

This is the repo's normal way to smoke the live `Avalonia/` tree (same as cmd-081 / cmd-082). Do **not** use `linux-build/run-avalonia.sh` or `linux-build/mscc-ui-x64.sh` — those still point at the retired `mscc-ui/Avalonia-Migration` tree.

From the repo root:

```bash
pwsh -NoProfile -File Avalonia/build-mscc-avalonia-deb.ps1 -Arch amd64
```

That writes `Avalonia/mscc-avalonia_1.7.05_amd64.deb` (gitignored; `Avalonia/.gitignore` has `*.deb`). Older 1.7.03 / 1.7.04 `.deb` files may already be in that folder; leave them.

Do not build arm64. Do not copy into `installers/rpi/` or `installers/linux/`. Do not `sudo` — Build cannot install the package. Tell Stew to reinstall with:

```bash
sudo apt install --reinstall -y /home/stew/Documents/GitHub/mscc-station/Avalonia/mscc-avalonia_1.7.05_amd64.deb
```

Then **stop** and wait for Stew's smoke test. Do not commit yet.

---

# Smoke test (Stew)

After the reinstall, title bar **1.7.05**. Client/server on this Ubuntu machine first (remote later).

1. **CPU idle.** Start the client and leave it **not connected**. In System Monitor or `top`, it should be nearly idle (a fraction of one core), not sitting at about half a core the way 12.1.1 did. Then connect as usual: some CPU use for the spectrum and waterfall is expected.
2. **AGC label at start.** Set AGC to FST, close the client, start it again, and connect. The button should say **FST**, not SLO. One press should go FST → SLO (not a "dead" first press). Same check with MED if you want. You can also confirm `~/.config/MSCC-Avalonia/mscc-avalonia.ini` has `AGC_LEVEL=2` for FST.
3. **CMP in Digital.** Switch the audio path to Digital. CMP should be greyed and not clickable, and compression should stay off. Switch back to Phones: CMP should work again.
4. **TX IQ wheel** (only with AMP off, TX IQ tab, **TX on** — the sliders are disabled until then). One mouse-wheel notch on OFFSET should change the number by **1**. One notch on PHASE should move one tick (**0.05°** on the readout). If that is already true, this part needed no code change.

If anything fails, tell Build before any commit.

---

# Finished / commit (only when Stew says "finished")

Stew's preference: **Build commits on this host; Stew pushes.** Do the commit, then stop. **No push. No GitHub Release.** Avalonia 1.7.05 rides the next GitHub Release.

This is an **Avalonia-only** job. Scope the root `CLAUDE.md` "finished" checklist to what applies. Skip: Pi ms-sdr `version.h`, Windows/Pi installer builds, `installers/rpi/*.deb`, firmware guide, `rpi/CLAUDE.md`, WPF, servers, Addendum PDF, GitHub Release.

## 1. Pull first

```bash
git fetch
git pull --rebase --autostash
```

If a newer fetch brings Avalonia edits, stop and report the conflict.

Confirm `git status` shows only the cmd-085 Avalonia files, this brief, `AGENTS.md`, and the docs listed below. **Do not add leftover files or anything from another job.**

## 2. One commit, no push

`git add` only these paths (drop any you did not actually change; add the wheel handler only if you had to fix it):

```
Avalonia/src/MSCC.Avalonia/MSCC.Avalonia.csproj
Avalonia/packaging/mscc-avalonia/DEBIAN/control
Avalonia/src/MSCC.Avalonia/ViewModels/MainViewModel.cs
Avalonia/src/MSCC.Avalonia/Views/MainWindow.axaml.cs
.mscc-coord/cmd-085-avalonia-small-fixes-brief.md
AGENTS.md
docs/manuals/MSCC-Software-Versions-Addendum.md
installers/windows/CHANGELOG.md
```

**Never add:** `docs/manuals/MSCC-Software-Versions-Addendum.pdf`, `Avalonia/mscc-avalonia_1.7.05_amd64.deb` (gitignored), `Avalonia/publish/`, anything under `installers/linux/*.deb` or `installers/rpi/`, Visual Studio bookkeeping (`.tlog`, `.recipe`, `.iobj`, `.ipdb`, `.lastbuildstate`), or other people's dirty files.

Commit message (adjust the wheel line if you did not touch it):

```
Avalonia 1.7.05: 12.1.6 idle CPU, AGC text, CMP in Digital

- Avalonia packages 12.1.1 → 12.1.6 (X11 event-loop busy-wait fixed
  upstream in 12.1.5). Ron measured idle 0.2–0.3 % on a Pi 5 with only
  this change.
- AGC button text no longer resets to SLO after LoadClientSettings.
- CMP CanExecute is off in Digital, like WPF; the button is greyed.
- TX IQ OFFSET / PHASE wheel: already one SmallChange per notch from
  1.7.03; confirmed on Ubuntu / or handler fixed if a notch still jumped.
- Addendum / Windows CHANGELOG: Avalonia 1.7.05. Addendum PDF to be
  rebuilt on Shack. No GitHub Release now; this rides the next release.
  WPF unchanged. amd64 .deb only; no arm64 build in this step.

With Ron
- Thanks for the 12.1.6 CPU diagnosis and the AGC / CMP notes
  (NOTE-FOR-STEW-AVALONIA-12.1.6-CPU-2026-11-15.md and
  NOTE-FOR-STEW-AVALONIA-AGC-TEXT-AND-CMP-DIGITAL-2026-11-15.md).
  Both are in this build.
- Pi mscc-avalonia will need a rebuild from this source (we built
  amd64 only). installers/rpi/INSTALL.md still lists 1.7.04; please
  update that versions cell when you build the arm64 package.
```

Then **stop**. Do not push.
