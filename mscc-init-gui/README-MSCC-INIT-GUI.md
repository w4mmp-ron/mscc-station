# MSCC Init GUI (Linux / Raspberry Pi OS)

Graphical wizard that writes the same config as CLI **`mscc-init`** under  
**`$HOME/.local/mscc/`**.

- **Operator** speaker / mic — user picks from PortAudio (**any rate**; sdrcore resamples if not 96 kHz)
- **Hidden from picker:** digi VirtualA/B **and** Proficio/Multus radio I/Q (not operator phones/mic)
- **Digi** fixed: **VirtualA** / **VirtualB.monitor** (not selectable)
- CAT port + PTT pin, keyer, Proficio MKII vs legacy (PROFICIO-MKII), optional USB serial
- After write: **Yes/No** dialog to start MSCC servers (`mscc start`)
- If servers are **already running** at launch: offer to **stop** them; if you decline, init **exits** (no reconfig while they hold audio/CAT)

For day-to-day radio use, run the **MSCC client on a PC** against servers on the Pi.  
This tool configures the **Pi**.

## Requirements (Pi desktop)

```bash
sudo apt install -y python3 python3-tk python3-pyaudio
# optional USB serial string:
sudo apt install -y python3-usb
```

Pulse/PipeWire digi sinks (from main `mscc` package):

```bash
mscc-virtual-audio
# or: systemctl --user enable --now mscc-virtual-audio
```

## Run from source

```bash
cd /path/to/mscc-init-gui
chmod +x mscc-init-gui
./mscc-init-gui
```

## Install .deb (when built)

```bash
sudo apt install -y ./mscc-init-gui_*.deb
```

Then open the menu: **MSCC Init** (or run `mscc-init-gui`).

**Do not run under `sudo apt` postinst** — use your normal desktop user so Pulse and `$HOME` are correct.

## Build .deb

On Linux / WSL with `dpkg-deb`:

```bash
./build-deb.sh
```

Produces `mscc-init-gui_<version>_all.deb`.

## Files written

| File | Source |
|------|--------|
| `mscc.ini` | USB serial + host ports |
| `cw.ini` | Keyer flag + defaults |
| `i2c.ini` | Fortis defaults |
| `comm-port.ini` | CAT + PIN |
| `digital-speaker.ini` | always `VirtualA` |
| `digital-microphone.ini` | always `VirtualB.monitor` |
| `operator-speaker.ini` | chosen playback name |
| `operator-microphone.ini` | chosen capture name |

## Notes

- Optional 96 kHz filter remains in code (`require_96k=True`) but GUI default is off.
- Virtual* digi devices are hidden from operator lists.
- CLI `mscc-init` remains available from the main `mscc` package for SSH/headless.
