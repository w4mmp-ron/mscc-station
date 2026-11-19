# Note for Stew: Pi Avalonia 1.7.08 is built and tested (2026-11-19)

Reply to `NOTE-FOR-RON-UBUNTU-1.0.102-AND-AVALONIA-1.7.08-2026-11-19.md`.

## Facts

- Ron built the Pi package from `Avalonia/` at `d2ebe929`: `mscc-avalonia_1.7.08_arm64.deb`,
  42,729,332 bytes (`pwsh Avalonia/build-mscc-avalonia-deb.ps1`).
- File by file against 1.7.04: 234 files in both, none added or dropped. Changed: control, the
  MSCC.Avalonia / MSCC.Core files, README, and the Avalonia library files (12.1.1 -> 12.1.6 since 1.7.05).
- Installed on proficio and tested by Ron: connect kept the server's settings, the client runs fine.

## For R11-19-2

**Ron asks: please build the Pi Avalonia 1.7.08 yourself and put it on R11-19-2** in place of
1.7.04. Installers are not in git, so Ron's file does not reach you with a pull.

```
pwsh Avalonia/build-mscc-avalonia-deb.ps1        # default arch arm64 -> Avalonia/mscc-avalonia_1.7.08_arm64.deb
```

- Version table line: "mscc-avalonia 1.7.08, Raspberry Pi, changed".
- Check: `dpkg-deb -f` says mscc-avalonia / 1.7.08 / arm64; 234 files, the same list as 1.7.04.
  Ron's build was 42,729,332 bytes; yours may differ by a few bytes (build time stamps).

## Also

- Your file check found a Python cache file in the Pi `mscc_1.0.102_arm64.deb`. It came from Ron's
  PC (a test run of `mscc-status-report`); it was never in git. It is removed from
  `rpi/mscc-deb/packaging`, so the next Pi build is without it. Harmless in 1.0.102.
- Your note says the SA session was to the Pi at 192.168.1.193 with ms-sdr 3.211, so the new
  Ubuntu servers (3.215 / 3.157 / 3.163) were installed but not run against the radio. Worth one
  session with the radio on stew-HP before or after the release.
