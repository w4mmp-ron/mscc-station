# For Ron: PSoC firmware folders renamed, Ultimus trees removed, releases named by version

From Stew, 2026-11-02. This affects your local clone and any Grok / Claude worktree paths for
the PSoC firmware. The Pi side doesn't reference these folders, so nothing changes there.

## Folders: one per radio family

The Ultimus radios run Proficio firmware. Since 10-31, `build-ultimus.py` builds them from the
Proficio source, so the old separate `Ultimus-*` source trees (frozen copies from 9/20) were
deleted. The Proficio trees were renamed to say that they cover both:

| Old | New | Builds |
|---|---|---|
| `Proficio-Legacy/` | `Proficio-Ultimus-Legacy/` | Proficio Legacy 1.x, Ultimus Legacy 6.x |
| `Proficio-MKII-PTT/` | `Proficio-Ultimus-MKII-PTT/` | Proficio MKII PTT 3.x, Ultimus MKII PTT 8.x |
| `Proficio-MKII-ATU/` | `Proficio-Ultimus-MKII-ATU/` | Proficio MKII ATU 4.x, Ultimus MKII ATU 7.x |
| `Ultimus-Legacy/`, `Ultimus-MKII-PTT/`, `Ultimus-MKII-ATU/` | removed | |

- The Creator projects inside keep their names (`Proficio-MKII-ATU.cydsn`, and so on).
- If an Ultimus ever needs its own code, we'll port it to a new tree then.
- After you pull, open each project in Creator from its new folder: Recent Projects still has the
  old paths.

## Releases: the version is in the file name, in the tree's own `Release/`

We had builds overwriting each other: same-day dated names, and three places to look (your
per-tree `Release/`, Stew's top-level `release/`, and the Debug folder). Now:

- Every build goes to that tree's `Release/` as `<Radio>-<major>.<minor>.cyacd` / `.hex`. Example:
  `Proficio-Ultimus-MKII-ATU/Release/Proficio-MKII-ATU-4.244.cyacd`. The Ultimus image from the
  same tree goes beside it: `Ultimus-MKII-ATU-7.244.cyacd`.
- A shared `radio-psoc-firmware/copy-release.py` reads the version from `basic-plus.h`. Each tree's
  `copy-release.bat` (the Creator post-build) and `build-ultimus.py` call it.
- **Nothing gets overwritten.** If a file for that version already exists with different
  contents, the new build is saved as `...-b2` with a warning to bump the minor version. Please
  bump `FIRMWARE_VERSION_MINOR` for every build that gets flashed for testing.
- The Creator post-build lines use the full path to `copy-release.bat`, because `${ProjDir}` didn't
  work. On your PC, change the path in Project → Build Settings → User Commands → Post Build, or
  just double-click the tree's `copy-release.bat` after a build.
- The top-level `release/` folder is retired and kept as an archive.
- `Geminus-MKII` got a `copy-release.bat` too, giving `Geminus-MKII-<ver>`. It isn't wired as a
  post-build yet.

## Current releases (rebuilt today, all tested versions)

| Radio | File |
|---|---|
| Proficio Legacy / Ultimus Legacy | `Proficio-Legacy-1.236`, `Ultimus-Legacy-6.236` |
| Proficio MKII PTT / Ultimus MKII PTT | `Proficio-MKII-PTT-3.243`, `Ultimus-MKII-PTT-8.243` |
| Proficio MKII ATU / Ultimus MKII ATU | `Proficio-MKII-ATU-4.244`, `Ultimus-MKII-ATU-7.244` |

## The 12 kHz comb fade: parked

The live-slot fade you agreed to was tried twice (3.244 / 3.245, never committed):

- **3.244:** re-pointed the Buff TDs and played slots one ms late. That gave noise, and a stalled
  audio stream. My guess is that USB writes about 2 slots ahead, not 1.
- **3.245:** scaled the next slot in place. It killed RX and TX completely. Cause unknown.

Firmware is back at the versions above. Before another try we need the real USB/DAC slot timing:
which `SyncSOF_USB_Buffer()` index USB fills, compared with the Buff TD that's playing, and how
much time that leaves. If you know it from the Peaberry design, that would save a debug build.
