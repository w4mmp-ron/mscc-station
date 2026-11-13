# For Stew: how releases are made now

From Ron, 2026-11-13. Just so you know how the commits and downloads come out on our side.

## One commit per finished job

Work on the Pi and Windows servers, the Pi package and the clients is not committed as it
goes. When Ron says the job is **finished**, everything for it goes out together:

1. The installers are built:
   - Windows: `mscc-net9-R<date>-<n>-install.exe` (WPF client + Windows servers + firmware + tools)
   - Pi: `mscc_<version>_arm64.deb` (required) and `mscc-avalonia_<version>_arm64.deb` (optional)
2. The docs are brought up to date: the **MSCC Software Versions Addendum** (`docs/manuals/`, .md
   and .pdf), `installers/windows/CHANGELOG.md` and the `INSTALL.md` files.
3. One commit, then a push. The commit message ends with your list ("With Stew").
4. A GitHub Release with the installers attached. Its notes say what changed and which file to
   download for which computer.

So between "finished"s, nothing new appears in git from us.

## Where things are

- **Downloads:** <https://github.com/w4mmp-ron/mscc-station/releases>. The installers are not in
  git (`.gitignore`).
- **Version numbers:** only in the Software Versions Addendum. The guides refer to it, so they
  don't change when only a version does.
- **Pi and Windows servers are kept the same.** A server change goes into both
  `rpi/*-linux` and `mscc-ui/windows-work-tree`; a client change goes into both WPF and Ron's
  Avalonia copy, whichever one it started in.
- **Pi ms-sdr `version.h`:** the Pi build bumps it on every build. At each "finished" the Pi's copy
  is put back into git, so the repo shows the version the Pi runs.

## Debian / Ubuntu

Once your Debian work is done, its amd64 .debs will be built and attached at every "finished"
too, and added to the Addendum. Until then the releases are Windows and Pi only.
