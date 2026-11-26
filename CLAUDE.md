# CLAUDE.md: shared working rules (mscc-station)

Claude Code loads this file in every session in this repo, for Ron (W4MMP) and for Stew (N8VET).
The rules are the same for both. "The owner" below means whoever is running the session.
Machine-specific notes stay in the folder files: `rpi/CLAUDE.md` (Ron's Pi / STM32 work) and
`mscc-ui/windows-work-tree/CLAUDE.md` (Stew's Windows work). Grok follows `AGENTS.md`.

## Keywords

| The owner says | Claude does |
|---|---|
| "build", "build R...", "make the installer" | Builds only. Installers go into `installers/`. Nothing is committed, pushed or published. |
| "update the notes" | Edits the notes only. No commit. |
| **"finished"** | Runs the checklist below: installers, docs, one commit, push, GitHub Release. |
| "stop" | Stops at once. |

**Pull first.** Before starting any new work, run `git pull --rebase --autostash` and tell the
owner what came in (who, when, one line each). Run `git fetch` again before the push at
"finished"; if the other person pushed meanwhile, rebase and rebuild whatever their changes touch.

**Nothing goes into git before "finished"**, notes included. Don't ask "shall I commit?" after
each step; the owner says "finished" when the job is done.

## Working style

- Short, plain answers. Say what was checked and what wasn't.
- **Ask before editing:** give the diagnosis and the proposed fix, and edit once the owner says OK.
- One commit per finished job, with `git add` of specific paths only (other people and tools work
  in this repo too).
- READMEs, how-tos and notes for the other person: the command first, then plain instructions.
- Notes for the other person go in `.mscc-coord/NOTE-FOR-STEW-*.md` / `NOTE-FOR-RON-*.md`. Word
  them as facts and options.

## Keep the trees in step

The Pi and Windows servers do the same job and are kept functionally identical (since 2026-11-10).

- **A server change (ms-sdr, recv, trans) goes into every server tree in the same step**, not at
  "finished":
  - Pi: `rpi/ms-sdr-linux`, `rpi/SDRcore-recv-linux`, `rpi/SDRcore-trans-linux`
  - Windows: `mscc-ui/windows-work-tree/ms-sdr-MKII`, `.../SDRcore-recv`, `.../SDRcore-trans`
  - Debian / Ubuntu: `linux/*-linux` (see `rpi/CLAUDE.md` for when this starts)
- Platform code stays different (USB, sockets, paths, audio device discovery).
- **A client change goes into both clients, either way:** WPF
  (`mscc-ui/windows-work-tree/mscc-mscc/mscc-new`) and Avalonia (`Avalonia/`). Each has its own
  copy of `MSCC.Core`; keep shared files such as `Protocol/KenwoodTs2000.cs` identical.
  `mscc-ui/Avalonia-Migration` is no longer kept up to date (to be deleted).
- Bump the version of every server whose code changed, in every tree.

## Versions

- Version numbers live only in the **Software Versions Addendum**
  (`docs/manuals/MSCC-Software-Versions-Addendum.md` + `.pdf`). Guides say "see the Software
  Versions Addendum" and never carry version numbers in their text.
- ms-sdr bumps its own `VERSION_MINOR` on every build (Pi: `bump-version.sh`; Windows: pre-build
  event). recv / trans: `VERSION_MINOR` in `sources/extern.h`, bumped by hand.

## "Finished" checklist

1. **Pi ms-sdr version.h:** the Pi build bumps its own copy; put the Pi's copy back into
   `rpi/ms-sdr-linux/source/version.h` (Pi -> repo only).
2. **Installers** for whatever changed (an unchanged one is reused and marked "unchanged"):
   - Windows: build the servers and/or the client, run `pwsh mscc-ui/installer/windows/stage.ps1`,
     set `AppVersion` and `SetupFileName` in `MSCC.aip`, run
     `"C:\Program Files (x86)\Actual Installer\actinst.exe" /S MSCC.aip` -> `installers/windows/`.
     - **Client number = R number** (R11-19-1 -> 11.19.1). `ClientVersion.txt` counts itself on
       every build: the first build of a day gives M.D.0, each later build +1. **Build the client
       once** per R number (a second build bumps it again); if the number is off, set
       `ClientVersion.txt` to one less and build once more. For a number-only build that leaves
       `C:\mscc-net9` alone: `dotnet build MSCC.Wpf.csproj -c Release -p:CustomAfterMicrosoftCommonTargets=<file>`,
       where the file holds `<Project><Target Name="CopyToMsccNet9" /></Project>`.
     - **stage.ps1 copies the client from the local WPF build.** Afterwards run
       `git status mscc-ui/installer/windows/payload` and look: only the files that belong in this
       release may change. If the payload already had a newer client or servers (the other person's
       pushed build), restore those payload files from git.
   - Pi: new server binaries into `rpi/mscc-binaries/`, bump `Version:` in
     `rpi/mscc-deb/packaging/DEBIAN/control`, build the .deb, compare it file by file with the last
     one -> `installers/rpi/` (old one to `installers/rpi/old/`). `mscc-avalonia` only if the
     client changed.
     - **File-by-file check:** unpack both (`dpkg-deb -R old.deb old`, `dpkg-deb -R new.deb new`),
       md5sum every file, diff the lists. Only the expected files may differ (control, the changed
       servers, firmware), and the file count stays the same unless a file was meant to be added.
   - Ubuntu (amd64): not part of "finished" and not in releases (Stew, 2026-11-25). Build only
     when asked; how it was done for R11-25-0 is in `rpi/CLAUDE-history.md`.
3. **Docs:** the Addendum (`python docs/manuals/make-addendum-pdf.py`, keep it to 2 pages), the
   Updating Firmware guide if a firmware tool or step changed (`python docs/manuals/make-addendum-pdf.py MSCC-Updating-Firmware.md`),
   `installers/windows/CHANGELOG.md`, `installers/windows/INSTALL.md`, `installers/rpi/INSTALL.md`,
   and the folder notes (`rpi/CLAUDE.md` / `mscc-ui/windows-work-tree/CLAUDE.md`).
4. **One commit, then push.** The message says what changed and ends with the current open list
   for the other person (Ron's commits: a "With Stew" list).
5. **GitHub Release** `R<month>-<day>-<n>` (e.g. R11-13-2):
   `gh release create R.. --title R.. --target main --notes-file notes.md <installers>`.
   **Every release carries ALL the installers** (Ron and Stew, 2026-11-19), changed or not: the
   Windows installer and the Pi (arm64) `mscc` and `mscc-avalonia` .debs. No Ubuntu files
   (Stew, 2026-11-25). The top release is always the complete current set. Notes start with a short **version table** (each package, its version, changed or
   unchanged), then what changed, then **"Downloads: which file do I need?"** (one line per file, what
   computer it's for), then the links:
   [Windows install guide](https://github.com/w4mmp-ron/mscc-station/blob/main/installers/windows/INSTALL.md) ·
   [Raspberry Pi install guide](https://github.com/w4mmp-ron/mscc-station/blob/main/installers/rpi/INSTALL.md) ·
   [Software versions (PDF)](https://github.com/w4mmp-ron/mscc-station/blob/main/docs/manuals/MSCC-Software-Versions-Addendum.pdf) ·
   [Documentation](https://github.com/w4mmp-ron/mscc-station/blob/main/docs/manuals/README.md)
   (the user documentation page: every guide, one line each; add new guides to it).
   Use links, not repo paths (people on the release page have no repo). Check the uploaded sizes.
   **Ubuntu .debs:** not in releases (Stew, 2026-11-25): no Ubuntu files, rows or Downloads lines.
   R11-25-0 carries them once; that release stays as it is.
   **Keep only the newest release and one earlier one** as the fallback: after publishing, delete
   the older releases (`gh release delete R.. -R w4mmp-ron/mscc-station --yes`; keep the git tags,
   so an old version can still be rebuilt from the repo).

If nothing installable changed, "finished" is just the commit and push; no new release.

**Installers are not in git** (`.gitignore`); people download them from
<https://github.com/w4mmp-ron/mscc-station/releases>.

## Never commit

- The Visual Studio build bookkeeping files in `mscc-ui/windows-work-tree`
  (`.tlog`, `.recipe`, `.iobj`, `.ipdb`, `.lastbuildstate`). They change at every build.
- Installers (`installers/windows/*.exe`, `installers/rpi/*.deb`).

## Build notes

- **Windows servers:** VS 2022 MSBuild, Configuration `MFC`; ms-sdr Platform `Win32`, recv / trans
  `x86`. Output: `mscc-ui/Release/windows-wpf/`.
- **Pi servers:** the Makefiles install into `~/mscc` (`BINDIR`); use `BINDIR=/tmp/...` for a test
  build. They don't track header files: after a header change (e.g. a version bump in
  `extern.h`), run `make clean` first.
- **Actual Installer:** run actinst from **PowerShell** (Git Bash turns `/S` into a path and the GUI
  opens). If it sits at 0 % CPU with no .exe, it's showing a hidden "subscribe to our newsletter?"
  box: answer No. Keep `MSCC.aip` at one UTF-8 BOM (a doubled BOM makes actinst exit 1).
- **`MSCC.aip` settings (don't guess them):** `UpgradeMode` must stay **0** (at 2 the installer
  refuses a fresh install: "This update program cannot continue"). `IfInstalled=2` = install over
  the old version without asking (0 = ask to uninstall, 1 = uninstall silently, 4 = ask the user).
  `Uninstall=1` + `ShowAddRemove=1` = uninstaller + Apps & features entry (settings in
  `%LOCALAPPDATA%\MSCC-NET9` are kept). Before changing any key, look it up in Actual Installer's
  help (`hh.exe -decompile` on its `Help.chm`), and test a fresh install, an install over it, and
  an uninstall.
- **Pi .deb from Windows:** build from a /tmp copy in WSL Debian, with CR stripped from text files
  (not from `.hex` / `.cyacd`).
