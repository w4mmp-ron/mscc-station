# MSCC for Ubuntu Desktop (x86_64 / amd64) — Change Log (draft)

Currently shipped in this folder:

| Package | Version |
|---------|---------|
| `mscc` (servers) | 1.0.52 |
| `mscc-ui` (Avalonia client) | 0.6.74 |
| `mscc-firmware` (optional radio firmware files) | 1.0.1 |
| `mscc-init-gui` | 1.0.13 (unchanged this period) |
| `mscc-portaudio` | 19.8.2 (unchanged this period) |

## mscc 1.0.52 / mscc-ui 0.6.74 — 2026-10-11

### Fixed
- Spectrum: a steady signal no longer drops 10-15 dB and jumps back while tuning (each screen pixel now shows the strongest of the points it covers, in the client and in the receiver).
- Spectrum dB scale now matches real dB: a 20 dB change in signal moves the trace 20 dB, so one dB CAL holds at every level.
- Spectrum: no more dark hole (or spike) at VFO -12 kHz; the noise there is levelled to match its surroundings. After a transmission the middle of the spectrum no longer shows a slow-fading dip.

### Added
- S-meter: click the reading to switch between S units and dBm. The choice is remembered.

### Changed
- Spectrum resolution fixed at Max (3200); the selector is gone.
- On first start, dB CAL trim, GRID MIN and the waterfall LOW/HIGH are reset once to new defaults (trim 0 = -83.6, GRID MIN -155, waterfall Proficio -115/-40, Geminus -105/-35). Set dB CAL once with a known signal if you need exact levels.
- S-meter S units round to the nearest unit (S9 is -76 to -73 dBm).
- Spectrum smoothing (PANADAPTER_AVERAGE in ~/.local/mscc/user_controls.ini): 0 looks as before; 1 to 100 give a calmer trace at full frame rate (value x 0.025 s).
- Receive server 3.144 (Ubuntu). Install mscc and mscc-ui together.

## mscc 1.0.51 / mscc-ui 0.6.73 — 2026-10-08 (superseded)

### Fixed
- Transmit server starts when the operator mic is missing or not found (TX voice off; TUNE/CW still work, I/Q output-only).
- Digital audio uses the digital mic even when the operator mic is missing.
- RX IQ START works on 2200 m and 630 m (UI).

If your mic changes, re-run MSCC Init, then Stop/Start MSCC.

## mscc 1.0.50 / mscc-ui 0.6.72 — 2026-10-07 (superseded)

### Fixed
- Receiver DC blocker (spur about 12 kHz below the VFO) and full-block spectrum FFT.
- SPECTRUM RESOLUTION 800 now reaches the receiver.
- MSCC UI dock icon shows as running on the pinned icon (no extra gear icon); the window shows the MSCC icon.

### Removed
- CW tab **PHONES** checkbox (amd64).

## mscc 1.0.49 / mscc-ui 0.6.71 — 2026-10-02 (superseded)

### Added
- **Save settings** in MSCC UI saves the live TX IQ, QRP and amplifier calibration for the connected radio on the server, filed per radio line (`~/.local/mscc/cal/<line>/`). Frequency (PPM) calibration is not part of this.
- The servers detect which radio is connected: the same radio keeps its live calibration; a different radio gets its saved calibration, or the factory calibration if it has none.
- Firmware Upload (bootloader GUI): **Load File** opens `/usr/share/mscc/firmware` when the `mscc-firmware` package is installed. If it is not installed you get a warning and can still browse anywhere.

## mscc-firmware 1.0.1 — 2026-10-07
### Changed
- One firmware file of each type per radio: only the dated `<Name>-YYYYMMDD.cyacd` and `.hex`. The undated duplicates (byte-identical) are gone; upgrading from 1.0.0 removes them.

## mscc-firmware 1.0.0 — 2026-10-02 (superseded by 1.0.1)
### Added
- Radio (PSoC) firmware files for all eight supported radios, installed to `/usr/share/mscc/firmware/<radio>/` for use with Firmware Upload.

## mscc 1.0.48 — 2026-10-02 (superseded by 1.0.49)
### Added
- Factory calibration tables (TX IQ, frequency, QRP power) per radio line installed with the package in `/usr/share/mscc/factory/`.

## mscc-ui 0.6.68 – 0.6.70 — 2026-09-29/30 (superseded by 0.6.71)
### Added
- **SPECTRUM RESOLUTION** control (800 / 1600 / 3200), matching the Windows client.
### Fixed
- Remote audio: switching between Phones and Digital, or turning Remote on with Digital selected, no longer crashes the audio stream.

Earlier changes: see git history.
