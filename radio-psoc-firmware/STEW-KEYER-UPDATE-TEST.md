# Stew: please test the keyer update on a PSoC radio

```
keyer-flash.gui                      Pi (menu MSCC -> Keyer Firmware Update)
keyer-flash-gui.exe                  Windows (C:\mscc-net9\Keyer Firmware\)
```

Ron, 2026-11-12. Ron has no PSoC radio with the daughter board, so **these PSoC builds have not
been run on a radio yet.** They build clean. Please test one when you can; the same change is in
all five, so one working radio covers the code (Proficio MKII PTT is the closest to what Ron
tested on the Black Pill).

| Radio | File (in the tree's Release folder) |
|---|---|
| Proficio MKII PTT | `Proficio-Ultimus-MKII-PTT\Release\Proficio-MKII-PTT-3.244.cyacd` |
| Proficio MKII ATU | `Proficio-Ultimus-MKII-ATU\Release\Proficio-MKII-ATU-4.245.cyacd` |
| Geminus MKII | `Geminus-MKII\release\Geminus-MKII-2.154.cyacd` |
| Ultimus MKII PTT | `Proficio-Ultimus-MKII-PTT\Release\Ultimus-MKII-PTT-8.244.cyacd` |
| Ultimus MKII ATU | `Proficio-Ultimus-MKII-ATU\Release\Ultimus-MKII-ATU-7.245.cyacd` |

-------------------------------------------------------------------------------

## What it does

The keyer chip (PIC on the daughter board) can now be updated over the radio's USB, without
the PICkit. The update tool talks to the radio with USB opcode 0x9F; the radio passes each
message on to the keyer over I2C. Nothing else in the radio firmware changed.

- The keyer needs its new I2C bootloader once, with the PICkit:
  `keyer-buffer\Release\Keyer-MCU-full-20261112.hex` (bootloader + keyer; `-Z` keeps the
  keyer's EEPROM, i.e. settings and CQ memories).
- After that, updates use `Keyer-MCU-app-<date>.hex` with the update tool.
- The tool refuses to run while the servers are up.

-------------------------------------------------------------------------------

## Test, one step at a time

1. Flash the radio firmware above (Firmware Upload, as usual). Check the radio works as
   before: receive, transmit, CW with the paddles, a CQ memory.
2. Put `Keyer-MCU-full-20261112.hex` on the keyer with the PICkit (`-Z`). Keyer works as before.
3. Stop the servers. Start Keyer Firmware Update. Press **Check Keyer**:
   it should say "Keyer app is answering."
4. Load File: `Keyer-MCU-app-20261112.hex` (same keyer, so nothing changes). **Program**.
   About half a minute: "Writing row ... of 179", "Checking ...", then
   "Keyer updated and running."
5. Start the servers. Keyer works as before, settings and CQ memories kept.

If step 3 says "radio firmware has no keyer update (opcode 0x9F)", the radio is not running
the new firmware. If step 4 fails half way, the keyer waits in its bootloader (it does not
key): press Program again, or put the full hex back with the PICkit.

Please tell Ron the result. If something fails, the window's log text is what helps.

-------------------------------------------------------------------------------

## The code (same in all three trees)

- `usbvend.h`: `CMD_KEYER_BOOT 0x9F`.
- `basic-plus.h`: state, buffers and limits (`E_kb_*`, `KB_*`), version bump.
- `usbvend01.c`: in the USB interrupt only. OUT 0x9F stores the request (wValue = I2C address,
  0x40 keyer or 0x5C keyer bootloader only; wIndex = bytes to read, up to 16; data = up to 18
  bytes to write). IN 0x9F returns the state and the bytes read.
- `cw.c` `Keyer_Boot_Main()`, called from the main loop (beater case 2, next to
  `Configure_CW()`): once the control write has finished (`USBFS_transferState` idle), one
  I2C transfer with the same blocking `I2C_DISPLAY` calls as `keyer_write()`, about 2 ms.
  Idle unless the update tool is running (servers stopped).

Same USB protocol as the Black Pill 3.16, so the tool is the same for both. Protocol and
bootloader details: `keyer-buffer\bootloader\README.md`.

Build note: the Creator post-build step points at a path on your PC
(`C:\Users\n8vet\...\copy-release.bat`), so on Ron's PC the builds were copied to Release
with each tree's `copy-release.bat` by hand. The Keil `L15` warning in the build
(`USBFS_LOADNEXTINEP`, Cypress USB code) was there before this change.
