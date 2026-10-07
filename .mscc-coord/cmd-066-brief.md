# cmd-066 — mscc-firmware 1.0.1: one dated .cyacd + one dated .hex per radio (Linux)

Repo: /home/stew/Documents/GitHub/mscc-station on stew-HP. Do NOT touch rpi/, Solidus/, installers/rpi/.
STOP BEFORE COMMIT — show `git status` + `git diff` for review.

## Background (read-only findings)
- Payload source: `radio-psoc-firmware/release/<Radio>/`. It holds 4 files per radio: dated `<Name>-YYYYMMDD.{cyacd,hex}`
  plus undated copies of the same files. `copy-release.bat` (per firmware tree, Shack/Windows) writes both on purpose.
  Leave release/ and the .bat files unchanged; filter in the Linux packager only.
  The release README is `radio-psoc-firmware/release/README.md`; the builder installs it as `/usr/share/mscc/firmware/README.md`.
- `linux-build/build-mscc-firmware-deb.sh` today:
  - Copies every `*.cyacd`/`*.hex` it finds.
  - Probes the undated name `Proficio-MKII-PTT/Proficio-MKII-PTT.cyacd` (line 36).
  - Requires at least 32 files.
  - Copies the deb into installers/linux AND installers/rpi.
- Dated names (Geminus-MKII uses a lowercase `mkii`):
  Proficio-Legacy-20260822, Proficio-MKII-PTT-20260822, Proficio-MKII-ATU-20260822, Geminus-Legacy-20260920,
  Geminus-mkii-20260903, Ultimus-Legacy-20260920, Ultimus-MKII-ATU-20260920, Ultimus-MKII-PTT-20260920.
- References to undated names outside rpi/Solidus: only the builder probe (line 36) and old brief `.mscc-coord/briefs/cmd-063.md:73`
  (history; leave it). The bootloader GUI (`linux/helpers/bootloader-gui`) only opens the folder `/usr/share/mscc/firmware`, with no file
  names. INSTALL.md uses `mscc-firmware_*_all.deb` (no version). install-mscc.sh doesn't name firmware files. mscc-init: none found.
  The rest are the firmware project's own build outputs (.cyprj/.cywrk/copy-release.bat/POST-BUILD.txt). Leave those.
- dpkg upgrade: a file that was in 1.0.0 but isn't in 1.0.1 is removed from disk when 1.0.1 is installed over it (dpkg deletes
  obsolete files from the old package's file list; these aren't conffiles). No maintainer-script cleanup is needed.

## Changes
1. `linux/mscc-firmware-deb/packaging/DEBIAN/control`: `Version: 1.0.0` → `1.0.1`. Description: `Files: /usr/share/mscc/firmware/<RadioName>/<Name>-YYYYMMDD.cyacd and .hex
   (one of each per radio)`. Install line: `sudo apt install ./mscc-firmware_1.0.1_all.deb` (or `_*_all.deb`).
2. `linux-build/build-mscc-firmware-deb.sh`:
   - Probe: `need_file "$SRC/Proficio-MKII-PTT/Proficio-MKII-PTT-20260822.cyacd"`. Better: drop the hard-coded probe and rely on the per-radio check below.
   - Per radio, copy only dated files, exactly one of each type:
     ```bash
     shopt -s nullglob
     cy=( "$SRC/$radio"/*-[0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9].cyacd )
     hx=( "$SRC/$radio"/*-[0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9].hex )
     shopt -u nullglob
     if [[ ${#cy[@]} -ne 1 || ${#hx[@]} -ne 1 ]]; then
       echo "ERROR: $radio needs exactly one dated .cyacd and one dated .hex (got ${#cy[@]}/${#hx[@]})" >&2; exit 1
     fi
     # stem check: .cyacd and .hex must be the same build
     [[ "$(basename "${cy[0]}" .cyacd)" == "$(basename "${hx[0]}" .hex)" ]] || { echo "ERROR: $radio cyacd/hex date mismatch" >&2; exit 1; }
     install -m 644 "${cy[0]}" "${hx[0]}" "$DEST/$radio/"; copied=$((copied + 2))
     ```
   - Count check: `-ne 16` → `ERROR: expected 16 files`. Update the summary echo.
   - Stop copying into installers/rpi (off-limits for this task). Put that line behind an opt-in (`MSCC_FW_DROP_RPI=1`) and
     add a note in the review that Ron/the Pi side still has 1.0.0 in installers/rpi. Do not edit installers/rpi.
3. Payload README (`radio-psoc-firmware/release/README.md`, used only by this package): add a short "Packaged files" section saying the
   `mscc-firmware` deb ships one dated `.cyacd` + `.hex` per radio (`<Name>-YYYYMMDD.*`). Undated copies in release/ are not packaged.
   Pick the newest dated file and mark which file is for USB bootloader Load File. Keep the existing text.
   (Optional, cleaner: make a separate `linux/mscc-firmware-deb/README.md` the payload README and leave release/README.md unchanged. Pick one and say which.)
4. Build: `./linux-build/build-mscc-firmware-deb.sh` → `linux/mscc-firmware-deb/mscc-firmware_1.0.1_all.deb` + copy into installers/linux.
5. Old version, following how the repo tracks versions:
   - installers/linux keeps only the current kit (cmd-065 deleted the old 1.0.49/0.6.71 there), so `git rm installers/linux/mscc-firmware_1.0.0_all.deb`.
   - linux/mscc-deb keeps history debs, and so does linux/mscc-firmware-deb. Keep `linux/mscc-firmware-deb/mscc-firmware_1.0.0_all.deb` and add 1.0.1.
     If Stew wants it gone too, `git rm` it. Ask in the review; don't guess.
6. `installers/linux/CHANGELOG.md`: shipped table → `mscc-firmware ... | 1.0.1`. New section:
   ```
   ## mscc-firmware 1.0.1 — 2026-10-07
   ### Changed
   - One firmware file of each type per radio: only the dated `<Name>-YYYYMMDD.cyacd` and `.hex`. The undated duplicates (byte-identical) are gone; upgrading from 1.0.0 removes them.
   ```
   Mark `mscc-firmware 1.0.0` "(superseded by 1.0.1)".
7. `installers/linux/INSTALL.md`: uses the `mscc-firmware_*_all.deb` wildcard, so no version change is needed. Update only if it describes file names.

## Checks before install
```bash
D=installers/linux/mscc-firmware_1.0.1_all.deb
dpkg-deb -f $D Package Version Architecture      # mscc-firmware 1.0.1 all
dpkg-deb -c $D | grep -E '\.(cyacd|hex)$' | wc -l # 16
dpkg-deb -c $D | grep -E '\.(cyacd|hex)$' | grep -vE -- '-[0-9]{8}\.(cyacd|hex)$'   # empty
ls installers/linux installers/rpi                # linux: only 1.0.1; rpi: unchanged
```

## Smoke (stew-HP, upgrade over installed 1.0.0)
1. `sudo apt install ./installers/linux/mscc-firmware_1.0.1_all.deb`
2. `find /usr/share/mscc/firmware -type f | sort` → exactly 16 dated files (8 radios × .cyacd/.hex) + README.md, no undated files.
   `dpkg -L mscc-firmware | grep -cE '\.(cyacd|hex)$'` → 16.
3. Menu → Firmware Upload → **Load File**: the dialog opens in /usr/share/mscc/firmware with no "missing" warning, and each radio folder shows 2 dated files.
   Don't flash anything.

## STOP — no commit/push. Report the diff, deb listing, smoke output, and the 1.0.0-in-linux/mscc-firmware-deb + installers/rpi decisions.
