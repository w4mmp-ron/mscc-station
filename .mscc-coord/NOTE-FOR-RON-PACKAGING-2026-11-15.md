# Note for Ron: scripted packaging plan (2026-11-15)

From Stew (written with Grok Bot). Proposal only. No packaging code changed yet. We pulled your R11-14-3 work first.

## Goal

Stew wants the packaging scripted, so the Windows installer, the amd64 .debs and the Pi .deb all pull the same approved files, firmware included. Picking "newest in the folder" is easy to get wrong now that the set of files is larger.

## What we already saw in your R11-14-3 commits

- You bumped the current release numbers: Windows installer R11-14-3 (title bar 11.14.3), `mscc` 1.0.95, Avalonia 1.7.02. `MSCC.aip` AppVersion and SetupFileName are updated. We will treat those as the current release.
- `stage.ps1`, `rpi/mscc-deb/build-deb.sh`, `postinst` and `linux-build/build-mscc-deb-amd64.sh` were not changed. The plan below still applies.
- `MSCC.aip` still has the absolute `C:\Users\Ron\.grok\worktrees` paths (SetupFolder and SourceDir). That is one of the items below.
- Thanks for the 0x9D reply. We will add paddle Farnsworth forwarding to the Multus IQ firmware batch and rebuild, so those images will not be pinned until Stew has tested the new set. The images already built on Shack (PL 1.237 / 6.237, PTT 3.245 / 8.245, ATU 4.246 / 7.246, GL 5.154, GM 2.155) do not have 0x9D yet and are not committed.

## Phase 1 (Stew / Build on Shack)

A release manifest and a read-only check-release script, in PowerShell and in bash. The manifest would name:

- the release tag
- the pinned firmware version for each radio (the 8 images)
- the keyer date
- the BlackPill version

The check script only reports what is missing or does not match. It does not copy or delete anything.

## Phase 2

`stage.ps1` copies only the pinned versions, fails loudly when something is missing, and clears unpinned files from the payload. The `.aip` gets a pre-install cleanup of the old firmware folders under `C:\mscc-net9`, which holds about 65 leftover files from earlier releases.

## Proposals for your side (your call)

We will stay hands-off: no edits to `rpi/`, `Solidus/` or `installers/rpi/`. These items are proposals only.

- `postinst` removes stale `~/.local/share/applications/mscc-{start,status,stop,bootloader}.desktop` and old `~/mscc/*.sh` and `bootloader-gui`, or ships a compat symlink. stew-HP still has a leftover desktop file, and uses a symlink `~/mscc/bootloader-gui` -> `/usr/share/mscc/bin/psoc-bootloader.gui`.
- `build-deb.sh` / `control` read the firmware versions from the manifest.
- Decide how amd64 shares the Pi version number. Today the amd64 script reuses the Pi `control` version (now 1.0.95) with 10 text substitutions.
- `MSCC.aip` has those absolute `C:\Users\Ron\.grok\worktrees` paths, and only `mscc-ui/installer/windows` is current of the three copies.
- About 38 tracked `.deb` files and about 360 Keil `Debug/` files are in git. (You already have "git-ignore the Visual Studio build bookkeeping files in mscc-ui/windows-work-tree" on the With Stew list; this is a related leftover.)
- `stage.ps1` expects `psoc-replacement-stm32` inside the repo, but on Shack it sits beside the repo.

## Questions

- Are you OK with the manifest approach?
- We will pin Multus IQ / cmd-080 firmware only after 0x9D is in and Stew has marked each image tested. Does that match what you want?
- Anything you are already doing that overlaps?
