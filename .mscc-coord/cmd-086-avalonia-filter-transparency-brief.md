# cmd-086 — Avalonia 1.7.06 filter passband transparency and sky-blue edges

Host: **ubuntu-stew** (stew-HP). Repo: `/home/stew/Documents/GitHub/mscc-station`.

Stew asked for this brief on 2026-11-16 (t967u), after cmd-085 was pushed. **Avalonia client only.** This is the first half of the filter look: a lighter passband fill (PowerSDR-like) and sky-blue edges to match WPF. The WPF half is a later **cmd-086b on Shack**. Do not edit WPF, servers, or firmware in this Build. Minimal change. No extra refactors.

**Do not edit** `rpi/`, `Solidus/`, or `installers/rpi/` (Ron's). Do not edit `linux/` servers. Do not edit `mscc-ui/Avalonia-Migration` (retired; the live client is `Avalonia/`). Do not `git add -A`. Do not push. Do not amend a previous commit. Do not create a GitHub Release. Do not `sudo`. Build the amd64 `.deb` for Stew to install and smoke; do not build arm64. Commit only when Stew says **"finished"**. Stop after the smoke build and wait for him.

Before editing, run `git pull --rebase --autostash` and tell Stew what came in (who, when, one line each). Starting HEAD on stew-HP when this brief was written: **`b5bbce30`** (`Notes: claude-clean after R11-16-1 (2 done Stew items to history, With Stew pointer e2d88a9b)`). cmd-085 (`29ebd1e9`, Avalonia 1.7.05) is already on main. The working tree was clean except this brief and an `AGENTS.md` current-work pointer (both uncommitted; they ride the finished commit). If pull conflicts, or if the tree has other dirty files, **stop and report**.

Version: bump Avalonia **1.7.05 → 1.7.06** in `Avalonia/src/MSCC.Avalonia/MSCC.Avalonia.csproj` and `Avalonia/packaging/mscc-avalonia/DEBIAN/control` only. Do not bump servers or WPF. The title bar reads the csproj version.

Line numbers below are from `b5bbce30` on stew-HP. Recheck them before you edit.

---

# 1. Lighter filter passband fill

Stew compared MSCC with PowerSDR (`80_20 waterfall.jpg`). MSCC's filter band is too opaque: signals inside the passband are hard to see. Both clients brighten every pixel in the band instead of laying a light tint over it. PowerSDR's fill is only about 5–10 %, with a solid edge line showing the filter.

Change only `Avalonia/src/MSCC.Avalonia/Controls/SpectrumRenderer.cs` (the live client). Do not touch `mscc-ui/Avalonia-Migration`.

The fill is at **lines 179–181**. It currently adds B+30, G+40, R+70:

```csharp
bgra[o + 0] = (byte)Math.Min(255, bgra[o + 0] + 30);
bgra[o + 1] = (byte)Math.Min(255, bgra[o + 1] + 40);
bgra[o + 2] = (byte)Math.Min(255, bgra[o + 2] + 70);
```

Change the three additives to **B+5, G+10, R+16** (about one-fifth of the current values, so signals inside the filter stay clearly visible):

```csharp
bgra[o + 0] = (byte)Math.Min(255, bgra[o + 0] + 5);
bgra[o + 1] = (byte)Math.Min(255, bgra[o + 1] + 10);
bgra[o + 2] = (byte)Math.Min(255, bgra[o + 2] + 16);
```

Do not change the loop bounds, the clamp, or anything else in this block.

If Stew's smoke test finds the band too faint to see, the fallback is **+8, +14, +22**. Use the first values unless he asks for the fallback.

WPF still uses B+25, G+45, R+80. Leave that for cmd-086b. Do not add WPF's "skip fill in SCOPE" branch here; leave Avalonia's current fill-in-SCOPE behaviour, and let Stew check that SCOPE still looks right with the lighter fill.

---

# 2. Filter edge lines: orange → sky blue (match WPF)

`DrawVLine` is declared at **line 304** as `(…, byte b, byte g, byte r)` and writes BGRA (`buf[o+0]=b`, `o+1=g`, `o+2=r`).

**Lines 185–186** currently call:

```csharp
DrawVLine(bgra, stride, width, plotBottom, left, 0x20, 0xB0, 0xFF);
DrawVLine(bgra, stride, width, plotBottom, right, 0x20, 0xB0, 0xFF);
```

That is B=0x20, G=0xB0, R=0xFF = RGB(255, 176, 32), which shows **orange**.

Change both calls to sky blue RGB(32, 176, 255), i.e. B=0xFF, G=0xB0, R=0x20, to match WPF (WPF already calls `DrawVerticalLine(..., 0xFF, 0xB0, 0x20, 255)`):

```csharp
DrawVLine(bgra, stride, width, plotBottom, left, 0xFF, 0xB0, 0x20);
DrawVLine(bgra, stride, width, plotBottom, right, 0xFF, 0xB0, 0x20);
```

There is currently no colour comment on those two Avalonia calls. If you add one, say sky blue RGB(32,176,255), not orange. Do not copy WPF's incorrect comment "Orange filter edges" (that comment is wrong: WPF's numbers are already sky blue; cmd-086b will fix the comment on Shack).

Do not change the centre tune line (lines 150–160, which is a different orange-ish colour). Do not change `DrawVLine` itself.

---

# Version, docs, build, smoke

## Version

Bump **1.7.05 → 1.7.06** in:

- `Avalonia/src/MSCC.Avalonia/MSCC.Avalonia.csproj` (`<Version>`)
- `Avalonia/packaging/mscc-avalonia/DEBIAN/control` (`Version:`)

## Docs (do these with the code, before Stew smokes)

Match cmd-085 / 1.7.05:

**Addendum** (`docs/manuals/MSCC-Software-Versions-Addendum.md` only):

- Change Avalonia **1.7.05 → 1.7.06** in the programs table and in the Pi installer filename (`mscc-avalonia_1.7.06_arm64.deb`).
- Update the short Ubuntu paragraph so it covers 1.7.06 (lighter filter fill so signals inside the passband stay visible, sky-blue filter edges to match WPF). Keep the 1.7.05 items that are still true (packages 12.1.6, AGC button text, CMP greyed in Digital, QRP/AMP last-tuned freq, AGC FAST RELEASE 0–2000 ms). Say it is built and tested on Ubuntu, and will be attached at the next GitHub Release. Do not invent an arm64 `.deb` that was not built on this host. Do not rewrite the firmware or Windows-installer rows (out of scope).

**Do not rebuild or commit the Addendum PDF.** Leave `docs/manuals/MSCC-Software-Versions-Addendum.pdf` alone (Stew rebuilds that on Shack).

**`installers/windows/CHANGELOG.md`:** add an Avalonia 1.7.06 entry dated 2026-11-16 **above** the current top entries. This is **not** a new Windows `R11-…` installer. WPF and the Windows servers are unchanged. Plain language, keep the file's style. Suggested:

```
## Avalonia 1.7.06 — 2026-11-16  (not a new Windows installer; WPF and the Windows servers are unchanged)

### Fixed
- The filter passband on the spectrum is much more transparent (fill reduced from B+30/G+40/R+70 to B+5/G+10/R+16), so signals inside the filter stay clearly visible, like PowerSDR.
- The filter edge lines are sky blue RGB(32,176,255), matching WPF. They had been orange because DrawVLine's (b,g,r) arguments were swapped.

### Tested
- Built and smoked on Ubuntu amd64. Pi arm64 package not built in this step.
```

**Do not edit** `installers/rpi/INSTALL.md` (Ron's folder). The versions table there still says Avalonia 1.7.04; that is in the With Ron list.

Do not add this `.deb` to `installers/linux/` and do not edit `installers/linux/CHANGELOG.md` in this job (1.7.05 also left those alone). The `.deb` stays in `Avalonia/` for Stew to install, and rides the next GitHub Release.

`AGENTS.md` current-work already points at this cmd (written with this brief, uncommitted). Include that file in the finished commit. Do not rewrite the rest of `AGENTS.md`.

## Build (amd64 Ubuntu client, stew-HP)

This is the repo's normal way to smoke the live `Avalonia/` tree (same as cmd-085). Do **not** use `linux-build/run-avalonia.sh` or `linux-build/mscc-ui-x64.sh` — those still point at the retired `mscc-ui/Avalonia-Migration` tree.

From the repo root:

```bash
pwsh -NoProfile -File Avalonia/build-mscc-avalonia-deb.ps1 -Arch amd64
```

That writes `Avalonia/mscc-avalonia_1.7.06_amd64.deb` (gitignored; `Avalonia/.gitignore` has `*.deb`). Older 1.7.05 / 1.7.04 `.deb` files may already be in that folder; leave them.

Do not build arm64. Do not copy into `installers/rpi/` or `installers/linux/`. Do not `sudo` — Build cannot install the package. Tell Stew to reinstall with:

```bash
sudo apt install --reinstall -y /home/stew/Documents/GitHub/mscc-station/Avalonia/mscc-avalonia_1.7.06_amd64.deb
```

Then **stop** and wait for Stew's smoke test. Do not commit yet.

---

# Smoke test (Stew)

After the reinstall, title bar **1.7.06**. Client/server on this Ubuntu machine first (remote later).

1. **Signals inside the filter.** On a band with some signals, the traces and peaks inside the filter band should be easy to see, not washed out. The filter band itself should still be faintly visible, so you can tell where it is.
2. **Edge colour.** The two vertical filter edges should be **sky blue**, like WPF, not orange.
3. **SCOPE mode.** Switch to SCOPE. The spectrum line should still look right: the lighter fill must not hide the trace, and the sky-blue edges should still be there.

If the band is too faint to see, tell Build to use the fallback fill **+8, +14, +22** and rebuild the amd64 `.deb` before any commit. If anything else fails, tell Build before any commit.

---

# Finished / commit (only when Stew says "finished")

Stew's preference: **Build commits on this host; Stew pushes.** Do the commit, then stop. **No push. No GitHub Release.** Avalonia 1.7.06 rides the next GitHub Release.

This is an **Avalonia-only** job. Scope the root `CLAUDE.md` "finished" checklist to what applies. Skip: Pi ms-sdr `version.h`, Windows/Pi installer builds, `installers/rpi/*.deb`, firmware guide, `rpi/CLAUDE.md`, WPF (that is cmd-086b on Shack), servers, Addendum PDF, GitHub Release.

## 1. Pull first

```bash
git fetch
git pull --rebase --autostash
```

If a newer fetch brings Avalonia edits, stop and report the conflict.

Confirm `git status` shows only the cmd-086 Avalonia files, this brief, `AGENTS.md`, and the docs listed below. **Do not add leftover files or anything from another job.**

## 2. One commit, no push

`git add` only these paths (drop any you did not actually change):

```
Avalonia/src/MSCC.Avalonia/Controls/SpectrumRenderer.cs
Avalonia/src/MSCC.Avalonia/MSCC.Avalonia.csproj
Avalonia/packaging/mscc-avalonia/DEBIAN/control
.mscc-coord/cmd-086-avalonia-filter-transparency-brief.md
AGENTS.md
docs/manuals/MSCC-Software-Versions-Addendum.md
installers/windows/CHANGELOG.md
```

**Never add:** `docs/manuals/MSCC-Software-Versions-Addendum.pdf`, `Avalonia/mscc-avalonia_1.7.06_amd64.deb` (gitignored), `Avalonia/publish/`, anything under `bin/` or `obj/`, anything under `installers/linux/*.deb` or `installers/rpi/`, Visual Studio bookkeeping (`.tlog`, `.recipe`, `.iobj`, `.ipdb`, `.lastbuildstate`), or other people's dirty files.

Commit message (adjust the fill numbers if Stew asked for the fallback):

```
Avalonia 1.7.06: lighter filter fill, sky-blue edges

- Filter passband fill reduced from B+30/G+40/R+70 to B+5/G+10/R+16
  so signals inside the filter stay visible (PowerSDR-like).
- Filter edge lines changed from orange RGB(255,176,32) to sky blue
  RGB(32,176,255) to match WPF. DrawVLine takes (b,g,r).
- Addendum / Windows CHANGELOG: Avalonia 1.7.06. Addendum PDF to be
  rebuilt on Shack. No GitHub Release now; this rides the next release.
  WPF unchanged in this step (cmd-086b on Shack). amd64 .deb only;
  no arm64 build in this step.

With Ron
- Pi mscc-avalonia will need a rebuild from this source (we built
  amd64 only). installers/rpi/INSTALL.md still lists 1.7.04; please
  update that versions cell when you build the arm64 package.
```

Then **stop**. Do not push.
