# MSCC for Ubuntu Desktop (x86_64 / amd64) — Change Log (draft)

Currently shipped in this folder:

| Package | Version |
|---------|---------|
| `mscc` (servers) | 1.0.49 |
| `mscc-ui` (Avalonia client) | 0.6.71 |
| `mscc-firmware` (optional radio firmware files) | 1.0.0 |
| `mscc-init-gui` | 1.0.13 (unchanged this period) |
| `mscc-portaudio` | 19.8.2 (unchanged this period) |

## mscc 1.0.49 / mscc-ui 0.6.71 — 2026-10-02

### Added
- **Save settings** in MSCC UI saves the live TX IQ, QRP and amplifier calibration for the connected radio on the server, filed per radio line (`~/.local/mscc/cal/<line>/`). Frequency (PPM) calibration is not part of this.
- The servers detect which radio is connected: the same radio keeps its live calibration; a different radio gets its saved calibration, or the factory calibration if it has none.
- Firmware Upload (bootloader GUI): **Load File** opens `/usr/share/mscc/firmware` when the `mscc-firmware` package is installed. If it is not installed you get a warning and can still browse anywhere.

## mscc-firmware 1.0.0 — 2026-10-02 (new, optional)
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

## Not yet in an installer / upcoming
- Receiver DC blocker (removes the spur about 12 kHz below the VFO) and full-block spectrum FFT — built in the sources (recv 3.143), waits for the next `mscc` package.
- 800-bin spectrum resolution fix (800 was not reaching the receiver) — in the sources after 1.0.49, waits for the next `mscc` package.
- CW tab **PHONES** checkbox removal — done for the Pi build of mscc-ui; the amd64 `mscc-ui_0.6.71` in this folder was built before that change and still shows it.
- Spectrum "strongest bin per display point" (done on the Pi, not yet ported).

Earlier changes: see git history.
