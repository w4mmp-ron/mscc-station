# For Stew: Claude can build PSoC firmware from the command line

From Ron's session, 2026-11-13.

```
cd radio-psoc-firmware\Proficio-Ultimus-MKII-PTT
"C:\Program Files (x86)\Cypress\PSoC Creator\4.4\PSoC Creator\bin\cyprjmgr.exe" -wrk Proficio-MKII-PTT.cywrk -c Debug -build
```

PSoC Creator 4.4 has a command-line builder, `cyprjmgr.exe`, so Claude can run a full build itself,
the same as Shift+F6 in Creator. Use the `.cywrk` file and the configuration (Debug / Release) of
the tree you are building.

Tested on Ron's PC today, MKII PTT tree: built in 22 s, flash 87.1 %, SRAM 44.2 %, only the usual
L15 USB warning and timing warnings. It made `Proficio-MKII-PTT.cyacd`, the file you flash. The
post-build step (`copy-release.bat`) runs as usual.

What Claude can't do:

- Edit the schematic (TopDesign). New hardware blocks (timers, ISRs, pins) still have to be added
  in the Creator window.
- Flash the radio or test it on the air.

Things to know:

- Creator 4.4 and the Keil licence have to be on the PC that runs the build.
- The Bootloadable component looks for `..\Proficio-bootloader`, so build in place, or copy the
  bootloader folder next to the tree if you build a copy.
- `build-ultimus.py` also drives the Keil / Cypress tools directly for the Ultimus builds.
