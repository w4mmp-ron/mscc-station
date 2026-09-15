# MSCC Volume GUI

Tk GUI to set **Pulse / PipeWire** volumes for MSCC’s configured devices:

| Row | Source |
|-----|--------|
| Digi speaker | `digital-speaker.ini` (default **VirtualA**) |
| Digi mic | `digital-microphone.ini` (default **VirtualB.monitor**) |
| Operator speaker | `operator-speaker.ini` → matched Pulse **sink** |
| Operator mic | `operator-microphone.ini` → matched Pulse **source** |

Also shows VirtualB / VirtualA.monitor when present.

## Install on the Pi

Copy the **entire** `rpi/mscc-volume-gui/` folder to:

**`~/mscc/mscc-volume-gui/`**

```bash
# from your PC clone, example:
scp -r rpi/mscc-volume-gui/ pi:~/mscc/mscc-volume-gui/
```

Window title must show **MSCC Volume v1.0.3** (or newer).

## Run (desktop user, not root)

```bash
cd ~/mscc/mscc-volume-gui
chmod +x mscc-volume-gui mscc-volume-restore
./mscc-volume-gui
```

Needs: `python3-tk`, `pactl` (`pipewire-pulse` or `pulseaudio-utils`).

## Sticky volumes (survive reboot)

Slider/mute → `~/.local/mscc/volume-levels.conf`

Restored on GUI Refresh, `mscc start`, and `mscc-virtual-audio` (after Virtual* recreate).

## Notes

- This controls the **OS mixer**, not MSCC software AF gains.
- Operator hardware matching uses name/description scoring (same idea as init).
- Refresh after replugging USB audio or re-running `mscc-virtual-audio`.
