# For Stew: an x86_64 package of my Avalonia copy

From Ron, 2026-11-06. An offer; only if it's useful to you.

My Avalonia copy is `Avalonia/` (package `mscc-avalonia` 1.x, installs beside your mscc-ui).
Remote Digital works on the air with it: WSJT-X on the client computer, CAT or RTS PTT, and its
own pair of virtual sound devices made at login. The top of `Avalonia/README.md` has the
details.

The code isn't tied to the Pi; only the build script is set for arm64 today. If you'd like an
x86_64 package for Ubuntu or Debian, I can:

- add an `-Arch amd64` choice to `Avalonia/build-mscc-avalonia-deb.ps1`, with the Pi build
  staying the default and unchanged,
- build `mscc-avalonia_<version>_amd64.deb` and put it in `installers/linux/`,
- add the PC install steps to `Avalonia/README.md`.

I haven't built or tried it on x86_64 yet, so the first package would be untested. Just say if
you want it.
