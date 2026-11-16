# cmd-084 — firmware folders: current only, retire the dated drop folder

**Approved by Stew on 2026-11-16 (t944u): OK with all of it**, including the decisions below. Do this on Shack in `C:\Users\n8vet\OneDrive\Documents\github\mscc-station`. Follow the repo-root `CLAUDE.md` "finished" rules. **Do not push.** cmd-080/080b is already committed as `dce2d93a` (`main` is **ahead 1** of `origin/main`); **do not amend** that commit and do not make a second cmd-080b commit. **Do not touch** `rpi/` or `installers/rpi/` (Ron's). **Do not touch** `radio-psoc-firmware/Proficio-Legacy - Copy/` (Ron's; the note asks him to `git rm -r` it). Do not `git add -A`. Do not stage Keil `Debug/` / `DP8051/`, `MSCC.aip`, or the dirty WPF payload DLLs. Do not run `stage.ps1`, do not build the Windows installer, do not build a `.deb`, and do not create a GitHub Release. Stop on any `git pull` conflict.

## Status of cmd-080b (already done — do not redo)

Build already committed cmd-080/080b on Shack at **2026-11-16 14:14 ET**:

- `dce2d93ad72564dd8148f4fcb967da94d4048ae7` — `cmd-080/080b: Multus IQ USB name + 0x9D Farnsworth forwarding (MKII)`
- `main` is **ahead 1** of `origin/main` (not pushed; Stew pushes).
- That commit already has the source, Addendum, CHANGELOG, the two briefs (as of then), the **final eight** images in each tree's `Release\` / `release\`, **and** the installer payload `PSoC Firmware` updated to only those eight. It also committed the intermediate test images (2.155, 3.245/8.245, 4.246/7.246).

So: **do not make a second cmd-080b commit, and do not amend `dce2d93a`.** This cmd-084 commit is a **new** commit after `dce2d93a`: folder cleanup and the copy into `C:\mscc-net9`. The earlier "leave untracked images out" rule is moot — the finals are already in git. This job also **`git rm`s the intermediate test images that were committed in `dce2d93a`** (Geminus MKII 2.155, Proficio/Ultimus MKII-PTT 3.245/8.245, Proficio/Ultimus MKII-ATU 4.246/7.246), plus the other dated and older versioned files, so only the current eight remain in each tree.

Working-tree leftovers that are **not** this job (leave them unstaged): `.mscc-coord/cmd-080b-brief.md` (later t941u edit), repo-root `MSCC.aip`, payload `MSCC.Wpf.*` / `MSCC.Core.dll`, and all `DP8051/Debug/` outputs.

## Goal

1. Copy the current eight PSoC images and the current keyer PIC hex files into `C:\mscc-net9` (the folder the Windows installer is built from: repo-root `MSCC.aip` has `SourceDir=C:\mscc-net9`). Match the existing subfolder names and the versioned `Radio-major.minor` / `Keyer-MCU-app|full-YYYYMMDD` naming. Then remove the older, dated, and unversioned firmware from `C:\mscc-net9\PSoC Firmware` so the installer cannot pick up leftovers. `.gitkeep` files may stay.
2. The repo placeholder `radio-psoc-firmware/release/` (dated drop folder, retired 2026-11-02, kept as an archive) is no longer needed. Remove it from git after the backup zip.
3. Unversioned / undated images (`Radio.cyacd` / `Radio.hex`, no date and no major.minor) move to `radio-psoc-firmware/original/` (the folder exists, is empty, and is not yet tracked).
4. Dated images and earlier versioned images are removed from the trees. Only the current release stays in each tree's `Release\` / `release\`:

   - Proficio MKII-PTT 3.246
   - Ultimus MKII-PTT 8.246
   - Proficio MKII-ATU 4.247
   - Ultimus MKII-ATU 7.247
   - Geminus MKII 2.156
   - Proficio Legacy 1.237
   - Ultimus Legacy 6.237
   - Geminus Legacy 5.154 (built, not hardware-tested)

   Current keyer PIC (keep only these two in `keyer-buffer/Release\`, copy into `C:\mscc-net9\Keyer Firmware\`):

   - `Keyer-MCU-app-20261112.hex` — app, for `keyer-flash` over USB
   - `Keyer-MCU-full-20261112.hex` — bootloader + app, for PICkit once

   Stew approved (t944u) removing the other hex files in that `Release\` folder (older dated images, the four `Keyer-MCU-backup-before-*.hex` dumps, and `Keyer-MCU-full-20261112-boot1.hex`), after they are in the backup zip.

5. Point the Linux `mscc-firmware` packager at the latest versioned tree images (it still reads the retired dated `release/` folder today). Pi package: see the Ron note; do not edit `rpi/`.

## 1. Pull first (local is already ahead 1)

```
git fetch
git status -sb
git log -1 --oneline
```

HEAD should be `dce2d93a` (or that commit plus this work). If `origin/main` has new commits, `git pull --rebase --autostash`. If a conflict appears, **stop** and report. Do not reset or rebase away `dce2d93a`.

## 2. Backup zip first (before any delete)

Create `C:\mscc-net9-fw-backup-2026-11-16\` (outside the repo) and put one zip in it, for example `fw-backup-before-cmd-084.zip`, containing at least:

- All of `C:\mscc-net9\PSoC Firmware\` (the 60 files as they are now, including the leftover `Proficio-MKII\` folder)
- `C:\mscc-net9\Keyer Firmware\Keyer-MCU-app-20261112.hex` and `Keyer-MCU-full-20261112.hex`
- Every repo file this brief will `git rm` or `git mv` (tree dated and older versioned images including the intermediate test images from `dce2d93a`, the whole `radio-psoc-firmware/release\` tree, the unversioned files that will move to `original\`, and the eight keyer hex files listed in section 5 that will be removed from `keyer-buffer/Release\`)

Do not delete anything until the zip is written and you have listed its contents. If the zip fails, **stop**.

## 3. Copy current firmware into C:\mscc-net9 (not a git action)

`C:\mscc-net9\PSoC Firmware` already uses the payload-style folder names (`Geminus-legacy`, `Geminus-MKII`, `Proficio-legacy`, `Proficio-MKII-ATU`, `Proficio-MKII-PTT`, `Ultimus-Legacy`, `Ultimus-MKII-ATU`, `Ultimus-MKII-PTT`). It still has **older** versioned files (3.244, 4.245, 2.154, 1.236, 5.153, 6.236, 7.245, 8.244), dated `YYYYMMDD` copies, and unversioned `Radio.cyacd` / `.hex`. None of the current eight are there yet. The installer source is this folder, so leftovers would be packed.

Copy these 16 files from the repo trees, replacing any same-named file:

| Destination under `C:\mscc-net9\PSoC Firmware\` | Source |
|---|---|
| `Proficio-MKII-PTT\Proficio-MKII-PTT-3.246.cyacd` / `.hex` | `radio-psoc-firmware\Proficio-Ultimus-MKII-PTT\Release\` |
| `Ultimus-MKII-PTT\Ultimus-MKII-PTT-8.246.cyacd` / `.hex` | same tree `Release\` |
| `Proficio-MKII-ATU\Proficio-MKII-ATU-4.247.cyacd` / `.hex` | `radio-psoc-firmware\Proficio-Ultimus-MKII-ATU\Release\` |
| `Ultimus-MKII-ATU\Ultimus-MKII-ATU-7.247.cyacd` / `.hex` | same tree `Release\` |
| `Geminus-MKII\Geminus-MKII-2.156.cyacd` / `.hex` | `radio-psoc-firmware\Geminus-MKII\release\` |
| `Proficio-legacy\Proficio-Legacy-1.237.cyacd` / `.hex` | `radio-psoc-firmware\Proficio-Ultimus-Legacy\Release\` |
| `Ultimus-Legacy\Ultimus-Legacy-6.237.cyacd` / `.hex` | same tree `Release\` |
| `Geminus-legacy\Geminus-Legacy-5.154.cyacd` / `.hex` | `radio-psoc-firmware\Geminus-Legacy\release\` |

Also copy (overwrite is fine; these two are already the current files there):

- `keyer-buffer\Release\Keyer-MCU-app-20261112.hex` → `C:\mscc-net9\Keyer Firmware\`
- `keyer-buffer\Release\Keyer-MCU-full-20261112.hex` → `C:\mscc-net9\Keyer Firmware\`

Then, only under `C:\mscc-net9\PSoC Firmware\`, delete the dated, unversioned, and older-versioned `.cyacd` / `.hex` (the 52 old images, including the four in the leftover `Proficio-MKII\` folder). Remove the leftover `Proficio-MKII\` folder if it is then empty. Leave `.gitkeep`, and do not touch `C:\mscc-net9\factory\`, `Black Pill Firmware\`, or the keyer `.exe` tools.

When you are done, each of the eight radio folders should contain only the current versioned pair (plus `.gitkeep` if present).

## 4. Repo: move unversioned images to original/

`radio-psoc-firmware/original/` exists and is empty (not tracked until files are added). `git mv` these six tracked unversioned files (no date, no major.minor):

```
git mv "radio-psoc-firmware/Proficio-Ultimus-Legacy/Release/Proficio-Legacy.cyacd" radio-psoc-firmware/original/Proficio-Ultimus-Legacy/Proficio-Legacy.cyacd
git mv "radio-psoc-firmware/Proficio-Ultimus-Legacy/Release/Proficio-Legacy.hex" radio-psoc-firmware/original/Proficio-Ultimus-Legacy/Proficio-Legacy.hex
git mv "radio-psoc-firmware/Proficio-Ultimus-MKII-ATU/Release/Proficio-MKII-ATU.cyacd" radio-psoc-firmware/original/Proficio-Ultimus-MKII-ATU/Proficio-MKII-ATU.cyacd
git mv "radio-psoc-firmware/Proficio-Ultimus-MKII-ATU/Release/Proficio-MKII-ATU.hex" radio-psoc-firmware/original/Proficio-Ultimus-MKII-ATU/Proficio-MKII-ATU.hex
git mv "radio-psoc-firmware/Proficio-Ultimus-MKII-PTT/Release/Proficio-MKII-PTT.cyacd" radio-psoc-firmware/original/Proficio-Ultimus-MKII-PTT/Proficio-MKII-PTT.cyacd
git mv "radio-psoc-firmware/Proficio-Ultimus-MKII-PTT/Release/Proficio-MKII-PTT.hex" radio-psoc-firmware/original/Proficio-Ultimus-MKII-PTT/Proficio-MKII-PTT.hex
```

Create the destination folders first if `git mv` needs them. Add a short `radio-psoc-firmware/original/README.md` saying these are unversioned / undated snapshots, not shipping images; shipping images live in each tree's `Release\` as `<Radio>-<major>.<minor>`.

There are no unversioned Ultimus or Geminus images in the tree `Release\` folders. Do **not** move Keil `Debug\` `Project.hex` / `.cyacd` (build outputs).

**Leave alone** (open questions, not in this commit unless Stew already said yes): the whole `radio-psoc-firmware/Proficio-Legacy - Copy\` tree (a tracked ~630-file snapshot, including unversioned and `Proficio-Legacy-20261026` images).

## 5. Repo: git rm dated and older versioned images

Keep only the eight current pairs in the tree `Release\` / `release\` folders. `git rm` the files below (94 image files in the trees + the retired top-level drop folder).

### Older versioned (36 files) — git rm

```
radio-psoc-firmware/Geminus-Legacy/release/Geminus-Legacy-5.153.cyacd
radio-psoc-firmware/Geminus-Legacy/release/Geminus-Legacy-5.153.hex
radio-psoc-firmware/Geminus-MKII/release/Geminus-MKII-2.153.cyacd
radio-psoc-firmware/Geminus-MKII/release/Geminus-MKII-2.153.hex
radio-psoc-firmware/Geminus-MKII/release/Geminus-MKII-2.154.cyacd
radio-psoc-firmware/Geminus-MKII/release/Geminus-MKII-2.154.hex
radio-psoc-firmware/Geminus-MKII/release/Geminus-MKII-2.155.cyacd
radio-psoc-firmware/Geminus-MKII/release/Geminus-MKII-2.155.hex
radio-psoc-firmware/Proficio-Ultimus-Legacy/Release/Proficio-Legacy-1.236.cyacd
radio-psoc-firmware/Proficio-Ultimus-Legacy/Release/Proficio-Legacy-1.236.hex
radio-psoc-firmware/Proficio-Ultimus-Legacy/Release/Ultimus-Legacy-6.236.cyacd
radio-psoc-firmware/Proficio-Ultimus-Legacy/Release/Ultimus-Legacy-6.236.hex
radio-psoc-firmware/Proficio-Ultimus-MKII-ATU/Release/Proficio-MKII-ATU-4.244.cyacd
radio-psoc-firmware/Proficio-Ultimus-MKII-ATU/Release/Proficio-MKII-ATU-4.244.hex
radio-psoc-firmware/Proficio-Ultimus-MKII-ATU/Release/Proficio-MKII-ATU-4.245.cyacd
radio-psoc-firmware/Proficio-Ultimus-MKII-ATU/Release/Proficio-MKII-ATU-4.245.hex
radio-psoc-firmware/Proficio-Ultimus-MKII-ATU/Release/Proficio-MKII-ATU-4.246.cyacd
radio-psoc-firmware/Proficio-Ultimus-MKII-ATU/Release/Proficio-MKII-ATU-4.246.hex
radio-psoc-firmware/Proficio-Ultimus-MKII-ATU/Release/Ultimus-MKII-ATU-7.244.cyacd
radio-psoc-firmware/Proficio-Ultimus-MKII-ATU/Release/Ultimus-MKII-ATU-7.244.hex
radio-psoc-firmware/Proficio-Ultimus-MKII-ATU/Release/Ultimus-MKII-ATU-7.245.cyacd
radio-psoc-firmware/Proficio-Ultimus-MKII-ATU/Release/Ultimus-MKII-ATU-7.245.hex
radio-psoc-firmware/Proficio-Ultimus-MKII-ATU/Release/Ultimus-MKII-ATU-7.246.cyacd
radio-psoc-firmware/Proficio-Ultimus-MKII-ATU/Release/Ultimus-MKII-ATU-7.246.hex
radio-psoc-firmware/Proficio-Ultimus-MKII-PTT/Release/Proficio-MKII-PTT-3.243.cyacd
radio-psoc-firmware/Proficio-Ultimus-MKII-PTT/Release/Proficio-MKII-PTT-3.243.hex
radio-psoc-firmware/Proficio-Ultimus-MKII-PTT/Release/Proficio-MKII-PTT-3.244.cyacd
radio-psoc-firmware/Proficio-Ultimus-MKII-PTT/Release/Proficio-MKII-PTT-3.244.hex
radio-psoc-firmware/Proficio-Ultimus-MKII-PTT/Release/Proficio-MKII-PTT-3.245.cyacd
radio-psoc-firmware/Proficio-Ultimus-MKII-PTT/Release/Proficio-MKII-PTT-3.245.hex
radio-psoc-firmware/Proficio-Ultimus-MKII-PTT/Release/Ultimus-MKII-PTT-8.243.cyacd
radio-psoc-firmware/Proficio-Ultimus-MKII-PTT/Release/Ultimus-MKII-PTT-8.243.hex
radio-psoc-firmware/Proficio-Ultimus-MKII-PTT/Release/Ultimus-MKII-PTT-8.244.cyacd
radio-psoc-firmware/Proficio-Ultimus-MKII-PTT/Release/Ultimus-MKII-PTT-8.244.hex
radio-psoc-firmware/Proficio-Ultimus-MKII-PTT/Release/Ultimus-MKII-PTT-8.245.cyacd
radio-psoc-firmware/Proficio-Ultimus-MKII-PTT/Release/Ultimus-MKII-PTT-8.245.hex
```

### Dated images in the tree Release folders (30 files) — git rm

```
radio-psoc-firmware/Proficio-Ultimus-Legacy/Release/Proficio-Legacy-20260814.cyacd
radio-psoc-firmware/Proficio-Ultimus-Legacy/Release/Proficio-Legacy-20260814.hex
radio-psoc-firmware/Proficio-Ultimus-Legacy/Release/Proficio-Legacy-20260817.cyacd
radio-psoc-firmware/Proficio-Ultimus-Legacy/Release/Proficio-Legacy-20260817.hex
radio-psoc-firmware/Proficio-Ultimus-Legacy/Release/Proficio-Legacy-20260822.cyacd
radio-psoc-firmware/Proficio-Ultimus-Legacy/Release/Proficio-Legacy-20260822.hex
radio-psoc-firmware/Proficio-Ultimus-Legacy/Release/Proficio-Legacy-20261031.cyacd
radio-psoc-firmware/Proficio-Ultimus-Legacy/Release/Proficio-Legacy-20261031.hex
radio-psoc-firmware/Proficio-Ultimus-MKII-ATU/Release/Proficio-MKII-ATU-20260816.cyacd
radio-psoc-firmware/Proficio-Ultimus-MKII-ATU/Release/Proficio-MKII-ATU-20260816.hex
radio-psoc-firmware/Proficio-Ultimus-MKII-ATU/Release/Proficio-MKII-ATU-20260817.cyacd
radio-psoc-firmware/Proficio-Ultimus-MKII-ATU/Release/Proficio-MKII-ATU-20260817.hex
radio-psoc-firmware/Proficio-Ultimus-MKII-ATU/Release/Proficio-MKII-ATU-20260822.cyacd
radio-psoc-firmware/Proficio-Ultimus-MKII-ATU/Release/Proficio-MKII-ATU-20260822.hex
radio-psoc-firmware/Proficio-Ultimus-MKII-ATU/Release/Proficio-MKII-ATU-20261031.cyacd
radio-psoc-firmware/Proficio-Ultimus-MKII-ATU/Release/Proficio-MKII-ATU-20261031.hex
radio-psoc-firmware/Proficio-Ultimus-MKII-ATU/Release/Proficio-MKII-ATU-20261102.cyacd
radio-psoc-firmware/Proficio-Ultimus-MKII-ATU/Release/Proficio-MKII-ATU-20261102.hex
radio-psoc-firmware/Proficio-Ultimus-MKII-PTT/Release/Proficio-MKII-PTT-20260814.cyacd
radio-psoc-firmware/Proficio-Ultimus-MKII-PTT/Release/Proficio-MKII-PTT-20260814.hex
radio-psoc-firmware/Proficio-Ultimus-MKII-PTT/Release/Proficio-MKII-PTT-20260817.cyacd
radio-psoc-firmware/Proficio-Ultimus-MKII-PTT/Release/Proficio-MKII-PTT-20260817.hex
radio-psoc-firmware/Proficio-Ultimus-MKII-PTT/Release/Proficio-MKII-PTT-20260822.cyacd
radio-psoc-firmware/Proficio-Ultimus-MKII-PTT/Release/Proficio-MKII-PTT-20260822.hex
radio-psoc-firmware/Proficio-Ultimus-MKII-PTT/Release/Proficio-MKII-PTT-20261019.cyacd
radio-psoc-firmware/Proficio-Ultimus-MKII-PTT/Release/Proficio-MKII-PTT-20261019.hex
radio-psoc-firmware/Proficio-Ultimus-MKII-PTT/Release/Proficio-MKII-PTT-20261021.cyacd
radio-psoc-firmware/Proficio-Ultimus-MKII-PTT/Release/Proficio-MKII-PTT-20261021.hex
radio-psoc-firmware/Proficio-Ultimus-MKII-PTT/Release/Proficio-MKII-PTT-20261022.cyacd
radio-psoc-firmware/Proficio-Ultimus-MKII-PTT/Release/Proficio-MKII-PTT-20261022.hex
```

### Retired top-level drop folder (37 files) — git rm -r

```
git rm -r radio-psoc-firmware/release
```

That folder is 28 dated `.cyacd`/`.hex`, 8 `.gitkeep`, and `README.md`. It is already marked retired in its README (2026-11-02). After this, shipping images live only in each tree's `Release\` / `release\`.

### Leave in the trees (16 files — do not rm)

`Geminus-Legacy-5.154`, `Geminus-MKII-2.156`, `Proficio-Legacy-1.237`, `Ultimus-Legacy-6.237`, `Proficio-MKII-ATU-4.247`, `Ultimus-MKII-ATU-7.247`, `Proficio-MKII-PTT-3.246`, `Ultimus-MKII-PTT-8.246` (each `.cyacd` + `.hex`). Those eight include the finals already committed in `dce2d93a`; the intermediate test images from that same commit (2.155, 3.245/8.245, 4.246/7.246) are in the older-versioned `git rm` list above.

### Keyer extras in `keyer-buffer/Release\` (8 files) — git rm (Stew t944u: yes)

Keep only `Keyer-MCU-app-20261112.hex` and `Keyer-MCU-full-20261112.hex`. After they are in the backup zip, `git rm` these eight:

```
git rm keyer-buffer/Release/Keyer-MCU-20260814.hex
git rm keyer-buffer/Release/Keyer-MCU-buffer-20261105.hex
git rm keyer-buffer/Release/Keyer-MCU-buffer-stream-20261110.hex
git rm keyer-buffer/Release/Keyer-MCU-backup-before-20261105.hex
git rm keyer-buffer/Release/Keyer-MCU-backup-before-boot-20261112.hex
git rm keyer-buffer/Release/Keyer-MCU-backup-before-buffer-20261105.hex
git rm keyer-buffer/Release/Keyer-MCU-backup-before-stream-20261110.hex
git rm keyer-buffer/Release/Keyer-MCU-full-20261112-boot1.hex
```

The four `Keyer-MCU-backup-before-*.hex` files are 93,705-byte radio dumps; they belong in the backup zip even though they leave git. `Keyer-MCU-full-20261112-boot1.hex` is the same date as the current full image but different bytes; it is not used by `stage.ps1`.

### Leave alone (Build does not touch)

- **`radio-psoc-firmware/Proficio-Legacy - Copy\`** — Stew says this was a working folder for testing new firmware and should be deleted, but it is Ron's, so **Build does not touch it**. The Ron note asks him to `git rm -r` that folder.
- `linux/mscc-firmware-deb/mscc-firmware_1.0.*.deb` (1.0.0–1.0.2) — leftover packages. Stew said leave them (t944u). Do not build a new one in this job.
- `C:\mscc-net9\factory\` and payload `factory\` (calibration INI, not PSoC images)
- PSoC bootloader trees, Black Pill, `Debug\` / `DP8051\`
- Standalone keyer bootloader build output: `keyer-buffer/bootloader/README.md` says `build.bat` produces `build\keyer-boot.hex`. There is no dated bootloader-only hex in `Release\`; the shipping bootloader is inside `Keyer-MCU-full-20261112.hex`. Do not invent a new file.

## 6. Script and doc path updates (required)

These **will break** if `radio-psoc-firmware/release/` is removed and they are not updated:

- `linux-build/build-mscc-firmware-deb.sh` — today requires **exactly one dated** `*-YYYYMMDD.cyacd` / `.hex` pair per radio under `radio-psoc-firmware/release/<RadioName>/`. Change it to pack the **newest** (and, after this cleanup, the only) `<Radio>-<major>.<minor>.cyacd` / `.hex` from each tree's `Release\` (Geminus: `release\`), the same mapping `stage.ps1` and `rpi/mscc-deb/build-deb.sh` already use. Accept either `Release` or `release`. Do not require dated names. Do not run the script and do not add a new `.deb`.
- `linux/mscc-firmware-deb/README.md` — rewrite the "one dated pair from radio-psoc-firmware/release/" description to match the new source (latest versioned tree images; dest is still `/usr/share/mscc/firmware/<RadioName>/`).
- `radio-psoc-firmware/README.md` — the layout row that calls top-level `release/` an archive, and the stale "shipping artifacts under `release/`" status table, need to say that folder is gone and current images are only in each tree's `Release\` / `release\`.

These do **not** need a path change (already correct), but look at them and confirm in the report:

- `mscc-ui/installer/windows/stage.ps1` — already copies the newest `<Radio>-<major>.<minor>` from each tree `Release\` into payload, and the newest `Keyer-MCU-app-<date>.hex` + matching `-full-` from `keyer-buffer\Release\`. After cleanup it still works. **Do not run it** (it also refreshes client/server exes).
- `radio-psoc-firmware/copy-release.py` and each tree's `copy-release.bat` — already write versioned files into the tree `Release\` only. No change.
- Repo-root `MSCC.aip` — `SourceDir=C:\mscc-net9`. No path change; step 3 refreshes that folder. Do not stage `MSCC.aip`.
- `rpi/mscc-deb/build-deb.sh` — already reads the Windows payload (one versioned pair per radio + current keyer). Do not edit `rpi/`. The note for Ron is `.mscc-coord/NOTE-FOR-RON-FIRMWARE-FOLDERS-2026-11-16.md`.
- Bootloader GUI (`firmware-programmers/*/psoc-usb-bootload/psoc-bootloader.gui`) — Load File opens `/usr/share/mscc/firmware`. No repo-folder path to change. Operator docs (`docs/manuals/MSCC-Updating-Firmware.md`, Addendum) already point at `C:\mscc-net9\PSoC Firmware\<Radio>\` and `/usr/share/mscc/firmware/<Radio>/` without a dated filename. Leave them unless a sentence still tells people to use `radio-psoc-firmware/release/`.

## 7. Stage, verify, commit (do not push). Then stop.

```
git add radio-psoc-firmware/original
git add linux-build/build-mscc-firmware-deb.sh
git add linux/mscc-firmware-deb/README.md
git add radio-psoc-firmware/README.md
git add .mscc-coord/cmd-084-firmware-folders-brief.md
git add .mscc-coord/NOTE-FOR-RON-FIRMWARE-FOLDERS-2026-11-16.md
```

(The `git mv` / `git rm` above already stage those paths.)

```
git diff --cached --stat
git diff --cached --name-only
```

Check: the index has the 16 current tree images still present (not deleted), the 6 unversioned files under `original/`, the retired `release/` folder gone, the eight keyer extras removed and only `Keyer-MCU-app-20261112.hex` and `Keyer-MCU-full-20261112.hex` left in `keyer-buffer/Release\`, the script + README updates, this brief, and the Ron note. It must **not** contain `Debug\`, `DP8051\`, `MSCC.aip`, payload WPF/Core DLLs, `rpi/`, `installers/rpi/`, `Proficio-Legacy - Copy`, or a new `.deb`. If anything extra is staged, `git restore --staged` it and check again.

Message:

```
cmd-084: keep only current PSoC/keyer firmware images

Retire radio-psoc-firmware/release/ (the dated drop folder). Each tree's
Release/ (Geminus: release/) keeps only the current eight:

- Proficio MKII-PTT 3.246, Ultimus MKII-PTT 8.246
- Proficio MKII-ATU 4.247, Ultimus MKII-ATU 7.247
- Geminus MKII 2.156, Geminus Legacy 5.154 (not hardware-tested)
- Proficio Legacy 1.237, Ultimus Legacy 6.237

Unversioned Radio.cyacd/.hex moved to radio-psoc-firmware/original/.
Intermediate test images from dce2d93a (2.155, 3.245/8.245, 4.246/7.246)
are removed. keyer-buffer/Release/ keeps only Keyer-MCU-app-20261112.hex
and Keyer-MCU-full-20261112.hex.

linux-build/build-mscc-firmware-deb.sh now packs those latest versioned
tree images (it used to require one dated pair in the retired folder).

Current images and keyer hex were copied into C:\mscc-net9\PSoC Firmware
and Keyer Firmware for the Windows installer Stew builds from that folder.
Older files there were backed up to C:\mscc-net9-fw-backup-2026-11-16
and removed.

With Ron
- Please delete radio-psoc-firmware/Proficio-Legacy - Copy/ (git rm -r).
  Stew says it was a working folder for testing new firmware. We left
  it for you; details in
  .mscc-coord/NOTE-FOR-RON-FIRMWARE-FOLDERS-2026-11-16.md
- Pi mscc package: please rebuild so /usr/share/mscc/firmware/<Radio>/
  and /usr/share/mscc/firmware/keyer/ carry only the current images
  above (and Keyer-MCU-app/full-20261112). Your build-deb.sh already
  reads the Windows payload, which already has these.
- The Linux mscc-firmware packager (linux-build/build-mscc-firmware-deb.sh)
  is updated in this commit to use the tree Release/ versioned files.
  We did not build a new .deb here.
- Geminus Legacy 5.154 is still not hardware-tested (no radio).
```

Then **stop**. Do not push.

## Stew's decisions (t944u, 2026-11-16) — already applied in this brief

1. **`radio-psoc-firmware/Proficio-Legacy - Copy\`** — delete it, but Ron does that (`git rm -r`). It was a working folder for testing new firmware. **Build does not touch it.** The ask is in the Ron note.
2. **Keyer `Release\` extras** — yes, `git rm` them after the backup zip. Keep only `Keyer-MCU-app-20261112.hex` and `Keyer-MCU-full-20261112.hex`.
3. **Old `linux/mscc-firmware-deb/*.deb`** (1.0.0–1.0.2) — leave them. Do not build a new one in this job.
