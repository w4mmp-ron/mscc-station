# For Stew: `mscc-ui/Avalonia-Migration` is no longer kept up to date

From Ron, 2026-11-13.

- **The Avalonia client that's kept up to date is Ron's copy, `Avalonia/`** (version 1.6.x, Pi
  package `mscc-avalonia`). It has its own `src/MSCC.Core`.
- **`mscc-ui/Avalonia-Migration` (0.6.x) is frozen.** Client changes no longer go into it, and Ron
  plans to delete it at some point.
- **WPF and Ron's Avalonia are now kept at par both ways.** A client change goes into both,
  whichever one it started in (the shared `CLAUDE.md` says so). Ron works in WPF too.

What this means for you:

- WPF client work: carry on as before. Any WPF change will also go into `Avalonia/`.
- If `Avalonia-Migration` has anything of yours that isn't in WPF or in `Avalonia/`, say so before
  it's deleted and it will be moved over.
