# cmd-086b — WPF filter passband transparency (match Avalonia 1.7.06)

Host: **windows-shack** (Shack). Repo: `C:\Users\n8vet\OneDrive\Documents\github\mscc-station`.

Stew asked for this brief on 2026-11-16 (t972u), after cmd-086 was pushed from stew-HP. **WPF client only.** This is the second half of the filter look: the same lighter passband fill Stew approved on the air in Avalonia 1.7.06 (B+5, G+10, R+16, PowerSDR-like). The filter edge colour is already sky blue RGB(32,176,255); only the wrong "Orange filter edges" comment is fixed. Do not edit Avalonia, servers, or firmware in this Build. Minimal change. No extra refactors.

**Do not edit** `rpi/`, `Solidus/`, or `installers/rpi/` (Ron's). Do not edit `linux/` servers, `Avalonia/`, `mscc-ui/Avalonia-Migration`, `radio-psoc-firmware/`, or anything under `mscc-ui/installer/windows/payload/`. Do not `git add -A`. Do not push. Do not amend a previous commit. Do not create a GitHub Release. Do not run `stage.ps1` or `actinst.exe`. Do not edit `MSCC.aip`. Commit only when Stew says **"finished"**. Stop after the Release build copies to `C:\mscc-net9` and wait for his smoke test.

Before editing, run `git pull --rebase --autostash` and tell Stew what came in (who, when, one line each). Starting HEAD on Shack when this brief was written: **`993a5d92`** (`Avalonia 1.7.06: lighter filter fill, sky-blue edges`). The working tree was clean except this brief and an `AGENTS.md` current-work pointer (both uncommitted; they ride the finished commit). If pull conflicts, or if the tree has other dirty files, **stop and report**.

Line numbers below are from `993a5d92` on Shack, verified 2026-11-16. Recheck them before you edit.

---

# Version (read this first — do not invent a number by hand)

Ron numbers the WPF client as **month.day.iteration** (`ClientVersion.txt`). The csproj target `IncrementClientVersion` runs `mscc-ui/windows-work-tree/mscc-mscc/mscc-new/src/MSCC.Wpf/bump-client-version.ps1` on every real `MSCC.Wpf` build (not design-time):

- New calendar month or day → iteration resets to **0**.
- Same calendar day → iteration increments.

The committed file is **11.15.0** (built on 15 November). Today is **16 November 2026**, so the **first** Release build of this job will write **11.16.0** automatically, and a second build the same day would become 11.16.1. Do **not** hand-edit the file to 11.16.0 (or anything else) before the build, and do **not** skip the script.

`C:\mscc-net9\MSCC.Wpf.exe` was last written 2026-11-16 16:51 (America/New_York). That is the installed R11-16-1 client (**11.15.0**), not a source rebuild: `ClientVersion.txt` in the repo is still 11.15.0. This job's Release build will overwrite that exe via `CopyToMsccNet9`.

**Do this:**

1. Leave `ClientVersion.txt` alone until the test build.
2. After the code edits, do **one** `dotnet` Release build of `MSCC.Wpf` (see Build below). Close any running `MSCC.Wpf` first so `C:\mscc-net9` is not locked.
3. Read `ClientVersion.txt` and the title-bar / `MSCC client version →` build message. That is the version for the Addendum and CHANGELOG (expected **11.16.0** if you only built once today). If a prior Debug/Release build today already bumped it, use whatever the file actually says — do not force it back.

**Version spots (what to touch, and what not to):**

| Spot | Action |
|------|--------|
| `mscc-ui/windows-work-tree/mscc-mscc/mscc-new/src/MSCC.Wpf/ClientVersion.txt` | Let the build script write it. Commit the result. |
| `MSCC.Wpf.csproj` `IncrementClientVersion` / `bump-client-version.ps1` | Do **not** edit. They already stamp `Version`, `InformationalVersion`, `AssemblyVersion`, and `FileVersion` (`M.D.I.0`) from the file. There is no separate `AssemblyInfo.cs` to bump. |
| `mscc-ui/installer/windows/MSCC.aip` (`AppVersion=11.15.0`, `SetupFileName=mscc-net9-R11-16-1-install`) | **Leave unchanged.** This job does not build a new Windows installer or GitHub Release. |
| `docs/manuals/MSCC-Software-Versions-Addendum.md` | After the build: WPF title bar cell **11.15.0 → the new number**. Keep the release header at **R11-16-1**. Avalonia stays **1.7.06**. Then rebuild the PDF. |
| `installers/windows/CHANGELOG.md` | After the build: add a **WPF 11.16.0** (or whatever was written) entry dated 2026-11-16 **above** the Avalonia 1.7.06 entry. This is **not** a new `R11-…` installer. |

---

# 1. Lighter filter passband fill

Stew compared MSCC with PowerSDR (`80_20 waterfall.jpg`). MSCC's filter band was too opaque: signals inside the passband were hard to see. Both clients brighten every pixel in the band instead of laying a light tint over it. PowerSDR's fill is only about 5–10 %, with a solid edge line showing the filter.

Avalonia 1.7.06 (cmd-086, `993a5d92`) already dropped its fill to **B+5, G+10, R+16**. Stew smoked that on the air and said signals inside the filter are now easy to see. Use those same amounts here.

Change only `mscc-ui/windows-work-tree/mscc-mscc/mscc-new/src/MSCC.Wpf/Controls/SpectrumDisplayControl.xaml.cs`.

The fill is at **lines 811–813**. It currently adds B+25, G+45, R+80:

```csharp
pixelBuffer[offset + 0] = (byte)Math.Min(255, pixelBuffer[offset + 0] + 25);
pixelBuffer[offset + 1] = (byte)Math.Min(255, pixelBuffer[offset + 1] + 45);
pixelBuffer[offset + 2] = (byte)Math.Min(255, pixelBuffer[offset + 2] + 80);
```

Change the three additives to **B+5, G+10, R+16**:

```csharp
pixelBuffer[offset + 0] = (byte)Math.Min(255, pixelBuffer[offset + 0] + 5);
pixelBuffer[offset + 1] = (byte)Math.Min(255, pixelBuffer[offset + 1] + 10);
pixelBuffer[offset + 2] = (byte)Math.Min(255, pixelBuffer[offset + 2] + 16);
```

The comment at **line 799** is wrong ("Light blue-ish passband"). Change it to:

```csharp
// Light passband fill (PowerSDR-like) -- skip for SCOPE to avoid fill under the trace
```

Do not change the `SCOPE` skip (`if (SpectrumColorSettings.CurrentFillMode != "SCOPE")` at line 800), the loop bounds, the clamp, or anything else in this block.

If Stew's smoke test finds the band too faint to see, the fallback is **+8, +14, +22** (same fallback as cmd-086). Use the first values unless he asks for the fallback.

---

# 2. Filter edge comments only (colour is already sky blue)

`DrawVerticalLine` is declared at **line 1107** as `(…, byte b, byte g, byte r, byte a)` and writes BGRA (`buffer[offset+0]=b`, `+1=g`, `+2=r`, `+3=a`).

**Lines 819–822** already call:

```csharp
DrawVerticalLine(pixelBuffer, stride, width, spectrumPlotBottom, filterLeft, 0xFF, 0xB0, 0x20, 255);
DrawVerticalLine(pixelBuffer, stride, width, spectrumPlotBottom, filterRight, 0xFF, 0xB0, 0x20, 255);
```

That is B=0xFF, G=0xB0, R=0x20 = RGB(32, 176, 255), which is already **sky blue**. **Leave these two calls and the colour values unchanged.**

The comment at **line 818** is wrong ("Orange filter edges"). Change only the comment to:

```csharp
// Sky blue filter edges RGB(32,176,255) (thin lines, keep even in SCOPE) — only if on-screen
```

Do not change the on-screen checks, the SCOPE "keep even in SCOPE" behaviour, `DrawVerticalLine` itself, or the centre tune line.

---

# Version, docs, build, smoke

## Docs (do these after the Release build, once the title-bar version is known, and before Stew smokes if you can; otherwise before the finished commit)

Match cmd-083 / WPF 11.15.0.

**Addendum** (`docs/manuals/MSCC-Software-Versions-Addendum.md`):

- Change WPF title bar **11.15.0 → the new number** in the programs table (currently "WPF, title bar 11.15.0").
- Keep the release header at **R11-16-1**. Keep the Windows installer filename `mscc-net9-R11-16-1-install.exe`. Avalonia stays **1.7.06**. Do not rewrite firmware or server rows (out of scope).
- Add a short Windows sentence (near the existing Avalonia 1.7.06 paragraph is fine) that WPF now has the same lighter filter fill as Avalonia 1.7.06, so signals inside the passband stay visible, and that the filter edges were already sky blue. It is a client-only drop to `C:\mscc-net9` for now; it will ride the next GitHub Release. Do not invent a new `mscc-net9-R11-…-install.exe`.

Then rebuild the PDF (this host is Shack; cmd-086 left the PDF for Shack):

```
python docs\manuals\make-addendum-pdf.py
```

Keep the PDF to **2 pages** (root `CLAUDE.md`). Commit both the `.md` and the `.pdf`. Do not rebuild the firmware guide. Do not edit `installers/windows/INSTALL.md` or `installers/rpi/INSTALL.md`.

**`installers/windows/CHANGELOG.md`:** add a WPF entry dated 2026-11-16 **above** the current top entry (Avalonia 1.7.06). This is **not** a new Windows `R11-…` installer. Servers and the installer number are unchanged. Plain language, keep the file's style. Suggested (adjust the version if the build wrote something other than 11.16.0, and the fill numbers if Stew asked for the fallback):

```
## WPF 11.16.0 — 2026-11-16  (not a new Windows installer; drop-in to C:\mscc-net9)

### Fixed
- The filter passband on the spectrum is much more transparent (fill reduced from B+25/G+45/R+80 to B+5/G+10/R+16), so signals inside the filter stay clearly visible, like PowerSDR and Avalonia 1.7.06.
- The "Orange filter edges" comment was wrong; the edges were already sky blue RGB(32,176,255). The comment now says sky blue. Colour unchanged.

### Tested
- Built and smoked on Shack (drop-in to C:\mscc-net9). No new Windows installer in this step.
```

`AGENTS.md` current-work already points at this cmd (written with this brief, uncommitted). Include that file in the finished commit. Do not rewrite the rest of `AGENTS.md`.

## Build (WPF Release on Shack)

This is the repo's normal way to smoke the WPF client (same as cmd-083). The csproj target `CopyToMsccNet9` copies `MSCC.Wpf.exe` and dependencies to **`C:\mscc-net9`**, leaving the servers in that folder alone. **Do not** copy into `mscc-ui/installer/windows/payload/`. **Do not** run `pwsh mscc-ui/installer/windows/stage.ps1`. **Do not** run `actinst.exe`. **Do not** edit `MSCC.aip`. **Do not** copy into `installers/windows/`.

Close any running MSCC first, then from the repo root (one Release build only, so the version stays at the first bump of the day):

```
dotnet build "mscc-ui\windows-work-tree\mscc-mscc\mscc-new\src\MSCC.Wpf\MSCC.Wpf.csproj" -c Release
```

Stew can smoke-test by running `C:\mscc-net9\MSCC.Wpf.exe`. A new Actual Installer run is **not** needed for this test.

Report the build's version line, any errors or warnings that look new, and the write time of `C:\mscc-net9\MSCC.Wpf.exe`.

Then **stop** and wait for Stew's smoke test. Do not commit yet.

---

# Smoke test (Stew)

After the Release copy to `C:\mscc-net9`, title bar must show the new client version (expected **11.16.0** if there was one Release build on 16 Nov).

1. **Signals inside the filter.** On a band with some signals, the traces and peaks inside the filter band should be easy to see, not washed out. The filter band itself should still be faintly visible, so you can tell where it is.
2. **Edge colour.** The two vertical filter edges should still be **sky blue** (they already were; this job does not change the colour).
3. **SCOPE mode.** Switch to SCOPE. The spectrum line should still look right: WPF already skips the fill in SCOPE, and the sky-blue edges should still be there.

If the band is too faint to see, tell Build to use the fallback fill **+8, +14, +22** and rebuild once before any commit. If anything else fails, tell Build before any commit.

---

# Finished / commit (only when Stew says "finished")

Stew's preference: **Build commits on this host; Stew pushes.** Do the commit, then stop. **No push. No GitHub Release.** WPF 11.16.0 (or whatever the build wrote) rides the next GitHub Release.

This is a **WPF-client-only** job. Scope the root `CLAUDE.md` "finished" checklist to what applies. Skip: Pi ms-sdr `version.h`, Windows/Pi installer builds, `stage.ps1`, `actinst`, `MSCC.aip`, `installers/rpi/*.deb`, firmware guide, `rpi/CLAUDE.md`, Avalonia (already done in cmd-086), servers, GitHub Release. Do the Addendum (`.md` + PDF) and the Windows CHANGELOG entry.

## 1. Pull first

```
git fetch
git pull --rebase --autostash
```

If a newer fetch brings WPF edits, stop and report the conflict.

Confirm `git status` shows only the cmd-086b WPF files, this brief, `AGENTS.md`, and the docs listed below. **Do not add leftover files or anything from another job.**

## 2. One commit, no push

`git add` only these paths (drop any you did not actually change; adjust the version in the message if the build wrote something other than 11.16.0):

```
mscc-ui/windows-work-tree/mscc-mscc/mscc-new/src/MSCC.Wpf/Controls/SpectrumDisplayControl.xaml.cs
mscc-ui/windows-work-tree/mscc-mscc/mscc-new/src/MSCC.Wpf/ClientVersion.txt
.mscc-coord/cmd-086b-wpf-filter-transparency-brief.md
AGENTS.md
docs/manuals/MSCC-Software-Versions-Addendum.md
docs/manuals/MSCC-Software-Versions-Addendum.pdf
installers/windows/CHANGELOG.md
```

**Never add:** Visual Studio bookkeeping under `mscc-ui/windows-work-tree` (`.tlog`, `.recipe`, `.iobj`, `.ipdb`, `.lastbuildstate`, `.vs/`, `*.user`), anything under `bin/` or `obj/`, Debug output, `C:\mscc-net9\`, anything under `mscc-ui/installer/windows/payload/`, `installers/windows/*.exe`, `MSCC.aip`, or other people's dirty files.

Commit message (adjust the fill numbers if Stew asked for the fallback, and the version if the build wrote something other than 11.16.0):

```
WPF 11.16.0: lighter filter fill (match Avalonia 1.7.06)

- Filter passband fill reduced from B+25/G+45/R+80 to B+5/G+10/R+16
  so signals inside the filter stay visible (PowerSDR-like), matching
  Avalonia 1.7.06.
- Filter edge colour was already sky blue RGB(32,176,255); only the
  wrong "Orange filter edges" comment is fixed.
- Addendum (md + PDF) and Windows CHANGELOG: WPF 11.16.0. No new
  Windows installer; Stew can run C:\mscc-net9\MSCC.Wpf.exe. No
  GitHub Release now; this rides the next release.

With Ron
- WPF now matches Avalonia 1.7.06 on filter passband transparency.
  Edge colour was already the same sky blue.
```

Then **stop**. Do not push.


---

## REVISION 1 (2026-11-16, after Stew's smoke test) — do this now, same cmd-086b

Stew's test result: the passband now has no color on any background, and the horizontal dB grid lines are gone inside the filter.

Why:
1. Lines 709-711 skip the grid inside the passband on purpose (old look). The old +25/+45/+80 fill hid that. With the faint fill, the passband reads as a hole with no grid.
2. Adding a constant to each pixel does nothing on a white or light background (it is already at 255), so there is no tint.

Avalonia already draws the grid through the passband, which is why it looks right there.

Changes in `SpectrumDisplayControl.xaml.cs` (WPF only, nothing else):

A. Grid through the passband: delete the skip at lines 709-711 (the comment `// Leave passband clear ...` and the `if (filterLeftX ... ) continue;`). Change the comment on line 692 to `// === Horizontal dB grid (S/W VIEW GRID) — full width, including filter passband ===`.

B. Passband fill = 12% blend toward sky blue RGB(32,176,255) instead of adding a constant. Replace the three fill lines (811-813) with:

```csharp
// 12% blend toward sky blue RGB(32,176,255): shows on light and dark backgrounds, signals stay readable
const int fillA = 30; // out of 256 (about 12%)
pixelBuffer[offset + 0] = (byte)(pixelBuffer[offset + 0] + (((0xFF - pixelBuffer[offset + 0]) * fillA) >> 8));
pixelBuffer[offset + 1] = (byte)(pixelBuffer[offset + 1] + (((0xB0 - pixelBuffer[offset + 1]) * fillA) >> 8));
pixelBuffer[offset + 2] = (byte)(pixelBuffer[offset + 2] + (((0x20 - pixelBuffer[offset + 2]) * fillA) >> 8));
```

(Bgra32: offset+0 = B = 0xFF, +1 = G = 0xB0, +2 = R = 0x20. The `>> 8` on a negative number rounds toward darker by at most 1, which is fine.)

Change the comment on line 799 to `// Sky blue passband tint (12% blend) -- skip for SCOPE to avoid fill under the trace`.

Keep: edges, SCOPE skip, version 11.16.2, CHANGELOG/Addendum wording (update the CHANGELOG line to say "12% sky-blue tint; dB grid now drawn through the passband").

Build: `dotnet build -c Release` of MSCC.Wpf as before (CopyToMsccNet9). Then stop for Stew's smoke test on a white and a dark background. Commit only when he says finished; include `.mscc-coord/NOTE-FOR-RON-RELEASE-NUMBERS-2026-11-16.md`. Do not push.

If Stew says the tint is too faint, use fillA = 40 (about 16%); too strong, use 20 (about 8%).
