# For Stew: the Pi server source builds on amd64 (test only)

From Ron, 2026-11-13. Just for your information, for the Debian / Ubuntu work.

The current Pi server source (`rpi/ms-sdr-linux`, `rpi/SDRcore-recv-linux`,
`rpi/SDRcore-trans-linux`) builds on amd64 with no changes:

```bash
make PORTAUDIO=distro
```

Built on Debian (x86-64, gcc 14.2) with Debian's own PortAudio (`portaudio19-dev`) and
`libusb-1.0-0-dev`.

| Server | Result | Warnings |
|---|---|---|
| ms-sdr | built, x86-64 | 36 |
| sdrcore-recv | built, x86-64 | 4 |
| sdrcore-trans | built, x86-64 | 3 |

No errors. The warnings are the same kind the Pi build gets (both are 64-bit): mostly `%ld`
used for a 32-bit number in log lines, and "string may be truncated".

**This was only a test.** The binaries were not run (no USB, no audio, no radio) and were
deleted afterwards. Nothing in the repo changed.

Things to know if you use it:

- The Makefiles put the binaries in `~/mscc`, not in the source folder.
- The `linux/` copies of the servers are from 2026-10-11; the Pi trees have moved on a lot since
  then (ms-sdr 92 files, recv 73, trans 116 differ). The Pi trees may be the easier starting point.

Your call how you use this.

## Later the same day: keeping `linux/` in step (from Ron)

Ron says you'll drop the "`linux/` is Stew's" rule in `rpi/CLAUDE.md` after your next pull.
Once that line is gone, Claude will treat the Debian servers like the Windows ones: every server
change goes into the Pi, Windows and `linux/` trees in the same step. Until then Claude leaves
`linux/` alone.

The first time will be a big one, because `linux/` is about a month behind (the 92 / 73 / 116
files above). Since the Pi source builds on amd64 as it is, the plan would be:

1. Copy the Pi server source into `linux/ms-sdr-linux`, `linux/SDRcore-recv-linux` and
   `linux/SDRcore-trans-linux`.
2. Test build on amd64 (`make PORTAUDIO=distro`).
3. From then on, each change goes into all three trees as it's made.

If you have Debian-only changes in `linux/` that aren't in the Pi trees, say so first and they'll
be kept.
