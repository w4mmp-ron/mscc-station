# For Stew: shared Claude rules in the repo root

From Ron, 2026-11-13.

Ron wants your working setup and his to be as close to the same as possible. So the rules his
Claude works by are now in one file both Claudes load: **`CLAUDE.md` at the repo root.**

Claude Code reads it in every session in this repo, so your Claude picks it up after your next
pull with nothing to set up. It covers:

- **Keywords:** "build" = build only; "finished" = installers, docs, one commit, push, GitHub
  Release; nothing goes into git before "finished".
- **The "finished" checklist** (the steps in `NOTE-FOR-STEW-RELEASES-AT-FINISHED-2026-11-13.md`,
  written out).
- **Keeping the trees in step:** a server change goes into the Pi, Windows and (later) Debian trees
  in the same step; a client change goes into both WPF and Avalonia, either way.
- **Versions** only in the Software Versions Addendum.
- **Never commit** the Visual Studio .tlog / .recipe / .iobj / .ipdb / .lastbuildstate files or
  the installers.
- **Build notes:** MSBuild settings, actinst from PowerShell, `make clean` after a header change.

Your `mscc-ui/windows-work-tree/CLAUDE.md` stays yours; it only got one line at the top pointing to
the shared file. Ron's machine-specific notes stay in `rpi/CLAUDE.md`.

The shared file is open to your changes. If something in it doesn't fit how you work, change it
or tell Ron.
