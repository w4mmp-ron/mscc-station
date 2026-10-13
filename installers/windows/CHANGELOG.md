# MSCC for Windows — Change Log (draft)

Installer: `mscc-net9-R<version>-install.exe` (WPF client + Windows servers). Install the newest one; older installers in this folder are kept for roll-back only.

## R10.13.0 — 2026-10-13  (`mscc-net9-R10-13-0-install.exe`, title bar 10.13.0)

### Changed
- TX ALC is always on. The ALC button is gone from the TX tab; the transmit server ignores an "ALC off" from older clients.
- Volume and mic gain (phones and digital): the slider settings are now saved on the host a few seconds after you stop moving them, and come back after a restart.
- Transmit server 3.147, ms-sdr 3.183 (Windows).

## R10.11.3 — 2026-10-11  (`mscc-net9-R10-11-3-install.exe`, title bar 10.11.3)

### Changed
- S-meter: the needle now moves smoothly in fractions of an S unit (6 dB per unit up to S9, 10 dB per unit above) instead of jumping a whole S unit. The digital readout still shows whole units (S9, S9+10) or dBm.
- S-meter: the receive server measures average power instead of the block peak, smoothed (fast rise, 0.3 s fall) and rounded to the nearest dB. Noise reads steadier and a little lower; a steady carrier reads the same.
- Receive server 3.147 (Windows).

## R10.11.2 — 2026-10-11  (`mscc-net9-R10-11-2-install.exe`, title bar 10.11.2)

### Fixed
- Spectrum: no more dark hole (or spike) in the middle of the spectrum and down the waterfall at VFO -12 kHz. The noise there is now levelled to match its surroundings.
- Spectrum: after a transmission the middle of the spectrum no longer shows a dip that takes many seconds to fade.

### Changed
- Spectrum smoothing (PANADAPTER_AVERAGE in user_controls.ini): 0 looks as before; 1 to 100 give a calmer trace at full frame rate (time constant = value x 0.025 s, e.g. 9 = 0.225 s). Signal levels are unchanged. The waterfall is smoothed the same way.
- Receive server 3.146 (Windows). Client unchanged apart from the version number.

## R10.11.1 — 2026-10-11  (`mscc-net9-R10-11-1-install.exe`, title bar 10.11.1)

### Added
- S-meter: click the digital readout to switch between S units (S9+10) and dBm (-63 dBm). The choice is remembered.

### Fixed
- Spectrum and waterfall: a steady signal no longer dips by 10-15 dB (and its waterfall line no longer breaks up) as you tune, at any zoom. Each screen pixel now shows the strongest point it covers instead of a single sample.
- Spectrum dB scale now matches real dB: a 20 dB change in signal moves the trace 20 dB (it moved only about 10 to 15 dB before), so one dB CAL holds at every level. The receive server's spectrum averaging no longer adds gain.

### Changed
- SPECTRUM RESOLUTION is fixed at Max (3200) and the selector is gone from the S/W controls. A saved Normal/High setting is ignored. Use the spectrum zoom to see more detail.
- At 1x zoom the noise floor on the spectrum looks slightly higher and smoother than before (same signal levels).
- On first start, dB CAL, the spectrum GRID MIN and the waterfall LOW/HIGH are reset once to new defaults (centre −83.6, GRID MIN −155, waterfall −150…−40), because the old values were set for the old scale. Signals now stand about twice as far above the noise floor, and the floor reads lower (about −140). Set dB CAL once with a known signal if you need exact levels.
- Receive server 3.145 (Windows). Install the client and the servers together (this installer does both).
- S-meter S units now round to the nearest unit (S9 is -76 to -73 dBm; it used to show S9 down to -78).
- Spectrum dB CAL centre set from a signal-generator step test (-70 dBm reads -70 with the trim at 0).

## R10.8.1 — 2026-10-08  (`mscc-net9-R10-8-1-install.exe`, title bar 10.8.1)

### Fixed
- MSCC no longer stops with "Server Not Responding" (keep-alive) or "SDRcore-trans initialization FAILED" when an audio device is missing or wrong. The transmit and receive servers check the saved device before opening it.
- RX IQ tab now works on 2200 m and 630 m (Geminus); it used to say "INVALID BAND (GENERAL)".

### Changed
- The radio's own **Multus IQ Sound** device is no longer offered in any audio list (it must never be used as a speaker or mic).
- Only **Operator Out** (speaker) is required to Start. Operator Mic, Digital Out and Digital Mic have a **(none)** choice, and the COM port is optional (blank = CAT off, no pop-up). Anything missing shows as a short orange setup note under Start.
- With no mic set, PTT in a voice mode shows "Set up audio (Mic) in the Settings tab to transmit voice." and does not key; DIG-U with no Digital Mic shows a similar message. TUNE and CW still key, and Remote audio is not affected.
- Note: if an older install saved a shortened mic name (for example "Line "), pick the mic again in SETTINGS.
## R10.4.0 — 2026-10-04  (`mscc-net9-R10-4-0-install.exe`, title bar 10.4.0)

### Fixed
- Saving calibration from a remote client now saves correctly on the radio host.

### Changed
- Spectrum: the receiver now removes the DC offset from the raw I/Q (DC blocker), which removes the spur that used to appear about 12 kHz below the VFO at the source. The old "notch" that blanked that spot on the display is gone.
- Spectrum: the FFT now uses the full sample block, so strong signals look narrower with less wide "skirt". Expect signal and noise levels on the spectrum to read slightly higher than before.

## R10.2.0 — 2026-10-02  (`mscc-net9-R10-2-0-install.exe`)

### Added
- **Save settings** now saves the calibration (TX IQ, QRP power and amplifier power) on the computer that runs the radio servers, filed under the radio line in use. This works for a local radio and for a client connected to a remote host.

## R10.1.2 — 2026-10-01  (`mscc-net9-R10-1-2-install.exe`)

### Added
- **Save settings** button: copies the live TX IQ, QRP and amplifier calibration for the connected radio into a per-radio store. Changing to a different radio puts the old radio's calibration away and loads the saved (or factory) calibration for the new one; starting again on the same radio keeps the live calibration.
- QRP CAL, AMP CAL and TX IQ tabs show "When done, press Save settings."

### Changed
- The **Save settings** button now sits at the bottom of the right panel, directly under LOG.
- QRP and QRO (amplifier) power calibration is now written by the transmit server once the slider has settled; a missing QRP table is no longer filled with a generic default.
- Shorter tooltips on Save settings and the calibration tabs.

## R9.30.4 — 2026-09-30  (superseded, no longer in this folder)

### Changed
- **PAN RESOLUTION** renamed **SPECTRUM RESOLUTION** (800 / 1600 / 3200). Label and tooltip only; your saved setting is kept.

## R9.30.1 – R9.30.3 — 2026-09-30  (superseded, no longer in this folder)

### Changed
- CW tab: unused **PHONES** checkbox removed. POTENTIA / QSK stays.
- POTENTIA / QSK tooltip now reads "Amplifier PIN diode T/R switching".
- Spectrum spur notch sized to the spectrum resolution (later replaced by the DC blocker in R10.4.0).

### Fixed
- FREQ CAL: pressing STOP a second time while a stopped run is still finishing no longer ends the clean-up early.

## Not yet in an installer / upcoming
- Spectrum: each display point showing the strongest FFT bin it covers (done on the Raspberry Pi servers; not yet ported to the Windows servers, and the client still draws one data point per pixel).

Earlier changes: see git history.
