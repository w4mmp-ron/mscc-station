# For Ron: QRP save step, calibration resets, and the 2 TONE button

From Stew, 2026-11-01. Reply to your `NOTE-FOR-STEW-CAL-STATUS-REPLY-2026-11-01.md`. Still no
code until Stew finishes the firmware tests.

## 1. QRP save step: agreed

- **Slider (0xA2):** live only. No file write, no radio write.
- When CALIBRATE is turned off, the pop-up asks **"Save this calibration?"**
  - **Yes:** 0xA2 with `value | 0x8000`. ms-sdr writes the radio, and trans writes `power_cal.ini`.
    The lamp turns green when the new mask comes back.
  - **No:** 0xA2 with the value from before the session (live only), as Cancel does today.
  - **Reject goes away.** Today it sends nothing, so the slider value stays live and has already
    been written to the radio.
- **One gap: a new ms-sdr with an old client.** The old client never sends the 0x8000 flag, so
  QRP calibration would silently never be saved. Options:
  - (a) ms-sdr keeps saving on slider steps until it has seen the flag once from this client.
  - (b) We always ship the client and the servers together (the Windows installer does, the Pi
    packages?).

  Stew is fine with (a) if it's simple for you; otherwise (b) with a version note.

## 2. Status masks: agreed

Layout as you proposed: kind (1 RX IQ, 2 TX IQ, 3 power), flags (bit 0 = the radio has a store),
uint16 mask, bit N = band N in MSCC order. Sent at connect, after each write to the radio, and
after a clear.

- **Opcode:** Stew leans to one dedicated opcode, **0x2D**, carrying the kind byte. It reads more
  clearly in the logs than reusing 0x57 / 0xA2, which mean "set" in the other direction. Reusing
  them is also OK if you prefer it.
- **Split of work:** as you said. You do the Pi ms-sdr and trans; Stew's side does the Windows
  ms-sdr, trans and the client (WPF and Avalonia).

## 3. How the calibration layers fit (Stew's picture)

| Layer | Where | Role |
|---|---|---|
| Factory | `C:\mscc-net9\factory\<freq, iq, power>\<line>\` (Pi: the package) | Shipped. Never edited. Targets: about 4.5 W on HF and 1 W on LF in QRP, TX IQ within FCC specs on every radio. |
| PC working copy | `%LocalAppData%\MSCC-NET9\cal\<line>\` + `LAST_LINE.txt` | Seeded from factory when a new product line connects. **Save settings** parks into it. Kept for radios without the 0xC8 store and for AMP CAL (not in the radio). Note: it is per product line, not per radio. |
| Radio (0xC8) | EEPROM | Only real user calibration. Wins at start for the bands it holds. |

Cases:

1. **New radio:** factory values in the working copy, the radio uses them, all lamps white.
2. **User calibrates a band:** saved to the radio, lamp green. A radio that already holds values
   shows green for those bands on any PC or Pi.
3. **Radio without the store:** works from the PC working copy and the local lamp file, as today.

## 4. Resets: clear the radio, never push factory values into it

Factory values never go into the radio (decision of 10-30). A reset **clears** the radio (your
type 6, as a control READ) so it falls back to the factory files.

- **SETTINGS → "Reset configuration to defaults":** client settings only (UI, audio, COM,
  colors). It no longer deletes the calibration cache. Today it does, but the radio's values win
  at the next start anyway: that is the guide's "reset doesn't stick".
- **Calibration resets, one per kind, each with a confirmation:**
  - **TX IQ RESET ALL:** type 6 clear for TX IQ (all bands). Reseed `iq.ini` from factory. TX IQ
    lamps white.
  - **QRP CAL RESET** (new): type 6 clear for power (all bands). Reseed `power_cal.ini` from
    factory. Lamps white.
  - **RX IQ reset:** the same for RX IQ, if you agree.
  - **FREQ CAL RESET:** not in the radio, so it stays factory-file only, as now.
- **Save settings:** needed only for AMP CAL and for radios without the store. The tooltip and the
  guides should say so.

Question: does type 6 take a kind as well as a band? Something like wValueHi = 6, wValueLo = band
or 0xFF, plus the kind in wIndex? Otherwise there's no way to clear only TX IQ.

## 5. 2 TONE button (client, trans, ms-sdr)

Stew's UI: a **2 TONE** button on the RX/TX tab, next to TUNE.

- Pressing it **arms** two-tone (the button shows active). When TUNE is then pressed, the radio
  sends a two-tone signal instead of the carrier. TUNE off stops it; 2 TONE stays armed until it
  is pressed again.
- trans generates 700 + 1900 Hz in place of the mic samples, **before the ALC**. So it goes
  through the real voice path (ceiling, 6 kHz LPF, modulator, PA).
- **Level: the SSB power slider, not the tune power slider.** The point is to show what voice will
  do at the user's SSB setting. The tones are sized so their combined peak equals the TUN carrier
  amplitude at the same drive (each tone -6 dB). So SSB slider at 100 % and an ALC ceiling at TUN
  level = PEP equals the calibrated power: 44 dBm per tone on an analyzer for 100 W, about 50 W on
  an average-reading wattmeter. Stepping the SSB slider down walks the IMD curve. No extra level
  control is needed.
- The radio must be in USB or LSB. If 2 TONE is armed in CW, AM or FM, the client switches to USB,
  or greys out the button. Disarm on Stop, and use the same auto-off timer as TUNE.
- **ms-sdr:** one new opcode (client → ms-sdr → trans) to arm or disarm. Keying stays the normal
  TUNE path, with SSB mode.

Questions:

- An opcode number for 2 TONE that is free on the Pi side?
- Still open from Stew's last note: your 0.89 voice ceiling sits about 11 % above the TUN amplitude
  (about 124 % of the calibrated power on peaks). Was that on purpose, or should the ceiling equal
  TUN (0.8)? 2 TONE would show it at once.
