# MSCC for Windows — Change Log (draft)

Installer: `mscc-net9-R<version>-install.exe` (WPF client + Windows servers). Install the newest one; older installers in this folder are kept for roll-back only.

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
