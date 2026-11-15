# Reply from Ron: AGC FAST RELEASE and the packaging plan (2026-11-15)

This answers your two notes from today: the AGC FAST RELEASE note and the packaging note.

## AGC FAST RELEASE

You're right. The slider should go from 0 to 2000 ms in steps of 10, the same as in WPF.

In Avalonia the program also cuts the value off at 1000 in a few places (the view-model and
the settings file). Please raise those to 2000 too.

Please make these Avalonia changes yourself, together with your mouse-wheel work.

## Packaging: your three questions

1. The manifest idea: yes, go ahead. Starting with a check script that only reports is fine.
2. Pinning the Multus IQ firmware only after 0x9D is in and you have tested each image: yes,
   that's what I want.
3. Anything that overlaps: no, I'm not changing anything right now.

One thing you should know: I don't build the Pi `mscc` .deb with `rpi/mscc-deb/build-deb.sh`.
Claude builds it on my PC with its own script. So if the manifest only feeds `build-deb.sh`, it
won't affect the Pi package as I build it today. Let's sort that out before Phase 2 changes
anything on the Pi side.

## Packaging: your ideas for my side

OK to all of them:

- The Pi install script (`postinst`) can delete the old desktop icons and old scripts that are
  left over, or add the link you suggested.
- The Pi package can take its firmware versions from the manifest, once the build-script
  question above is sorted out.
- The PC Linux packages and the Pi package keep using the same version number.
- In `MSCC.aip`, change the `C:\Users\Ron\.grok\worktrees` paths so they work on any PC.
- The .deb files and the Keil `Debug/` files can come out of git. They stay on disk; git just
  stops tracking them.
- Fix the `stage.ps1` path to `psoc-replacement-stm32` on your side.
