# Black Pill 3.27: USB audio now shows up like the PSoC (2026-11-24)

Stew,

This answers sections 3 and 5 of your note
(`QUESTIONS-FOR-RON-blackpill-usb-audio.md`). Black Pill firmware **3.27**
makes the pill's USB audio look like the PSoC in Windows. It is flashed in
my rig, and the Pi servers found the radio on receive and transmit.

**3.27 includes the 3.26 HDSDR and Quisk fixes**
(`NOTE-FOR-STEW-PILL-3.26-HDSDR-QUISK-2026-11-24.md`). Flash 3.27 only.

**What changed in 3.27** (your items 1 to 6)

1. **Receive input terminal: Line connector (0x0603)**, was Microphone.
2. **Transmit output terminal: Line connector (0x0603)**, was Speaker.
3. **The transmit feature unit is gone.** The output terminal's source is
   now the input terminal, as on the PSoC. The mute and volume handlers
   are removed too.
4. **Strings:**
   - The manufacturer is now "www.multus-sdr.com".
   - The vendor interface is now "Multus SDR Control".
   - The other strings are unchanged.
5. **Channel config is L+R (0x0003)** on all four places, was
   NON_PREDEFINED.
6. **bcdDevice is now 0x0102** (was 0x0101), so Windows enumerates it
   fresh.

I checked the built image's descriptor bytes: the lengths add up, there is
no feature unit, and both radio-side terminals are type 0x0603.

**Item 7: I'm keeping UAC2 asynchronous with feedback**, as you
recommended. The codec clock runs free at about 95.99 kHz. Going to UAC1
synchronous would mean locking the I2S clock to USB SOF, which is real
work, and nobody would see the difference. With items 1 to 6, Windows shows
`Line (Multus IQ Sound)` both ways, which is the part that matters.

**No server changes are needed.**

- **Windows:** the servers open the radio by VID:PID only.
- **Pi:** the servers look for "Multus" anywhere in the sound device name.
  - The PulseAudio name is now
    `alsa_input.usb-www.multus-sdr.com_Multus_IQ_...` (the PSoC's form).
  - On my rig, sdrcore-recv picked it for I/Q capture, and sdrcore-trans
    picked it for transmit output, both at 96 kHz.

**Please test (Windows)**

1. Close MSCC and the servers. Flash 3.27 the same way you flashed 3.25.
2. Windows creates new sound devices. Check Sound settings:
   - both directions should show as **`Line (Multus IQ Sound)`**
   - there should be no "Audio enhancements" and no volume slider in the
     I/Q path
3. Optional: remove the old hidden "Multus IQ Sound" entries in Device
   Manager (View → Show hidden devices). That clears the "7-" numbering.
4. **Point MSCC at the new devices**, then check receive and transmit.
5. Then the 3.26 tests: HDSDR and Quisk.
   - HDSDR: pick the new Line devices.
   - Start MSCC fresh afterwards.

The same reminder as before: don't press "Factory default" in HDSDR on a
Multus radio.

**Section 5, housekeeping**

- `dist\pill-diag.exe` stays tracked in git. It is rebuilt and committed
  whenever `pill-diag.py` changes.
- The how-to is `NOTE-FOR-STEW-PILL-DIAG-HOWTO-2026-11-24.md`.

Ron
