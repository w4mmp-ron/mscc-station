# From Ron: correction about how the Pi package is built (2026-11-15)

This corrects one point in `NOTE-FOR-STEW-AGC-AND-PACKAGING-REPLY-2026-11-15.md`.

I wrote that the Pi `mscc` .deb is not built with `rpi/mscc-deb/build-deb.sh`, so a manifest
feeding `build-deb.sh` would not reach the Pi package. That was wrong.

The Pi package **is** made by `build-deb.sh`. On my PC, Claude runs it through a short wrapper
that copies the files it needs to a Linux folder in WSL, takes the Windows line endings out of
the text files (never the firmware images), runs `build-deb.sh` there and copies the .deb back.
That wrapper is now in git: `rpi/mscc-deb/build-deb-from-windows.sh`. It was tested: the
package it builds is the same as the released `mscc_1.0.95_arm64.deb`, file for file.

So:

- Your manifest can go straight into `build-deb.sh` / `control`. The Pi package will follow it,
  whether it is built from Windows or on Linux.
- On Linux (your Ubuntu machine, or a Pi) just run `./build-deb.sh` in `rpi/mscc-deb`; the
  wrapper isn't needed there.
- Nothing else in my reply changes.
