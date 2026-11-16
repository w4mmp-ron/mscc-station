# cmd-080b: pass 0x9D SET_PADDLE_FARNSWORTH to the keyer PIC (extends uncommitted cmd-080)

This extends the uncommitted cmd-080 "Multus IQ" firmware batch on Shack. Read first:
- `.mscc-coord/NOTE-FOR-STEW-KEYER-PADDLE-FARNSWORTH-2026-11-05.md` (Ron: "PSoC boards: pass 0x9D to the PIC like 0x76 in Configure_CW")
- `.mscc-coord/NOTE-FOR-STEW-0x9D-PADDLE-FARNSWORTH-REPLY-2026-11-14.md` (Ron, 866de411: not left out on purpose; add it to this Multus IQ batch so versions do not clash)
- `.mscc-coord/cmd-080-brief.md` (already built, not committed: keep the "Multus IQ" USB product name)

Follow the repo-root `CLAUDE.md` and `AGENTS.md`. Pull only with `git pull --rebase --autostash`, and STOP on a conflict. The working tree already has the uncommitted cmd-080 work (source, Debug objects, and the 1.237 / 3.245 / 4.246 / 5.154 / 2.155 images). Do NOT `git reset`, `git checkout --`, or `stash drop`. NOTHING goes into git until Stew says "finished".

Do NOT flash a radio, the keyer PIC, or touch the servers. Stew is updating the keyer PIC himself. You only edit, compile, and check the new images.

## Goal

Unknown USB opcode 0x9D currently stalls (ms-sdr log: `usbControlMsgOUT FAILED. Status: -9`). Add a one-byte OUT handler and a Configure_CW I2C push, exactly like 0x76, in the three MKII trees that have a keyer PIC. Then bump those versions one step above the cmd-080 images Stew may already have flashed, rebuild, and keep "Multus IQ".

This is simple forwarding. Do not refactor Configure_CW, do not change USB descriptors, and do not edit `build-ultimus.py` (cmd-080 already removed the "Ultimus" product-name rewrite).

## What the host already sends (do not change ms-sdr)

Linux and Windows ms-sdr are the same. `SET_PADDLE_FARNSWORTH` is 0x9D, value 0 = off, 1 = on (`cw_record.paddle_farns`, from `CW_Paddle_Farnsworth=` in cw.ini).

It is sent only for MKII radios with the keyer installed, from `initialize_keyer` (after 0x76), and again if the client sends 0x9D over UDP. Legacy radios (`!G_proficio_mkii`) only get `SET_TX_HOLD`; they never receive 0x9D.

USB vendor OUT (same as 0x76 / SET_WPM):

- `usbControlMsgOUT(command, 0x0700 + 27, 0, (char *)(&cmd_value), sizeof(int))`
- bRequest = 0x9D, wValue = 0x071B, wIndex = 0
- data is a 4-byte little-endian int; the value is in the first byte (0 or 1)
- the PSoC 0x76 case already accepts only `sizeof(uint8_t)` = 1 byte of that payload; do the same for 0x9D
- a stall is logged only (`usbControlMsgOUT FAILED. Status: %d`); it does not stop comms

Reference: `linux/ms-sdr-linux/source/main-controller.c` 745–804 and 2508–2585; Windows `mscc-ui/windows-work-tree/ms-sdr-MKII/source/main-controller.c` 745–803 and 2609–2613; `usbcmd.c` (Linux libusb 76–84, Windows 39–51).

## 0x76 pattern to copy (MKII-PTT reference)

`radio-psoc-firmware/Proficio-Ultimus-MKII-PTT/Proficio-MKII-PTT.cydsn/`:

- `usbvend.h:58` `#define SET_MEM_TEXT_WPM 0x76`
- `usbvend01.c:350-355` OUT case: `USBFS_currentTD.pData = &E_mem_text_wpm`, `count = sizeof(E_mem_text_wpm)`, `USBFS_InitControlWrite()`
- `basic-plus.h:127` `extern uint8_t E_mem_text_wpm;`
- `main.c:38` `uint8_t E_mem_text_wpm = 0;`
- `cw.h:19` same `#define SET_MEM_TEXT_WPM 0x76`
- `cw.c` Configure_CW, case 6 (lines 301–311): if the host value changed, `buffer[0] = SET_MEM_TEXT_WPM; buffer[1] = E_mem_text_wpm;`, then `state = 10`. Case 7 (313–324) is the CQ-memory queue. Case 10 (326–354) sends `buffer[0]` then `buffer[1]` as two 1-byte I2C writes via `keyer_write`.

ATU is the same line numbers in `cw.c`. Geminus-MKII is the same pattern at `cw.c` 183–208 (case 6 = 0x76, case 7 = memory, case 10 = I2C send) and `usbvend01.c:356-358`.

## What the keyer PIC expects (already in Keyer-MCU-app-20261112; do not edit the keyer)

`keyer-buffer/main.c` (this is the 2026-11-12 source: "Proficio Configure_CW sends cmd then param as two 1-byte I2C writes"). `I2C_SlaveReceiveCallbackHandler` (296–313) pairs those two bytes and `CMD_queue_add(msg, param1, 0)`. `CW_Update_Config` (1054–1065):

```
case SET_PADDLE_FARNSWORTH:   /* 0x9D */
    cw_paddle_farns = (p1 != 0) ? 1 : 0;
```

0 = off, anything else = on. RAM only (off at PIC power-up until the radio sends it). So the PSoC must forward `buffer[0] = 0x9D`, `buffer[1] = 0 or 1`, same as 0x76. Default `E_paddle_farns = 0`.

## Trees (3 projects, 5 images)

| Project | Now (cmd-080, uncommitted) | New |
|---|---|---|
| Proficio-Ultimus-MKII-PTT/Proficio-MKII-PTT.cydsn | 3.245 / Ultimus 8.245 | 3.246 / 8.246 |
| Proficio-Ultimus-MKII-ATU/Proficio-MKII-ATU.cydsn | 4.246 / Ultimus 7.246 | 4.247 / 7.247 |
| Geminus-MKII/Geminus.cydsn | 2.155 | 2.156 |

Do NOT edit or rebuild: Proficio-Ultimus-Legacy, Geminus-Legacy (no keyer; ms-sdr never sends 0x9D), `Proficio-Legacy - Copy`, `Proficio-bootloader`, `_ultimus-build`, `radio-psoc-firmware/release/`, installer payload, `rpi/`, `Solidus/`, `installers/rpi/`. Leave the already-built cmd-080 Legacy images (1.237 / 6.237, 5.154) as they are.

## Step 1: code, same six files in each of the three trees

Minimal edits only. Use MKII-PTT as the model; ATU and Geminus-MKII get the same changes (line numbers differ slightly on Geminus-MKII).

1. `usbvend.h` — after the 0x76 block, add `#define SET_PADDLE_FARNSWORTH 0x9D` with a short comment: 0=off, 1=on; pass to the PIC like 0x76.
2. `cw.h` — add the same `#define`.
3. `basic-plus.h` — next to `E_mem_text_wpm`, add `extern uint8_t E_paddle_farns; /* SET_PADDLE_FARNSWORTH 0x9D: 0=off, 1=on */`
4. `main.c` — next to `E_mem_text_wpm`, add `uint8_t E_paddle_farns = 0;`
5. `usbvend01.c` — next to the `SET_MEM_TEXT_WPM` case, add:

```
case SET_PADDLE_FARNSWORTH:
    USBFS_currentTD.pData = (void *)&E_paddle_farns;
    USBFS_currentTD.count = sizeof(E_paddle_farns);
    requestHandled  = USBFS_InitControlWrite();
    break;
```

6. `cw.c` Configure_CW only:
   - add `static uint8_t paddle_farns = 0;` next to `mem_text_wpm`
   - insert a new **case 7** copied from case 6, using `SET_PADDLE_FARNSWORTH` / `E_paddle_farns` / `paddle_farns`
   - rename the existing CQ-memory **case 7** to **case 8** (the label only). Case 6 already does `state++` when 0x76 is unchanged, so it will fall into the new case 7; the new case 7 should `state++` into case 8 when unchanged. Case 8 still goes to `state = 0` when the memory queue is empty.
   - do not touch case 10 (`keyer_write`) or any other state machine

Do not change TopDesign, USB strings, or `build-ultimus.py`.

## Step 2: version bumps (MINOR only)

In each of the three `basic-plus.h`, set `FIRMWARE_VERSION_MINOR` one higher and add a comment like
`// x.yyy (2026-11-15): pass 0x9D SET_PADDLE_FARNSWORTH to the keyer PIC like 0x76`.
Keep the older comments. Do not change the `FIRMWARE_VERSION_MAJOR` lines:

- PTT already has `#ifdef ULTIMUS` major 8 / `#else` major 3 (lines 42–46). Leave them.
- ATU already has `#ifdef ULTIMUS` major 7 / `#else` major 4 (lines 44–48). Leave them.
- Geminus-MKII has only major 2 (line 40). Leave it.

So: PTT minor 245 → 246, ATU 246 → 247, Geminus-MKII 155 → 156.

## Step 3: build

- Build only the three MKII projects in PSoC Creator (Shift+F6), or from the command line as in the root CLAUDE.md build notes. A "Rebuild Failed" coming only from a post-build copy step is OK, but report it.
- Then, from `radio-psoc-firmware/`: `py -3 build-ultimus.py mkii-ptt mkii-atu` (not `all`; do not rebuild Legacy).
- Leave older versioned files in place. Expected new files (5 pairs):
  `Proficio-MKII-PTT-3.246`, `Ultimus-MKII-PTT-8.246`,
  `Proficio-MKII-ATU-4.247`, `Ultimus-MKII-ATU-7.247`,
  `Geminus-MKII-2.156`
  (PTT/ATU go in each tree's `Release\`; Geminus in `release\`).

## Step 4: check every new .hex (all 5)

Product / interface name (same one-liner as cmd-080). Expected: `product name: ['Multus IQ']  interface name ok: True`.

```
py -3 -c "import sys;d=bytearray();[d.extend(bytes.fromhex(l.strip()[9:-2])) for l in open(sys.argv[1]) if l.startswith(':') and l[7:9]=='00'];p=lambda s:bytes([2+2*len(s),3])+s.encode('utf-16-le') in d;print('product name:',[n for n in ['Multus IQ','Proficio','Ultimus'] if p(n)],' interface name ok:',p('Multus IQ Sound'))" <file.hex>
```

Also report each file's name, size and date. The version is in the file name (copy-release.py reads `basic-plus.h`); confirm the five new names match the table above. Do not flash.

## STOP and report

List the files changed (`git status`; expect the six source files per tree plus the new images and usual Creator Debug output), the .hex check table, and the build results. Do NOT commit, do NOT copy into the installer payload or `radio-psoc-firmware/release/`.

## Smoke test (Stew, after he has updated the keyer PIC and flashed a new MKII image)

On his Ultimus MKII (ATU image 7.247, or whichever radio he flashes):

1. Restart ms-sdr (or the usual start). In the ms-sdr log, `initialize_keyer ... SET_PADDLE_FARNSWORTH` must return a success size (4), not `usbControlMsgOUT FAILED. Status: -9`.
2. Both WPF and Avalonia already have a **PADDLE** checkbox next to FARNSWORTH on the CW tab (`MainWindow.xaml` / Avalonia `MainWindow.axaml`). Tick it on and off. The log should show `SET_PADDLE_FARNSWORTH paddle_farns=1` (or 0) with no FAILED -9. With PADDLE on, FARNSWORTH set lower than SPEED, and the new PIC, paddle letters should go out with stretched gaps; off should feel like the old keyer.

## Paste-ready prompt for Build

Work on Shack in C:\Users\n8vet\OneDrive\Documents\github\mscc-station. Follow `.mscc-coord/cmd-080b-brief.md` exactly. This extends the uncommitted cmd-080 Multus IQ firmware. Do not git reset, checkout, or stash-drop that work. Do not flash the radio, the keyer, or touch the servers. Add 0x9D SET_PADDLE_FARNSWORTH forwarding in the three MKII trees only (PTT, ATU, Geminus-MKII), copying the 0x76 pattern: E_paddle_farns, usbvend01.c OUT case, Configure_CW state (insert case 7, rename the memory case to 8). Bump FIRMWARE_VERSION_MINOR only: PTT 3.246/8.246, ATU 4.247/7.247, Geminus-MKII 2.156. Keep the Multus IQ USB name. Rebuild those three projects in PSoC Creator, then `py -3 build-ultimus.py mkii-ptt mkii-atu`. Check every new .hex with the one-liner in the brief. Do not edit rpi/, Solidus/, installers/rpi/, or the Legacy trees. Do not commit. Stop and report.


## Finished — commit + release (Build does this)

Stew said **finished** (t940u, 2026-11-16). All eight images tested OK except Geminus Legacy 5.154 (no radio; keep it in this release, noted as built but not hardware-tested). The keyer PIC update was done on all MKII radios and works. The mid-hold TUNE bursts seen on Shack were traced to that PC / USB cable, not the firmware.

Do this on Shack in `C:\Users\n8vet\OneDrive\Documents\github\mscc-station`. Follow the repo-root `CLAUDE.md` "finished" checklist, with the extras below. **Do not push** (Stew pushes). **Do not touch** `rpi/` or `installers/rpi/` (Ron's). Do not `git add -A`. Do not stage Keil `Debug/` outputs, VS bookkeeping, publish output, or `.deb` files. Stop on any `git pull` conflict.

### 1. Pull first (likely already even)

```
git fetch
git status -sb
git pull --rebase --autostash
```

If a conflict appears, **stop** and report. Origin was already even with local `main` when this section was written (HEAD `74a7e387`); this is a safety check in case Ron pushed in the meantime. After a successful rebase, say what came in (who, when, one line each).

### 2. Docs and payload (before staging)

Version numbers live only in the Software Versions Addendum. Update:

- `docs/manuals/MSCC-Software-Versions-Addendum.md` — firmware table to the versions below. Mark Geminus Legacy 5.154 as **built, not hardware-tested**. Clear the old "not tested on a radio yet" flags on the MKII images that Stew has now tested. Rebuild the PDF: `python docs/manuals/make-addendum-pdf.py` (keep it to 2 pages).
- `installers/windows/CHANGELOG.md` — new top section for this release. Include: USB product name "Multus IQ" on all eight radios; 0x9D SET_PADDLE_FARNSWORTH forwarded to the keyer PIC on the three MKII trees; keyer PIC update tested on all MKII radios; Geminus Legacy 5.154 built but not hardware-tested; **Shack TUNE bursts were the PC/USB cable, not the firmware.**
- Leave the Operator's Guides alone (they must not carry version numbers). Only touch `installers/windows/INSTALL.md` if a firmware tool or step actually changed (it should not).

Windows payload (tracked in git): run `pwsh mscc-ui/installer/windows/stage.ps1`. That script already copies the **newest** versioned `<Radio>-<major>.<minor>.cyacd` / `.hex` from each tree's `Release\` (Geminus: `release\`) into `mscc-ui/installer/windows/payload/PSoC Firmware\<folder>\`, replacing the previous pair. Then run `git diff --stat -- mscc-ui/installer/windows/payload` and **look at it**. For this job, only the eight PSoC Firmware folders should change (final images: 1.237, 5.154, 2.156, 3.246, 4.247, 6.237, 7.247, 8.246). If stage.ps1 also refreshed client or server exes, Keyer Firmware, or Black Pill files, **do not stage those** unless Stew has said they belong in this same installer. Do not copy into the retired top-level `radio-psoc-firmware/release/` archive.

Do **not** run `linux-build/build-mscc-firmware-deb.sh` and do **not** add any `.deb`. That script still packs dated files from the retired `radio-psoc-firmware/release/` folder and would get the wrong images. Do not edit `rpi/`. Add a "With Ron" line that he updates the Pi `mscc` package with these images (they install to `/usr/share/mscc/firmware/<Radio>/`).

### 3. Stage explicit paths only

There are about **406** dirty paths (378 modified, 28 untracked). A blanket `git add -u radio-psoc-firmware/<tree>` would also pick up **219 already-tracked Keil Debug/** files (158 `.lst`, plus `.obj`-class `.omf` / `.elf` / `.map` / `.cod` / `.lib` / `.ihx` and unversioned Debug `.cyacd` / `.hex`). **Do not stage any of `DP8051/` or `Debug/`.** Use path-limited add instead:

```
git add radio-psoc-firmware/build-ultimus.py

git add radio-psoc-firmware/Geminus-Legacy/Geminus.cydsn/basic-plus.h
git add radio-psoc-firmware/Geminus-Legacy/Geminus.cydsn/TopDesign
git add -u -- radio-psoc-firmware/Geminus-Legacy/Geminus.cydsn/Generated_Source
git add -u -- radio-psoc-firmware/Geminus-Legacy/Geminus.cydsn/codegentemp
git add -u -- radio-psoc-firmware/Geminus-Legacy/Geminus.cydsn/Geminus.cyfit
git add -u -- radio-psoc-firmware/Geminus-Legacy/Geminus.cydsn/Geminus.rpt
git add -u -- radio-psoc-firmware/Geminus-Legacy/Geminus.cydsn/Geminus_timing.html
git add radio-psoc-firmware/Geminus-Legacy/release/Geminus-Legacy-5.154.cyacd
git add radio-psoc-firmware/Geminus-Legacy/release/Geminus-Legacy-5.154.hex

git add radio-psoc-firmware/Geminus-MKII/Geminus.cydsn/basic-plus.h
git add radio-psoc-firmware/Geminus-MKII/Geminus.cydsn/cw.c
git add radio-psoc-firmware/Geminus-MKII/Geminus.cydsn/cw.h
git add radio-psoc-firmware/Geminus-MKII/Geminus.cydsn/main.c
git add radio-psoc-firmware/Geminus-MKII/Geminus.cydsn/usbvend.h
git add radio-psoc-firmware/Geminus-MKII/Geminus.cydsn/usbvend01.c
git add radio-psoc-firmware/Geminus-MKII/Geminus.cydsn/TopDesign
git add -u -- radio-psoc-firmware/Geminus-MKII/Geminus.cydsn/Generated_Source
git add -u -- radio-psoc-firmware/Geminus-MKII/Geminus.cydsn/codegentemp
git add -u -- radio-psoc-firmware/Geminus-MKII/Geminus.cydsn/Geminus.cyfit
git add -u -- radio-psoc-firmware/Geminus-MKII/Geminus.cydsn/Geminus.rpt
git add -u -- radio-psoc-firmware/Geminus-MKII/Geminus.cydsn/Geminus_timing.html
git add radio-psoc-firmware/Geminus-MKII/release/Geminus-MKII-2.155.cyacd
git add radio-psoc-firmware/Geminus-MKII/release/Geminus-MKII-2.155.hex
git add radio-psoc-firmware/Geminus-MKII/release/Geminus-MKII-2.156.cyacd
git add radio-psoc-firmware/Geminus-MKII/release/Geminus-MKII-2.156.hex

git add radio-psoc-firmware/Proficio-Ultimus-Legacy/Proficio-Legacy.cydsn/basic-plus.h
git add radio-psoc-firmware/Proficio-Ultimus-Legacy/Proficio-Legacy.cydsn/TopDesign
git add -u -- radio-psoc-firmware/Proficio-Ultimus-Legacy/Proficio-Legacy.cydsn/Generated_Source
git add -u -- radio-psoc-firmware/Proficio-Ultimus-Legacy/Proficio-Legacy.cydsn/codegentemp
git add -u -- radio-psoc-firmware/Proficio-Ultimus-Legacy/Proficio-Legacy.cydsn/Proficio-Legacy.cyfit
git add -u -- radio-psoc-firmware/Proficio-Ultimus-Legacy/Proficio-Legacy.cydsn/Proficio-Legacy.rpt
git add -u -- radio-psoc-firmware/Proficio-Ultimus-Legacy/Proficio-Legacy.cydsn/Proficio-Legacy_timing.html
git add radio-psoc-firmware/Proficio-Ultimus-Legacy/Release/Proficio-Legacy-1.237.cyacd
git add radio-psoc-firmware/Proficio-Ultimus-Legacy/Release/Proficio-Legacy-1.237.hex
git add radio-psoc-firmware/Proficio-Ultimus-Legacy/Release/Ultimus-Legacy-6.237.cyacd
git add radio-psoc-firmware/Proficio-Ultimus-Legacy/Release/Ultimus-Legacy-6.237.hex

git add radio-psoc-firmware/Proficio-Ultimus-MKII-ATU/Proficio-MKII-ATU.cydsn/basic-plus.h
git add radio-psoc-firmware/Proficio-Ultimus-MKII-ATU/Proficio-MKII-ATU.cydsn/cw.c
git add radio-psoc-firmware/Proficio-Ultimus-MKII-ATU/Proficio-MKII-ATU.cydsn/cw.h
git add radio-psoc-firmware/Proficio-Ultimus-MKII-ATU/Proficio-MKII-ATU.cydsn/main.c
git add radio-psoc-firmware/Proficio-Ultimus-MKII-ATU/Proficio-MKII-ATU.cydsn/usbvend.h
git add radio-psoc-firmware/Proficio-Ultimus-MKII-ATU/Proficio-MKII-ATU.cydsn/usbvend01.c
git add radio-psoc-firmware/Proficio-Ultimus-MKII-ATU/Proficio-MKII-ATU.cydsn/TopDesign
git add -u -- radio-psoc-firmware/Proficio-Ultimus-MKII-ATU/Proficio-MKII-ATU.cydsn/Generated_Source
git add -u -- radio-psoc-firmware/Proficio-Ultimus-MKII-ATU/Proficio-MKII-ATU.cydsn/codegentemp
git add -u -- radio-psoc-firmware/Proficio-Ultimus-MKII-ATU/Proficio-MKII-ATU.cydsn/Proficio-MKII-ATU.cyfit
git add -u -- radio-psoc-firmware/Proficio-Ultimus-MKII-ATU/Proficio-MKII-ATU.cydsn/Proficio-MKII-ATU.rpt
git add -u -- radio-psoc-firmware/Proficio-Ultimus-MKII-ATU/Proficio-MKII-ATU.cydsn/Proficio-MKII-ATU_timing.html
git add radio-psoc-firmware/Proficio-Ultimus-MKII-ATU/Release/Proficio-MKII-ATU-4.246.cyacd
git add radio-psoc-firmware/Proficio-Ultimus-MKII-ATU/Release/Proficio-MKII-ATU-4.246.hex
git add radio-psoc-firmware/Proficio-Ultimus-MKII-ATU/Release/Proficio-MKII-ATU-4.247.cyacd
git add radio-psoc-firmware/Proficio-Ultimus-MKII-ATU/Release/Proficio-MKII-ATU-4.247.hex
git add radio-psoc-firmware/Proficio-Ultimus-MKII-ATU/Release/Ultimus-MKII-ATU-7.246.cyacd
git add radio-psoc-firmware/Proficio-Ultimus-MKII-ATU/Release/Ultimus-MKII-ATU-7.246.hex
git add radio-psoc-firmware/Proficio-Ultimus-MKII-ATU/Release/Ultimus-MKII-ATU-7.247.cyacd
git add radio-psoc-firmware/Proficio-Ultimus-MKII-ATU/Release/Ultimus-MKII-ATU-7.247.hex

git add radio-psoc-firmware/Proficio-Ultimus-MKII-PTT/Proficio-MKII-PTT.cydsn/basic-plus.h
git add radio-psoc-firmware/Proficio-Ultimus-MKII-PTT/Proficio-MKII-PTT.cydsn/cw.c
git add radio-psoc-firmware/Proficio-Ultimus-MKII-PTT/Proficio-MKII-PTT.cydsn/cw.h
git add radio-psoc-firmware/Proficio-Ultimus-MKII-PTT/Proficio-MKII-PTT.cydsn/main.c
git add radio-psoc-firmware/Proficio-Ultimus-MKII-PTT/Proficio-MKII-PTT.cydsn/usbvend.h
git add radio-psoc-firmware/Proficio-Ultimus-MKII-PTT/Proficio-MKII-PTT.cydsn/usbvend01.c
git add radio-psoc-firmware/Proficio-Ultimus-MKII-PTT/Proficio-MKII-PTT.cydsn/TopDesign
git add -u -- radio-psoc-firmware/Proficio-Ultimus-MKII-PTT/Proficio-MKII-PTT.cydsn/Generated_Source
git add -u -- radio-psoc-firmware/Proficio-Ultimus-MKII-PTT/Proficio-MKII-PTT.cydsn/codegentemp
git add -u -- radio-psoc-firmware/Proficio-Ultimus-MKII-PTT/Proficio-MKII-PTT.cydsn/Proficio-MKII-PTT.cyfit
git add -u -- radio-psoc-firmware/Proficio-Ultimus-MKII-PTT/Proficio-MKII-PTT.cydsn/Proficio-MKII-PTT.rpt
git add -u -- radio-psoc-firmware/Proficio-Ultimus-MKII-PTT/Proficio-MKII-PTT.cydsn/Proficio-MKII-PTT_timing.html
git add radio-psoc-firmware/Proficio-Ultimus-MKII-PTT/Release/Proficio-MKII-PTT-3.245.cyacd
git add radio-psoc-firmware/Proficio-Ultimus-MKII-PTT/Release/Proficio-MKII-PTT-3.245.hex
git add radio-psoc-firmware/Proficio-Ultimus-MKII-PTT/Release/Proficio-MKII-PTT-3.246.cyacd
git add radio-psoc-firmware/Proficio-Ultimus-MKII-PTT/Release/Proficio-MKII-PTT-3.246.hex
git add radio-psoc-firmware/Proficio-Ultimus-MKII-PTT/Release/Ultimus-MKII-PTT-8.245.cyacd
git add radio-psoc-firmware/Proficio-Ultimus-MKII-PTT/Release/Ultimus-MKII-PTT-8.245.hex
git add radio-psoc-firmware/Proficio-Ultimus-MKII-PTT/Release/Ultimus-MKII-PTT-8.246.cyacd
git add radio-psoc-firmware/Proficio-Ultimus-MKII-PTT/Release/Ultimus-MKII-PTT-8.246.hex

git add docs/manuals/MSCC-Software-Versions-Addendum.md
git add docs/manuals/MSCC-Software-Versions-Addendum.pdf
git add installers/windows/CHANGELOG.md
git add mscc-ui/installer/windows/payload/PSoC Firmware
git add .mscc-coord/cmd-080-brief.md
git add .mscc-coord/cmd-080b-brief.md
```

The intermediate versioned images (3.245 / 8.245, 4.246 / 7.246, 2.155) stay in each tree's `Release\` beside the finals, which is the existing convention (older versioned files are kept, nothing is overwritten). The payload gets only the newest of each radio.

**Do not stage:** `MSCC.aip` (any of the three copies — see "Stew must decide" below); anything under `*/DP8051/` or `*/Debug/`; `linux/mscc-firmware-deb/*.deb`; `rpi/`; `installers/windows/*.exe`; `installers/rpi/`.

### 4. Verify the index

```
git diff --cached --stat
git diff --cached --name-only | findstr /I /C:"\\Debug\\" /C:"\\DP8051\\" /C:".lst" /C:".deb" /C:"MSCC.aip"
```

The `findstr` must print nothing. Expected cached set, before counting payload/docs, is about **186** paths already dirty plus the Addendum / CHANGELOG / payload updates:

- 20 hand-edited sources (`.c` / `.h`)
- 5 `TopDesign/TopDesign.cysch`
- 1 `radio-psoc-firmware/build-ultimus.py` (cmd-080: stop rewriting the USB product string to "Ultimus")
- 117 already-tracked `Generated_Source/` + `codegentemp/` files (includes the "Multus IQ" `USBFS_descr.c`)
- 15 already-tracked Creator reports (`*.cyfit`, `*.rpt`, `*_timing.html` at the `.cydsn` root)
- 26 new versioned `Release/` / `release/` images (16 finals + 10 intermediates)
- 2 briefs
- Addendum `.md` + `.pdf`, `CHANGELOG.md`, and the eight payload PSoC Firmware pairs (old files removed, new finals added)

If the stat shows Debug listings, `.deb`s, or `MSCC.aip`, unstage them (`git restore --staged -- <path>`) and check again.

### 5. Commit (do not push)

Message (subject + body; include the With Ron trailer):

```
cmd-080/080b: Multus IQ USB name + 0x9D Farnsworth forwarding (MKII)

USB iProduct is "Multus IQ" on all eight PSoC images. The three MKII trees
also forward 0x9D SET_PADDLE_FARNSWORTH to the keyer PIC like 0x76.

- Proficio MKII-PTT 3.246
- Ultimus MKII-PTT 8.246
- Proficio MKII-ATU 4.247
- Ultimus MKII-ATU 7.247
- Geminus MKII 2.156
- Proficio Legacy 1.237
- Ultimus Legacy 6.237
- Geminus Legacy 5.154 (built, not hardware-tested)

Keyer PIC update tested on all MKII radios and works. Shack TUNE bursts
were the PC/USB cable, not the firmware.

With Ron
- Please put these images in the Pi mscc package
  (/usr/share/mscc/firmware/<Radio>/).
- Keyer update on a PSoC radio with the daughter board: done on all MKII
  radios, works. Please add to the instructions: power-cycle the radio
  after the keyer PIC update (no RF until then).
- Geminus Legacy 5.154 is in this release but not hardware-tested (no
  radio; Stew has parts to make one later).
- WinUSB OS descriptors (NOTE-FOR-STEW-NO-ZADIG-WINUSB-2026-11-16): Stew
  likes the idea and will copy the other trees from your tested MKII PTT
  example when it is ready. Not in this batch.
```

Then **stop for Stew to push**. Run `git fetch` again only after he has pushed, if you are asked to do the GitHub Release.

### 6. Windows installer and GitHub Release (after the commit, and after Stew pushes)

Root `CLAUDE.md` says "finished" includes a Windows installer (if something installable changed) and a GitHub Release `R<month>-<day>-<n>`. Firmware rides in the Windows installer payload, so a new installer is the usual way users get these images. Installers are not in git.

**Stop and ask Stew before building the installer** if these are still open (they were when this section was written):

- The dirty repo-root `MSCC.aip` is **not** this firmware job. It already has `AppVersion=11.15.0+784e5fcc` and `SetupFileName=mscc-net9-R11-15-0-install` (the waiting WPF 11.15.0 / Avalonia 1.7.04 drop-in). There are three `MSCC.aip` files; recent installer commits used `mscc-ui/installer/windows/MSCC.aip`. Do not commit the current root `.aip` as-is.
- Suggested release name, if none exists yet: **R11-16-1** (check with `gh release list`). The Addendum already said WPF 11.15.0 and Avalonia 1.7.04 would "ride the next GitHub Release with the firmware."
- By convention, GitHub Release assets are the **installers**, not the raw `.cyacd` / `.hex` (those live in git and inside the installer). Do not attach a Pi `.deb` (Ron's). Attach raw firmware images only if Stew asks.

If Stew says to go ahead: set `AppVersion` and `SetupFileName` in the agreed `.aip`, run actinst from **PowerShell** (`"C:\Program Files (x86)\Actual Installer\actinst.exe" /S MSCC.aip` → `installers/windows/`). After he has pushed the commit, create the release:

```
gh release create R11-16-1 --title R11-16-1 --target main --notes-file notes.md installers/windows/mscc-net9-R11-16-1-install.exe
```

Release notes: what changed (Multus IQ name, 0x9D on MKII, versions, Geminus Legacy not hardware-tested, Shack TUNE bursts were the cable), then **"Downloads: which file do I need?"**, then the usual links to the Windows install guide, the Raspberry Pi install guide, and the Software Versions Addendum PDF (use GitHub blob URLs, not repo paths). Check the uploaded file size. If the Pi package is not ready, say so and that Ron will add the images to `mscc`.

### Stew must decide (do not guess)

1. **`MSCC.aip`** — leave unstaged unless he says it belongs in this finished job, and which of the three copies / which release name to use.
2. **Windows installer contents** — firmware only, or firmware plus the already-waiting WPF 11.15.0 / Avalonia 1.7.04 client (as the Addendum planned).
3. **GitHub Release assets** — installer only (convention), or also the raw `.cyacd` / `.hex`.
4. **Intermediate Release images** (3.245, 4.246, 2.155, and the matching Ultimus) — this section stages them because the trees keep every versioned file. Say if he wants only the finals in git.
