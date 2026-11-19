# Reply to Stew: mscc status versions and the release layout (2026-11-19)

Answers to `NOTE-FOR-RON-MSCC-STATUS-VERSIONS-2026-11-19.md`.

## 1. Versions section in `mscc status`: yes, done on the Pi

File: `rpi/mscc-deb/packaging/usr/share/mscc/bin/mscc-status-report` (the copy the Pi package
installs). The Versions section is the first one in the report. It shows:

- the installed `mscc` and `mscc-avalonia` (from `dpkg-query`),
- each server's version, read from the program file (see 2),
- the firmware files in `/usr/share/mscc/firmware`,
- the newest GitHub release, with each package marked "up to date" or "newer available".
  Offline or no answer in 4 seconds: one line says so and the rest of the report prints.

It never adds a FAIL. "Newer available" is a WARN.

**For the Ubuntu copy:** the block between `# Versions (2026-11-19 ...` and
`# --- end of Versions block ---` is self-contained. Paste it above `def summary()` and add one
line, `check_versions(R)`, at the start of `main()`. It only needs the `Report` class.

Tested on Ron's Pi: `mscc 1.0.96: newer available (1.0.101 in R11-19-0)`.

## 2. Server versions: no `--version` switch, on purpose

Running a second ms-sdr, even just to print a version, starts a second instance: it wipes the
log and zeroes `user_controls.ini`. So the servers now carry their version as text inside the
program file, and the report reads the file without running it:

    MSCC-VERSION ms-sdr 3.215
    MSCC-VERSION sdrcore-recv 3.157
    MSCC-VERSION sdrcore-trans 3.163

The same line goes into each server's log at start-up. Code: `main.c` in each Pi server, a
`static const char mscc_version_tag[]` built from `VERSION_MAJOR` / `VERSION_MINOR`, marked
`__attribute__((used))` so the compiler keeps it. Older builds show "no version tag".

For the Ubuntu servers (`linux/*-linux`), the same few lines in each `main.c` give the same tag.
The Windows servers could carry it later, after the hold.

## 3. Release layout: Ron agrees

From the next release:

- every release carries all the installers, changed or not: Windows, Pi `mscc` and
  `mscc-avalonia`, Ubuntu `mscc` and `mscc-avalonia`;
- the notes start with a short version table;
- only the newest release and one earlier one are kept; older ones are deleted (the git tags stay).

The shared rule is in the repo `CLAUDE.md`, "finished" step 5.

**One question back:** how do your Ubuntu (amd64) debs get to us for a release? For example,
you attach them to the newest release yourself after we publish it, or you leave them somewhere
we can pick them up. Until we hear, our releases carry the Windows and Pi files only.
