# For Ron: `linux/` is now in step with the Pi trees

From Stew, 2026-11-13. Answer to `NOTE-FOR-STEW-PI-SERVERS-BUILD-ON-AMD64-2026-11-13.md`.

## Done

1. The Pi server source was copied into the Debian / Ubuntu trees, as you suggested:

   | Pi tree | Debian tree | Version |
   |---|---|---|
   | `rpi/ms-sdr-linux` | `linux/ms-sdr-linux` | 3.207 |
   | `rpi/SDRcore-recv-linux` | `linux/SDRcore-recv-linux` | 3.154 |
   | `rpi/SDRcore-trans-linux` | `linux/SDRcore-trans-linux` | 3.159 |

   There were no Debian-only changes worth keeping. The old `linux/` trees had only build
   leftovers, `.bak` files and `last_used.c`, which you had already removed on the Pi.
2. The line "Ubuntu (`linux/`) work is Stew's" in `rpi/CLAUDE.md` is gone. From now on, every
   server change goes into `rpi/`, Windows and `linux/` in the same step.
3. My Nov 2 Ubuntu work (recv 3.152 / trans 3.151 / ms-sdr 3.192) was dropped as out of date.
   It is not in the repo.

## Test build on Ubuntu amd64 (gcc 15.2)

```bash
make BINDIR=/tmp/test        # default PORTAUDIO=mscc (/usr/local)
```

| Server | Result | Warnings |
|---|---|---|
| ms-sdr | built | 8 |
| sdrcore-recv | built | 17 (14 are gcc 15's new `-Wheader-guard`) |
| sdrcore-trans | built | 18 (16 `-Wheader-guard`) |

No errors. Not run with a radio yet.

`-Wheader-guard` means a header's `#ifndef X` and `#define Y` use different names, so the guard
doesn't work. It is harmless today, but the Pi build will show it too once its gcc reaches 15.

## One thing in `rpi/` for you

Two x86-64 binaries have been in git since the September reorganization (1a36668):

- `rpi/ms-sdr-linux/ms-sdr`
- `rpi/SDRcore-trans-linux/sdrcore-trans`

They don't run on the Pi, and the Pi build puts its binaries in `~/mscc`. I left them out of the
`linux/` copy and didn't touch `rpi/`. Options: `git rm` them, or leave them.

## Avalonia: amd64 package (your offer of 2026-11-06, done on my side)

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File build-mscc-avalonia-deb.ps1              # Pi, as before
pwsh -NoProfile -File build-mscc-avalonia-deb.ps1 -Arch amd64                                 # Ubuntu / Debian PC
```

Changes in `Avalonia/`:

- `build-mscc-avalonia-deb.ps1`: new `-Arch arm64|amd64` (default arm64). amd64 builds
  `linux-x64` and sets `Architecture: amd64` in the staged control file. The template in git
  still says arm64. Paths now use `/`, and the temp folder comes from `GetTempPath()`. `python3`
  is used when there is no `python`. With these changes the script also runs under `pwsh` on
  Linux. The `.desktop` file is now saved with LF line endings too.
- **`packaging/mscc-avalonia/usr/bin/mscc-avalonia` was never in git.** The `bin/` line in
  `Avalonia/.gitignore` hid it, so the script only worked on your PC. I took it from your
  `mscc-avalonia_1.6.96_arm64.deb` (release R11-13-3) unchanged and added a `!` exception to
  `.gitignore`.
- `README.md`: a short "Ubuntu / Debian PC (amd64)" section.

Built here: `mscc-avalonia_1.6.96_amd64.deb`, with the same 224 files as your arm64 1.6.96.
It bundles .NET 9.0.23; yours has 9.0.20. It isn't installed or run yet.

**Please check on your next Pi build:** the script still makes a good arm64 package on Windows.
I could only test it on Linux.

## Server package: `mscc_<version>_amd64.deb` now follows your Pi package

```bash
./linux-build/build-mscc-deb-amd64.sh      # -> installers/linux/mscc_<version>_amd64.deb
```

The old amd64 script (mscc 1.0.53, separate portaudio / init / firmware packages) has been
rewritten. **It runs your `rpi/mscc-deb/build-deb.sh`** with these substitutions:

- the binaries folder
- the output file
- the x86-64 check in place of AArch64
- an amd64 PortAudio .deb
- an amd64 control file
- a firmware payload copy
- no drop-installers

So the version, the package files, the firmware, MSCC Init / Volume, the flash tools and `postinst`
all come from `rpi/`. **If you change one of the lines it substitutes, the amd64 build stops with
"rpi/mscc-deb/build-deb.sh changed, update build-mscc-deb-amd64.sh".** It can't go wrong silently.
The same goes for the amd64 control file, which is made from yours.

What is amd64-only:

- the programs, built from `linux/` and from `Solidus/servers/psoc-usb-bootload-linux`. They are
  built from a temp copy, so ms-sdr is **not** bumped and reports the same 3.207 as the Pi.
- PortAudio 19.8.3, built from `portaudio/`. It is an autotools build (`libportaudio.so.2.0.0`);
  yours is a CMake build (`.so.19.8`). Both have the soname `libportaudio.so.2`.
- the control file: Architecture amd64, `linux-headers-generic`, PC wording.
- the firmware `.cyacd` / `.hex` files are converted to CRLF, so they are byte-identical to yours
  (git gives LF here).

Compared file by file with your `mscc_1.0.90_arm64.deb` from R11-13-3: everything is identical
apart from the 4 programs, PortAudio, `control` and `postinst` (below).

### One change in `rpi/` (shared `postinst`)

The AudioInjector boot-overlay step now runs only when `/proc/device-tree/model` says
"Raspberry Pi". On a PC it logs "not a Raspberry Pi: ... skipped" and does nothing else. On the
Pi nothing changes. It goes into your next `mscc` build; no rebuild is needed for it.

## Linux recv 3.155 / trans 3.160, and mscc 1.0.91 (in `rpi/` too)

My Ultimus MKII ATU (7.244) failed on Ubuntu: trans "TRANSCEIVER I/Q AUDIO DEVICE NOT FOUND", and
recv took the `.monitor` of the radio's output as I/Q. Cause: on Linux the sound card is named
after the USB **product** name. The firmware reports "Proficio" on every model (Geminus too), and
`build-ultimus.py` changes that to "Ultimus". Windows shows the audio **interface** name ("Multus
IQ Sound"), so Windows never noticed.

Changed in `rpi/` and `linux/` (same code in both, Linux only):

- recv `main.c`: a `.monitor` is never radio I/Q. The radio check also accepts "Ultimus".
- recv `output-devices.c`, trans `main.c` and `input-devices.c`: "Ultimus" accepted, like
  "Proficio" / "Multus".
- recv `VERSION_MINOR` 155, trans 160 (`extern.h`). **Windows recv / trans are unchanged (3.154
  / 3.159)**; this is device discovery, which stays platform-specific. Bump Windows to match if
  you prefer equal numbers.
- `rpi/mscc-binaries/mscc-status-report`, `rpi/mscc-init-gui/mscc_init_gui/devices.py`: "ultimus"
  recognised.
- `rpi/mscc-deb/packaging/DEBIAN/control`: Version **1.0.91**. The amd64 1.0.91 is built; the
  Pi 1.0.91 is yours to build (new recv / trans binaries + the postinst guard above).

**Firmware (Stew's decision):** the USB product name becomes **"Multus IQ"** on every radio, and
`build-ultimus.py` stops changing it. Stew does this on Windows; the details are in
`NOTE-FOR-STEW-WINDOWS-FIRMWARE-MULTUS-IQ-2026-11-13.md`. The Linux servers already match "Multus",
so nothing else changes. "Ultimus" stays accepted for the already-released Ultimus images.

## Next on my side

- Install `mscc_1.0.90_amd64.deb` + `mscc-avalonia_1.6.96_amd64.deb` here and test with the radio.
- After that, delete the old `linux/mscc-deb/packaging` and `linux/helpers` (kept until tested).

Until those are done and tested, nothing goes into a Debian release.
