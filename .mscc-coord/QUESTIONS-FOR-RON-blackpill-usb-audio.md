# Questions for Ron: Black Pill USB audio vs PSoC (2026-11-24)

**STOP notes (read first)**
- Nothing in this note has been changed in any repo. No firmware was built or flashed. Nothing was committed.
- Do not rename the audio devices on a radio that is in use until MSCC's sound-device choices are reset afterwards: new descriptors make Windows create new endpoints, and MSCC must be pointed at them again.
- Stew's request is firm: the Black Pill must show up in Windows exactly like the PSoC does.

## 1. What we found

- **Black Pill receive on Windows is fixed.** The cause was Windows, not the radio. The pill's receive endpoint shows as `Microphone (7- Multus IQ Sound)`, and "Audio enhancements" was on for that microphone. That processing wiped out the I/Q. With enhancements off, receive works. The level now follows the Windows microphone slider (60 matches the input).
- The PSoC shows both directions as `Line (Multus IQ Sound)`. Windows does not offer microphone enhancements or a microphone slider on a Line endpoint, so the PSoC never had this problem.
- Receive on the Pi works (after moving USB ports for bandwidth).
- **Transmit is still a comb.** The 12 kHz test tone comes out with comb lines, and the wanted carrier is about 50 dB low.

## 2. Descriptor differences, pill vs PSoC

Pill: `psoc-replacement-stm32/proficio-stm32f411-25MHz/firmware/pio-async/` (`include/usb_descriptors.h`, `src/usb_descriptors.c`, `include/tusb_config.h`).
PSoC: `mscc-station/radio-psoc-firmware/Proficio-Ultimus-MKII-PTT/Proficio-MKII-PTT.cydsn/Generated_Source/PSoC3/USBFS_descr.c`.

| Item | PSoC (USBFS_descr.c) | Black Pill 3.24 | Same? |
|---|---|---|---|
| Audio class | UAC1, bcdADC 1.00 (l.94) | UAC2 (TinyUSB audio20, usb_descriptors.h l.38-107) | NO |
| VID:PID | 16c0:05dc (l.44-45) | same (usb_descriptors.c l.20-21) | yes |
| bcdDevice | 0x0003 (l.46) | 0x0101 (usb_descriptors.c l.22) | no (harmless) |
| iManufacturer | "www.multus-sdr.com" (idx 15, l.47 / l.404) | "Multus" (usb_descriptors.c l.108) | NO |
| iProduct | "Multus IQ" (idx 14, l.48) | "Multus IQ" (l.109) | yes |
| iSerial | silicon die ID (0x80, l.49) | "MKII-PTT-F411" (l.113) | no (harmless) |
| Vendor interface string | "Multus SDR Control" (idx 12, l.75) | "Multus Control" (l.117) | NO |
| Audio Control interface string | "Multus IQ Sound" (idx 9, l.87) | "Multus IQ Sound" (stridx 4, usb_descriptors.c l.43, l.114) | yes |
| Receive path terminals (radio to PC) | Input terminal **0x0603 Line connector** (l.105), output terminal 0x0101 USB streaming (l.118) | Input terminal **0x0201 Microphone** (usb_descriptors.h l.80), output terminal 0x0101 USB streaming (l.82) | **NO: this is why Windows says "Microphone"** |
| Transmit path terminals (PC to radio) | Input terminal 0x0101 USB streaming (l.129), output terminal **0x0603 Line connector** (l.142) | Input terminal 0x0101 USB streaming (l.71), output terminal **0x0301 Speaker** (l.78) | **NO: this is why Windows says "Speakers"** |
| Feature unit (mute/volume) | **none** (AC header total 0x34 = 2 input + 2 output terminals only, l.95) | **Present on the transmit path**: mute + volume, master and both channels (usb_descriptors.h l.74-77). Volume range -20..0 dB is reported (usb_audio_uac2.c l.312-314), but the value is only stored and never applied to the samples (l.342) | **NO** |
| Terminal names (iTerminal) | 0 (none) | 0 (none) | yes |
| Streaming interface strings | Receive "Multus IQ Receive" (idx 10, l.157/169), transmit "Multus IQ Transmit" (idx 11, l.222/234) | Same text (usb_descriptors.c l.115-116) | yes |
| Channels / channel config | 2, wChannelConfig 0x0003 = L+R (l.107-108, l.131-132) | 2, NON_PREDEFINED (usb_descriptors.h l.72, l.81, l.88) | no (minor) |
| Sample rate | 96000 fixed (tSamFreq 0x017700, l.190, l.255) | 96000 fixed (tusb_config.h l.49; usb_audio_uac2.c l.269) | yes |
| Format | PCM 16-bit, 2-byte slots (l.187-188, l.252-253) | PCM 16-bit, 2-byte slots (tusb_config.h l.53-56) | yes |
| Endpoints | IN 0x82, OUT 0x03, both bmAttributes 0x2D = isochronous **synchronous** (l.196-197, l.261-262) | Isochronous **asynchronous** (usb_descriptors.h l.92, l.107) | NO |
| Feedback endpoint for transmit | none | explicit feedback endpoint 0x82 (usb_descriptors.h l.25, l.97; tusb_config.h l.69) | NO |
| Max packet size | 384 bytes = exactly 96 frames per 1 ms (l.198, l.263) | 388 bytes = up to 97 frames (audio.h l.15, tusb_config.h l.73-83) | NO (follows from async) |

Notes:
- The Windows servers open the radio by VID/PID only. Manufacturer and product strings are passed empty (`ms-sdr-MKII/source/main.c:1172`, `mscc-init/sources/main.c:761`), so changing the strings is safe for them.
- The "7-" in `Microphone (7- Multus IQ Sound)` is Windows numbering a repeat install of the same device name. It is not in the firmware. Removing the old hidden "Multus IQ Sound" entries in Device Manager clears it.

## 3. Request: make the pill's USB audio look like the PSoC

Please change `pio-async`:
1. Receive input terminal: 0x0201 Microphone -> **0x0603 Line connector** (usb_descriptors.h l.80).
2. Transmit output terminal: 0x0301 Speaker -> **0x0603 Line connector** (usb_descriptors.h l.78).
3. **Remove the transmit feature unit** (usb_descriptors.h l.74-77 and its length in l.43/l.66). Point the output terminal's source at the input terminal, as on the PSoC. Drop the mute/volume handlers in `usb_audio_uac2.c`. With no volume control, Windows has nothing to process or turn down, the same as the PSoC.
4. Strings: manufacturer "www.multus-sdr.com", vendor interface "Multus SDR Control" (usb_descriptors.c l.108, l.117). Keep "Multus IQ", "Multus IQ Sound", "Multus IQ Receive", "Multus IQ Transmit".
5. Channel config: L+R (0x0003) instead of NON_PREDEFINED.
6. Bump bcdDevice so Windows enumerates it fresh.
7. **Your decision, please answer:** a byte-for-byte match also means UAC1 plus synchronous endpoints with fixed 384-byte packets and no feedback. The pill's codec clock runs free (95.99 kHz from PLLI2S, not locked to USB), and that is why `pio-async` moved to asynchronous with feedback. Our recommendation: keep UAC2 asynchronous with feedback, and make items 1-6 identical. Windows then shows `Line (Multus IQ Sound)` for both directions, with no enhancements and no slider in the I/Q path. Go to UAC1 synchronous only if you can lock the I2S clock to USB SOF.

   **Answer:** (Ron, 2026-11-24) Items 1-6 are done in Black Pill 3.27. Item 7: keep UAC2 asynchronous with feedback. See `NOTE-FOR-STEW-PILL-3.27-USB-AUDIO-2026-11-24.md`.

## 4. Transmit comb: analysis and request

Facts:
- pill-diag step 5 (2026-11-22) plays its own 12 kHz tone at 10% in **exclusive mode**, which bypasses all Windows processing. Result: 0 pill underruns, 0 drops, 0 PC underruns.
- On the analyser, the two "twin" markers sit exactly at LO -36 kHz and LO -84 kHz (LO 14.074 MHz): the -3rd and -7th products of the 12 kHz tone. The wanted tone at 14.086 MHz was just outside the 100 kHz window.
- Stew now sees the comb with the wanted carrier about 50 dB low.
- The same 3.24 binary transmits clean on your rig. On Stew's side the comb followed the pill into his known-good second radio.
- Receive now works on the codec's clock set 2 (`PCM3060_ADC_USE_CLK2`). The DAC also uses set 2 by default (CSEL2 = 0), so the transmit clocks are proven good.

Best-cause ranking:
1. **Most likely: the transmit data line on Stew's pill or daughter board**: PB15 (I2S2_SD) -> J5 A18 -> PCM3060 DIN (pin 12). The DAC samples it on BCK2 rising (pin 10). Clean odd-only products with the wanted tone far down mean the DAC is getting the tone's bits wrong in a repeatable way (bad joint, slow or ringing edge, or bits read a clock late). The only part of the transmit chain not shared with the now-working receive is DIN.
2. **Adds to the MSCC transmit test, but not the pill-diag one: Windows processing on the "Speakers" endpoint** (enhancements, spatial sound, loudness) in shared mode. The fix in section 3 removes it. Until then, Stew should turn off enhancements and spatial sound on `Speakers (Multus IQ Sound)`. This cannot explain the exclusive-mode pill-diag comb.
3. **Unlikely: the USB OUT path** (async/feedback vs PSoC synchronous, 388 vs 384 packets, 96 kHz). A wrong rate or feedback gives underruns, drops or slips, which show up as noise or clicks, not a clean comb. pill-diag counted 0 of each.
- Not the comb, but check it: the transmit I/Q swap default (`audio.c` l.250-251 says a swapped transmit lands 24 kHz low, mirror image). Stew's MSCC TUNE had the strongest line 24 kHz below the dial.

Requests:
- (a) Confirm the PB15 -> A18 -> codec pin 12 routing on Stew's board revision.
- (b) Have Stew scope codec pin 12 (DIN) against pin 10 (BCK2) while pill-diag step 5 is keyed. DIN should be clean 3.3 V logic that changes on the falling BCK edge, at least 10 ns before the rising edge. Also re-run step 5 with the analyser centred at 14.074 MHz, span 200 kHz, to read the wanted tone against the comb.
- (c) If DIN looks right, say what firmware-side transmit check you want next.

   **Answer:**

## 5. Housekeeping

- `utility-scripts/Windows/dist/pill-diag.exe` is committed (cea409e7) together with `pill-diag.py` and `build-pill-diag.cmd`. Is a built .exe under `dist\` meant to be tracked in git?
- There is no NOTE-FOR-STEW on how to run pill-diag. Only the results note exists (`NOTE-FOR-STEW-PILL-DIAG-RESULTS-2026-11-22.md`). Please add a short how-to (where the .exe is, close MSCC first, dummy load for step 5, where the zip lands).

   **Answer:** (Ron, 2026-11-24) Yes, keep `dist\pill-diag.exe` tracked in git. Rebuild it with `build-pill-diag.cmd` and commit it whenever `pill-diag.py` changes.
## Update 2026-11-24 afternoon (Stew)

- **TX comb solved.** It was overdrive. The radio's existing factory cal (made with the PSoC) overdrives the pill. After QRP CAL and TX IQ cal with the pill, TX is clean on both the Pi and Windows (carrier -23.5 dBm, worst spur 54 dB down). The data-line scope check is no longer needed.
- **Cal note.** Cal values are host-side per radio, so a radio recalibrated for the pill is wrong for the PSoC, and the reverse. The pill is an experiment for now, so Stew is recalibrating every band. No change requested for the Proficio.
- **Maximus.** The Maximus will be a new radio with a new firmware major, built around the pill from the start. Stew is moving forward with it now that the pill works.
- **Hardware recommendation: add 47 ohm series resistors on LRCK and BCK.** Without them, Stew's board rang at over 4 Vpp, with noise on the high and low levels. With 47 ohm resistors in series at the source, the clocks are clean at about 3.5 Vpp, and signal quality improved noticeably. Recommend adding them to the Maximus (and any pill daughter-board respin) for LRCK and BCK, and consider MCLK as well.
