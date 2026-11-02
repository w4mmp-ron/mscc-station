# Radio PSoC firmware

PSoC Creator / Keil trees for Multus radio application firmware. Moved here so the repo root stays cleaner.

**Host:** build and flash work on **Shack** (only Keil license). Do not plan PSoC compile on NEW-HP, stew-HP, or Pi.

## Layout

| Path | Radio / role | Current `FIRMWARE_VERSION_MAJOR` (as of 2026-09-20) |
|------|----------------|------------------------------------------------------|
| `Proficio-Legacy/` | Proficio Legacy | **1** |
| `Proficio-MKII-PTT/` | Proficio MKII PTT | **3** |
| `Proficio-MKII-ATU/` | Proficio MKII ATU | **4** |
| `Proficio-bootloader/` | Shared Proficio Creator bootloader project | (LOADER; MiniProg3 only) |
| `Geminus-Legacy/` | Geminus Legacy | **5** |
| `Geminus-MKII/` | Geminus MKII | **2** |
| `Ultimus-Legacy/` | Ultimus Legacy — clone of Proficio-Legacy; Creator project still named Proficio-Legacy | **6** |
| `Ultimus-MKII-PTT/` | Ultimus MKII PTT — clone of Proficio-MKII-PTT; Creator project still named Proficio-MKII-PTT | **8** |
| `Ultimus-MKII-ATU/` | Ultimus MKII ATU — clone of Proficio-MKII-ATU; Creator project still named Proficio-MKII-ATU | **7** |
| `<Tree>/Release/` | **Releases (2026-11-02):** `<Tree>-<major>.<minor>.cyacd` / `.hex`, see below | — |
| `release/` | Old drop folder, **no longer written** (kept as an archive) | — |

Planned major map (client header / factory cal key): **1** Proficio Legacy · **2** Geminus MKII · **3/4** Proficio MKII PTT/ATU · **5** Geminus Legacy · **6** Ultimus Legacy · **7** Ultimus MKII ATU · **8** Ultimus MKII PTT.

## Releases: version in the file name, in the tree's own `Release/` (2026-11-02)

Every build is saved under the version it reports, so two builds on the same day never overwrite
each other and nothing has to be dug out of `DP8051\...\Debug`:

- **Proficio / Geminus:** build in PSoC Creator. The post-build (`<Project>.cydsn\post-build-release.bat`
  -> `<Tree>\copy-release.bat` -> `copy-release.py`) copies the build to
  `<Tree>\Release\<Tree>-<major>.<minor>.cyacd` / `.hex`, e.g. `Proficio-MKII-ATU\Release\Proficio-MKII-ATU-4.244.cyacd`.
  Double-click `<Tree>\copy-release.bat` if the post-build didn't run.
- **Ultimus:** `build-ultimus.bat` writes into the Proficio tree it builds from, e.g.
  `Proficio-MKII-ATU\Release\Ultimus-MKII-ATU-7.244.cyacd`.
- **No silent overwrite:** if that version's file already exists with different contents, the new
  build is saved as `...-4.244-b2` and you're told to bump `FIRMWARE_VERSION_MINOR`.
  **Bump the minor for every build you flash for testing.**

## Ultimus builds (from the Proficio trees, 2026-10-31)

Ultimus radios are hand-buildable (through-hole) Proficios: same firmware, different major
(6 Legacy, 8 MKII PTT, 7 MKII ATU) and USB product string "Ultimus". They are built from the
Proficio trees, so a Proficio fix is an Ultimus fix:

1. Build the Proficio project in PSoC Creator (Shift+F6).
2. Run `build-ultimus.bat` (or `py -3 build-ultimus.py legacy|mkii-ptt|mkii-atu|all`).
3. Output: `<Proficio tree>/Release/Ultimus-<radio>-<major>.<minor>.cyacd` / `.hex`.

The script works in a git-ignored copy (`_ultimus-build/`), recompiles the application with
`DF(ULTIMUS)` (the `#ifdef ULTIMUS` in `basic-plus.h` picks the major), patches the USB product
string, and links / makes the .cyacd with the same tools and options as Creator. A tree without
the `#ifdef ULTIMUS` switch is skipped. Done so far: **Legacy**. The old `Ultimus-*` source trees
are frozen copies from 2026-09-20 and are retired as each Proficio tree gets the switch.

## Upload tools (not this folder)

| Host | CLI + GUI |
|------|-----------|
| Raspberry Pi | `firmware-programmers/rpi/psoc-usb-bootload/` |
| Ubuntu x86_64 | `firmware-programmers/debian/psoc-usb-bootload/` |

Field flash notes: each Proficio tree still has `STEW-FIRMWARE-UPDATE.md`.

## STM32 / other

`proficio-stm32f411-*`, Solidus, etc. stay at **repo root** — not PSoC Creator radio apps.

## Status (2026-09-20)

Majors applied and rebuilt on Shack; shipping artifacts under `release/`:

| Radio | Major.Minor |
|-------|------------:|
| Proficio Legacy | 1.231 |
| Geminus MKII | 2.151 |
| Proficio MKII PTT | 3.232 |
| Proficio MKII ATU | 4.151 |
| Geminus Legacy | 5.120 |
| Ultimus Legacy | 6.231 |
| Ultimus MKII ATU | 7.151 |
| Ultimus MKII PTT | 8.232 |

Creator project folders under Ultimus trees are still named `Proficio-*` (rename deferred). See `GROK-BUILD-FIRMWARE-MAJORS.md` for history / related client work.