# MSCC for Windows — Change Log (draft)

Installer: `mscc-net9-R<version>-install.exe` (WPF client + Windows servers). Install the newest one; older installers in this folder are kept for roll-back only.

## R11.12.2 — 2026-11-12  (`mscc-net9-R11-12-2-install.exe`, title bar 11.11.4)

### Fixed
- Keyer Firmware Update (`keyer-flash-gui.exe`): no more blank command windows flashing up when it starts and at each button (the check for running servers now runs hidden).
- Everything else unchanged from R11-12-1.

## R11.12.1 — 2026-11-12  (`mscc-net9-R11-12-1-install.exe`, title bar 11.11.4)

### Added
- Keyer firmware update over USB, no PICkit: `C:\mscc-net9\Keyer Firmware\` holds `keyer-flash-gui.exe` (Start menu **Keyer Firmware Update**), `keyer-flash.exe` (command line) and the keyer files `Keyer-MCU-app-20261112.hex` (for the update tool) and `Keyer-MCU-full-20261112.hex` (PICkit only: bootloader + app). Close the servers first; the tool refuses while they run. Keyer settings and CQ memories are kept. The keyer needs the new bootloader put on once with the PICkit (the full file).

### Changed
- Black Pill firmware 3.16 (`BlackPill-25MHz-3.16-app.bin` / `-boot-app.bin`): passes the keyer update through to the keyer chip (USB opcode 0x9F). Nothing else changed.
- Client, servers and PSoC firmware unchanged from R11-11-5.

## R11.11.5 — 2026-11-11  (`mscc-net9-R11-11-5-install.exe`, title bar 11.11.4)

### Changed
- Black Pill firmware files now carry their version, like the PSoC files: `BlackPill-25MHz-3.15-app.bin` and `BlackPill-25MHz-3.15-boot-app.bin` (were `app-firmware-async.bin` / `boot-app-firmware-async.bin`). Same firmware (3.15). `bootload-PA8.bin` is unchanged. The ST-Link script's `-Image combined` picks the newest versioned file.
- Manuals: a "Firmware update required" note: upgrade the radio firmware for your transceiver (the calibration is now stored in the radio).
- Client and servers unchanged from R11-11-4.

## R11.11.4 — 2026-11-11  (`mscc-net9-R11-11-4-install.exe`, title bar 11.11.4)

### Added
- S/W window: new SPECTRUM SIZE slider (20–80 %) under Climbing. It sets how much of the display height the spectrum uses; the waterfall gets the rest. Default 40 % (was fixed at 38 %). One setting for all bands, saved.

## R11.11.2 — 2026-11-11  (`mscc-net9-R11-11-2-install.exe`, title bar 11.11.2)

### Changed
- Power meter: the dial is now labelled in watts and picks a full scale of 10, 20, 50, 100, 200, 500 or 1000 W (was ×1 / ×10 / ×100). About 110 W now reads on the 200 W scale, just past the middle, instead of near the bottom of the 1000 W scale.
- RX IQ START: the client now says "receive" before it sends the band. After a TX I/Q calibration, RX IQ START could retune to the TX calibration frequency and put the transmit server in TUNE.
- ms-sdr 3.208: WSJT-X right-click Tune (CAT `AC111;`) now gives a 10-11 s tune carrier, the same as the client's TUN button (SWR meter tune flag first, then RF; back to the old mode after). The client shows "Server owns TX" while it runs.

## R10.13.2 — 2026-10-13  (`mscc-net9-R10-13-2-install.exe`, title bar 10.13.2)

### Changed
- RX IQ tab: RESET ALL is now labelled APPLY ALL (it copies the current value to all bands). START now reads ACTIVE while a session runs. Cancelling APPLY now says the value was not saved.

## R10.13.1 — 2026-10-13  (`mscc-net9-R10-13-1-install.exe`, title bar 10.13.1)

### Changed
- S/W window: REFRESH is now SMOOTHING. It sets the host's spectrum smoothing (0 = light, higher = calmer; default 3) and the spectrum and waterfall always run at full speed (no more dropped frames). The waterfall therefore scrolls faster than with the old REFRESH 3-4.
- Waterfall: new lines now appear at the top and move down (Falling) by default. Tick "Climbing" for the old direction. Existing settings are moved to Falling once.
- S/W window: BASELINE and the waterfall HIGH / LOW / GAIN / ZERO are remembered per band (bands with different noise keep their own settings). A band you have not adjusted keeps the current values.

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
