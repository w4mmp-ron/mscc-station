# keyer-flash: update the keyer chip over USB

```
keyer-flash.gui                                   Pi: window (menu MSCC -> Keyer Firmware Update)
keyer-flash --check                               Pi: is the keyer app or its bootloader answering?
keyer-flash Keyer-MCU-app-20261112.hex            Pi: update
keyer-flash --exit                                Pi: leave the bootloader, run the keyer
keyer-flash-gui.exe / keyer-flash.exe             Windows (C:\mscc-net9\Keyer Firmware\)
```

Updates the PIC keyer on the daughter board through the radio's USB: Pi or PC -> USB opcode
0x9F -> Black Pill (3.16 or later) -> I2C -> keyer. No PICkit, except once: the keyer needs its
I2C bootloader, which goes on with the PICkit and the `Keyer-MCU-full-<date>.hex` file
(bootloader + app). After that, use the `Keyer-MCU-app-<date>.hex` file with this tool.

Stop the servers first (Pi: `mscc stop`; Windows: close them). If any server is running, the
tool says so and stops without touching the keyer. An update takes about half a minute; keyer
settings and CQ memories are kept. If it is cut short, the keyer waits in its bootloader: run it
again (in the window, Start Keyer lights up only then).

PSoC radios: not yet (the PSoC firmware has no 0x9F); use the PICkit.

-------------------------------------------------------------------------------

Files here:
- `keyer-flash.py`: does the work (also the command line). Needs pyusb.
- `keyer-flash-gui.py`: the window (tkinter); loads keyer-flash.py from the same folder.
- `keyer-flash.desktop`: Pi menu entry, installed as `/usr/share/applications/mscc-keyer-flash.desktop`.
- `build-exe.cmd`: builds `..\windows\keyer-flash\keyer-flash.exe` and `keyer-flash-gui.exe`
  (PyInstaller, Python and pyusb built in). Rebuild after changing either script.

Where they go: Pi .deb (`rpi/mscc-deb/build-deb.sh`) puts the scripts in `/usr/share/mscc/bin`
with links in `/usr/local/bin` and the hex files in `/usr/share/mscc/firmware/keyer/`; the
Windows installer (`mscc-ui/installer/windows/stage.ps1`) puts the .exe files and hex files in
`Keyer Firmware\`. Protocol and bootloader: `keyer-buffer/bootloader/README.md`.
