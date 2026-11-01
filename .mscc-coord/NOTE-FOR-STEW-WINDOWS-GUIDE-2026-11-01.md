# For Stew: Windows Operator's Guide, Release 1 draft

From Ron, 2026-11-01. The Windows guide has been rewritten for client 10.31.2:
`docs/manuals/MSCC-Operators-Guide-Windows-Release-1-DRAFT.docx` (43 pages, pushed in ac794b7 / 07ffd58).
I am reviewing it now in LibreOffice. It is a Word file from now on (the Release 0 PDF had no
editable source in the repo), so edit the .docx directly.

## What changed

- New order: what MSCC is (with a glossary), install and first start, firmware update,
  operating how-tos (SSB, CW, AM/FM, WSJT-X, amplifier), the controls, calibration in job order
  (FREQ CAL, RX IQ, QRP CAL, TX IQ, AMP CAL, Save settings), troubleshooting.
- Written for users: no server program names, file paths only in a "for support" appendix.
- Brought up to date with your changes since 10.8.1: calibration stored in the radio, TX IQ
  PHASE, RX IQ APPLY ALL, ALC always on, SMOOTHING, S-meter dBm readout, waterfall Climbing.
- POTENTIA / QSK now says what the firmware does (my 9/29 answer, never applied before).
- Control lists are keyed to the label printed on each control, so the pictures need no numbers.

Gray "DRAFT" boxes in the text and Appendix C list everything still open.

## Questions for you

1. Firmware table (section 2.6): which files does installer R10.31.2 ship in
   `C:\mscc-net9\PSoC Firmware`? The guide lists 1.236, 2.153, 3.242, 4.243, 5.120, 6.236,
   7.243, 8.242. Are 4.243 and the Ultimus MKII files ready to list, or not until flashed?
2. Does the calibration stored in the radio survive a firmware update (bootloader) on every
   radio?
3. ALC: Release 0 told users "just into the yellow". With the new ceiling and your peak meter,
   what reading should a user aim for, for voice and for digital?
4. AMP CAL: is "adjust until the amplifier just reaches its rated output" the right target?
5. Would you retake these pictures on your rig with 10.31.2 (or later)? Main window and title
   bar, RX/TX tab (still shows the ALC button), S/W window, RX IQ tab, TX IQ tab with PHASE
   plus a before / after example, Debug Log, bootloader with a `C:\mscc-net9` path, AMP CAL tab
   (old one is cropped). Until then the captions say what differs.

## Things the guide found in the client / servers

- **Reset to factory doesn't stick.** TX IQ RESET ALL and SETTINGS "Reset configuration" reset
  the PC files, but the values stored in the radio win at the next start. The guide warns about
  this for now (section 7.8). It needs the radio-side clear you have on your list.
- **Start tooltip** still says it is disabled until COM + speaker/mic are set; since 10.8.1 only
  Operator Out is needed.
- **"When done, press Save settings"** on QRP CAL and TX IQ: with calibration in the radio it is
  only needed for AMP CAL. Change the text?
- **"Full Power"** (RX/TX tab) means AMP on, which lowers the drive. Would "AMP" (as on the right
  rail) be clearer?
- **POTENTIA / QSK tooltip** says "Amplifier PIN diode T/R switching"; it is really "Potentia
  50/100 attached: amp line and PA keyed together".
- **Windows NR** is still the old adaptive filter (see NOTE-FOR-STEW-RECV-NR-SPECTRAL-2026-10-31);
  the guide's NR paragraph will be updated when you port the new one.

The Linux and Remote guides can reuse most chapters once the Windows one is settled.
