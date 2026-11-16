# cmd-080: USB product name "Multus IQ" in all radio firmware (Shack, PSoC Creator)

Source: `.mscc-coord/NOTE-FOR-STEW-WINDOWS-FIRMWARE-MULTUS-IQ-2026-11-13.md` (read it first).
Follow the repo-root `CLAUDE.md` rules: pull first, NOTHING goes into git until Stew says "finished".

## Goal
USB Device Descriptor iProduct (string index 14) = "Multus IQ" on every radio (was "Proficio",
or "Ultimus" via build-ultimus.py). Linux names the sound card from iProduct; Windows uses the
iInterface "Multus IQ Sound", which must NOT change.

## Projects (5) -> images (8), all under radio-psoc-firmware/
| Project | Now | New |
|---|---|---|
| Proficio-Ultimus-Legacy/Proficio-Legacy.cydsn | 1.236 / Ultimus 6.236 | 1.237 / 6.237 |
| Proficio-Ultimus-MKII-PTT/Proficio-MKII-PTT.cydsn | 3.244 / Ultimus 8.244 | 3.245 / 8.245 |
| Proficio-Ultimus-MKII-ATU/Proficio-MKII-ATU.cydsn | 4.245 / Ultimus 7.245 | 4.246 / 7.246 |
| Geminus-Legacy/Geminus.cydsn | 5.153 | 5.154 |
| Geminus-MKII/Geminus.cydsn | 2.154 | 2.155 |
Do not touch: `Proficio-Legacy - Copy`, `Proficio-bootloader`, `_ultimus-build`, `release/`, anything in rpi/ or linux/.

## Step 0 (Build): pre-check, no edits
- `git pull`; `git status` clean.
- For each project, confirm in `Generated_Source/PSoC3/USBFS_descr.c` that the string descriptor
  iProduct points to (index 14) is "Proficio" and that "Multus IQ Sound" is index 9. Report the table.
- Note: `TopDesign/TopDesign.cysch` is a binary file. Do NOT edit it with a script.

## Step 1 (STEW, by hand in PSoC Creator, 5 times)
Open each project -> TopDesign -> double-click USBFS -> String Descriptors tab -> select "Proficio"
-> change to `Multus IQ` -> OK -> Save. Change nothing else (interface names, manufacturer,
VID/PID, string order). Tell Build "step 1 done".

## Step 2 (Build): build-ultimus.py
- Remove the product-name step: `PRODUCT = "Ultimus"`, `set_product_string()`, its call, and the
  USBFS_descr.c C51 recompile + LIBX51 REPLACE that goes with it (around lines 58, 102-115, 147-156).
- Ultimus images then differ only by the DF(ULTIMUS) major. Update the docstring (lines ~5 and ~20).

## Step 3 (Build): version bumps
`FIRMWARE_VERSION_MINOR` +1 in each `basic-plus.h`, comment like
`// x.yyy (2026-11-14): USB product name "Multus IQ" (was "Proficio" / "Ultimus")`.
Leave the MAJOR lines alone.

## Step 4: build
- Build each of the 5 projects in PSoC Creator (Shift+F6), or from the command line as in the root
  CLAUDE.md build notes. A "Rebuild Failed" coming only from a post-build copy step is OK, but report it.
- `py -3 build-ultimus.py all` for the 3 Ultimus images.

## Step 5 (Build): check every new .hex (all 8)
Run the one-liner from the note's step 5 on each. Expected:
`product name: ['Multus IQ']  interface name ok: True`. Also report each file's name, size and date.

## STOP and report
List the files changed (git status), the .hex check table, and the build results. Do NOT commit,
do NOT copy into the installer payload or release/. Stew flashes his Ultimus MKII ATU first and checks:
Windows still shows "Line (Multus IQ Sound)", MSCC connects, and on Ubuntu `cat /proc/asound/cards`
shows "Multus IQ" and the Linux servers find the radio.

## Addendum (Stew, 2026-11-14): release file names
- Each tree's `Release\` (Geminus: `release\`) gets versioned files `<Radio>-<major>.<minor>.cyacd` and `.hex`,
  same as now (e.g. `Proficio-MKII-PTT-3.244.cyacd`), so users can match the file with the version in the MSCC header.
- Expected new files (8 pairs):
  Proficio-Legacy-1.237, Ultimus-Legacy-6.237, Proficio-MKII-PTT-3.245, Ultimus-MKII-PTT-8.245,
  Proficio-MKII-ATU-4.246, Ultimus-MKII-ATU-7.246, Geminus-Legacy-5.154, Geminus-MKII-2.155.
- The Proficio and Ultimus images of each shared tree come from the same source (Proficio via Creator,
  Ultimus via build-ultimus.py). Confirm both files exist for all three Proficio/Ultimus trees.
- Leave the older versioned files in place, and don't touch `radio-psoc-firmware/release/` or the installer payload until "finished".


## Finished — commit + release

The commit and release steps for the combined cmd-080 / cmd-080b batch (final versions, including 0x9D) are at the end of `.mscc-coord/cmd-080b-brief.md`.
