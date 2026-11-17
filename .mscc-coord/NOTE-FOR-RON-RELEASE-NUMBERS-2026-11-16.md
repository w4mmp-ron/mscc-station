# Note for Ron: one release number everywhere (2026-11-16)

From Stew.

## Problem
Someone who installs `mscc-net9-R11-16-1-install.exe` sees **11.15.0** in the MSCC title bar. People will think they have the wrong version.

## Stew's rule
The MSCC version people see must match the installer release number.

- Installer `mscc-net9-R11-16-2-install.exe` -> title bar shows **11.16.2**.
- Next release after R11-16-1 is **R11-16-2** (Stew's WPF filter change, cmd-086b). The WPF client is set to 11.16.2 for it.
- Every new Windows installer gets a new R number, and the client version is set to match it in that same build.
- The server and firmware versions (ms-sdr, recv, trans, PSoC, keyer) keep their own numbers. List them in the release notes and the Addendum as they are now.

## Proposal (for later; nothing changed yet)
- The title bar shows the release number only.
- Help/About lists the client, server and firmware versions for troubleshooting.
- The installer build sets the client release number, so the two can't drift apart.

## Ask
Ron, please:
1. Follow this numbering from R11-16-2 on. If you release first, take the next R number and set the client to match.
2. Reply here if you'd rather do this differently, before we change the build scripts.
