# Note for Ron: show software versions in `mscc status` (2026-11-19)

From Stew (via Build Commander). No code changed yet. This is a heads-up and a request.

## What Stew wants

When he runs `mscc status` on a Linux machine, he wants a short **Versions** section at the top that shows:

1. The installed MSCC packages and their versions (`mscc`, `mscc-avalonia`), read from `dpkg-query`.
2. The version of each server on the machine: ms-sdr, sdrcore-recv, sdrcore-trans.
3. The firmware files shipped in the package (PSoC per radio, Black Pill, keyer), by file name.
4. The newest release on GitHub (`https://api.github.com/repos/w4mmp-ron/mscc-station/releases/latest`), with each package marked **up to date** or **newer available** against that release's `.deb` names.
5. If the computer is offline or GitHub doesn't answer within a few seconds, the GitHub line says so and the rest of the report still prints. The report never fails because of this section.

Why: today there is no easy way to see what is on a machine. Before connecting, the Avalonia window shows its own version, and Core shows up after connecting, but the server versions and the `mscc` package version are hidden. This morning the Pi had `mscc` 1.0.96 while the newest release has 1.0.100, and the only way to see that was `dpkg -l | grep -i mscc`.

## Who changes what

- **Pi copy (`rpi/mscc-deb/packaging/usr/share/mscc/bin/mscc-status-report`)**: yours. Stew is keeping hands off `rpi/`. Please add the same section in your next Pi `mscc` build, if you agree.
- **Ubuntu copy (`linux/helpers/mscc-status-report` and the `linux/mscc-deb` package)**: Stew will have a Build add it when he next tests on stew-HP. The two copies have already drifted (yours has the folded device-name matching), so the Versions section should be a self-contained block that can be pasted into either copy.

## Two side notes for the releases page

- The releases have Pi (arm64) debs only. There are no Ubuntu (amd64) debs, so stew-HP has nothing on GitHub to compare against yet. Stew will decide whether to start posting the amd64 debs.
- Server versions: if the servers don't already print their version on a command-line switch (for example `--version`), the status report can read it from the version line each server writes to its log at start-up. Your call which way is cleaner.

## Questions for Ron

1. OK to add the Versions section to the Pi `mscc-status-report`?
2. Do ms-sdr, sdrcore-recv and sdrcore-trans have a switch that prints their version and exits? If not, would you rather add one or have the report read the logs?

## Stew's request for the GitHub releases page

Right now each release holds only the files that changed, so finding the current version of everything means digging through older releases (for example, `mscc-avalonia` 1.7.04 is only in R11-16-1). Stew's recommendation:

1. **Every release includes all the installers**, even the ones that didn't change: the Windows installer, the Ubuntu (amd64) `mscc` and `mscc-avalonia` debs, and the Pi (arm64) `mscc` and `mscc-avalonia` debs. The top release is then always the complete current set.
2. **Each release starts with a short table** listing all five packages and their versions, so anyone can see at a glance what is current.
3. **Starting now, keep only the newest release plus one earlier release as the fallback**, and delete the older ones. If testing needs to go back further, Stew and Ron rebuild that version from the repo history (the release tags and commits stay in git). Today there are 13 releases (R11-13-3 to R11-18-4, about 1.3 GB of files).

Release files are stored apart from the git repo, so they don't make clones or pulls bigger.

Question 3 for Ron: OK with this release layout, and can you include the amd64 debs (Stew can build them on stew-HP) starting with the next release?
