# For Stew's Windows (shack) Claude: USB product name "Multus IQ" in all radio firmware

From Stew's Ubuntu session, 2026-11-13. Read this first, then do the firmware job below with Stew.

## 1. Catching up: what changed on the Ubuntu side today

- **Repo rules:** the repo-root `CLAUDE.md` holds the shared rules for Ron and Stew: keywords
  ("build", "finished" ...), nothing goes into git before "finished", and the trees stay in step.
  Read it.
- **Ubuntu repo reset:** Stew's Nov 2 Ubuntu work was dropped as out of date. The Ubuntu clone was
  reset to `origin/main` (R11-13-3). The old work is only on a local branch there.
- **`linux/` = Pi:** `linux/ms-sdr-linux`, `linux/SDRcore-recv-linux` and
  `linux/SDRcore-trans-linux` are now copies of the Pi trees in `rpi/`. From now on, every server
  change goes into `rpi/`, Windows and `linux/` in the same step. Device discovery stays
  platform-specific.
- **Debian / Ubuntu amd64 packages** (built on Ubuntu, in `installers/linux/`):
  - `mscc_<ver>_amd64.deb`: `linux-build/build-mscc-deb-amd64.sh` runs Ron's
    `rpi/mscc-deb/build-deb.sh` with amd64 substitutions, so it has the same content and version
    as the Pi package.
  - `mscc-avalonia_<ver>_amd64.deb`: `Avalonia/build-mscc-avalonia-deb.ps1 -Arch amd64`. That
    script now also runs under `pwsh` on Linux, and arm64 stays the default.
- **Linux servers recv 3.155 / trans 3.160** (Pi and `linux/` trees only):
  - recv never takes a `.monitor` as radio I/Q.
  - recv and trans accept the "Ultimus" USB product name, so the already-released Ultimus images
    keep working on Linux.
  - Windows recv / trans are unchanged (3.154 / 3.159). Windows finds the radio by "Multus" in the
    endpoint name, so nothing is needed there.
- **Notes for Ron** are in `.mscc-coord/NOTE-FOR-RON-LINUX-TREES-IN-STEP-2026-11-13.md`.

## 2. The problem

Windows and Linux take the radio's audio name from different USB fields:

| USB field (string index) | Text in the firmware | Who uses it |
|---|---|---|
| Audio Control interface, iInterface (9) | "Multus IQ Sound" (+ 10 "Multus IQ Receive", 11 "Multus IQ Transmit") | **Windows**: "Line (Multus IQ Sound)" |
| Device descriptor, **iProduct (14)** | **"Proficio"**, on every model, Geminus included | **Linux**: ALSA card name "Proficio: USB Audio", PipeWire `..._Proficio_...` |
| iManufacturer (15) | "www.multus-sdr.com" | Linux (PipeWire name) |
| strings 1-7 ("AE9RB", "Peaberry SDR" ... "www.obdev.at") | Peaberry leftovers | nothing points to them |

- `radio-psoc-firmware/build-ultimus.py` (since 2026-10-31) also replaces iProduct "Proficio"
  with **"Ultimus"** in the Ultimus images. The Linux servers didn't know that name, so an Ultimus
  radio failed on Linux: trans "TRANSCEIVER I/Q AUDIO DEVICE NOT FOUND".
- On Linux a Geminus shows up as "Proficio". Stew doesn't want that.

**Decision (Stew): iProduct = "Multus IQ" on every radio.** Same idea as Windows' "Multus IQ
Sound", and the same name on every model.

Nothing else depends on iProduct:

- ms-sdr opens the radio by VID/PID only: `srOpen(MULTUS_VID, MULTUS_PID, "", "", ...)`, in both
  the Pi and Windows trees.
- The udev rules match `16c0:05dc` / `04b4:b71d`.
- The Linux servers already match "Multus".
- Windows shows the iInterface name, as before.

## 3. The firmware job (Windows, PSoC Creator)

### Projects (5) -> images (8)

| Project | Images (current) |
|---|---|
| `Proficio-Ultimus-Legacy/Proficio-Legacy.cydsn` | Proficio-Legacy 1.236, Ultimus-Legacy 6.236 |
| `Proficio-Ultimus-MKII-PTT/Proficio-MKII-PTT.cydsn` | Proficio-MKII-PTT 3.244, Ultimus-MKII-PTT 8.244 |
| `Proficio-Ultimus-MKII-ATU/Proficio-MKII-ATU.cydsn` | Proficio-MKII-ATU 4.245, Ultimus-MKII-ATU 7.245 |
| `Geminus-Legacy/Geminus.cydsn` | Geminus-Legacy 5.153 |
| `Geminus-MKII/Geminus.cydsn` | Geminus-MKII 2.154 |

All paths are under `radio-psoc-firmware/`.

### Steps

1. In each of the 5 projects: TopDesign -> USBFS component -> Configure -> **String
   Descriptors**. Change the string **"Proficio"** (index 14, the one the Device Descriptor's
   iProduct points to) to **"Multus IQ"**. The strings are stored in `TopDesign/TopDesign.cysch`.
   - Change nothing else: the "Multus IQ Sound / Receive / Transmit" interface names, the
     manufacturer, VID/PID, and the string order and indexes all stay as they are.
2. `build-ultimus.py`: take out the product-name step, which is `PRODUCT = "Ultimus"`,
   `set_product_string()` and the `USBFS_descr.c` recompile / LIBX51 REPLACE that goes with it.
   The Ultimus images then differ from the Proficio ones only by the `DF(ULTIMUS)` major.
   - As written, `set_product_string()` stops with "USB product string "Proficio" not found"
     once step 1 is done, so the script must change anyway.
   - Update the docstring at the top ("only the firmware major ... and the USB product string
     differ").
3. Bump `FIRMWARE_VERSION_MINOR` in each project's `basic-plus.h`, with a comment, e.g.
   `// x.yyy (2026-11-..): USB product name "Multus IQ" (was "Proficio" / "Ultimus")`.
4. Build each Proficio / Geminus project (Shift+F6). Then run `py -3 build-ultimus.py all` for the
   3 Ultimus images. Release files go to each tree's `Release\` folder as usual.
5. **Check every new `.hex` before flashing.** The command looks for the whole string
   descriptor (length byte, type 0x03, then the UTF-16 text), so "Multus IQ Sound" doesn't count
   as "Multus IQ":

   ```powershell
   py -3 -c "import sys;d=bytearray();[d.extend(bytes.fromhex(l.strip()[9:-2])) for l in open(sys.argv[1]) if l.startswith(':') and l[7:9]=='00'];p=lambda s:bytes([2+2*len(s),3])+s.encode('utf-16-le') in d;print('product name:',[n for n in ['Multus IQ','Proficio','Ultimus'] if p(n)],' interface name ok:',p('Multus IQ Sound'))" <file.hex>
   ```

   - Expected: `product name: ['Multus IQ']  interface name ok: True`.
   - Today's images give `['Ultimus']` (Ultimus 7.245) and `['Proficio']` (Geminus MKII 2.154);
     the command was tested on Ubuntu.
6. Flash Stew's radio (Ultimus MKII ATU) and check that Windows still shows **"Line (Multus IQ
   Sound)"** and the servers still find the radio.
7. Then the usual release work at "finished", following the root `CLAUDE.md`: copy the new images
   into `mscc-ui/installer/windows/payload/PSoC Firmware/<Radio>/` (one versioned
   `.cyacd` + `.hex` per radio; `stage.ps1`), the Windows installer, the Addendum, and the notes
   for Ron. The Pi `mscc` package takes its firmware from that payload, so Ron needs to rebuild it.

### After flashing, back on Ubuntu (Stew does this; nothing for Windows Claude)

- `cat /proc/asound/cards` should show "Multus IQ".
- The servers should find the radio under "Multus IQ: USB Audio" / `..._Multus_IQ_...` with no code
  change.

## 4. Open after this

- Remove the "Ultimus" acceptance from the Linux servers once nobody runs the old Ultimus images.
  It isn't urgent.
- Windows: nothing to change in the servers.
