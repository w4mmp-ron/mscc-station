# For Stew: answers on QRP save, status, resets, ALC 0.89 and 2 TONE

From Ron, 2026-11-01. This replies to `NOTE-FOR-RON-CAL-SAVE-RESET-AND-2TONE-2026-11-01.md` and to
the ALC part of `NOTE-FOR-RON-CAL-STATUS-LAMPS-2026-11-01.md`.

## QRP save step, status opcode and resets

- **New ms-sdr with an old client:** option **(b)**. Update the client: the new servers need the
  new client. No fallback in ms-sdr. A line in the release notes is enough.
- **Status opcode:** **0x2D**, one dedicated opcode with the kind byte. Agreed.
- **Resets:** agreed. No factory values ever go into the radio; reset = clear. Yes to an RX IQ
  reset as well.
- **Type 6 clear:** yes, it needs a kind. Proposal:
  - wValueHi = 6
  - wValueLo = band, or 0xFF for all bands
  - **wIndex = kind:** 1 = RX IQ, 2 = TX IQ, 3 = power, 0 = all kinds

  It stays a control READ with a data stage, returning the 2-byte status (as in
  `NOTE-FOR-STEW-CAL-CLEAR-2026-10-31.md`). The Black Pill gets the same handler.

## ALC ceiling 0.89

Yes, 0.89 is on purpose. We measured it on my rig on 2026-10-31 (Proficio MKII PTT, power read
on the ATU-100). Lower ceilings gave too little output power:

| Pi trans ALC ceiling | WSJT-X steady tone | TUN |
|---|---|---|
| 0.45 (your Windows value) | about 40 W | 109 W |
| 0.80 (= the TUN level) | 89 W | 109 W |
| 0.89 (Pi trans 3.149, in use now) | 95 W | 109 W |

A steady tone at 0.89 stays **below** TUN power: about 87 %, not 124 %. The math assumes power
rises with the square of the drive, but the PA is near compression up there. After 3.149 I
recalibrated power (QRP and QRO), and the output power is right. I haven't done a splatter check
at 0.89, because I have no analyzer.

Your two-tone test with a spectrum analyzer is the right way to settle it: PEP and IMD at 0.8
against 0.89, on a calibrated rig.

## Two-tone test

Good idea, and it's yours to build: trans, ms-sdr and the client, on both Windows and the Pi. Pick
any opcode that is free in both the Windows and Pi trees; you don't need to ask me. Please name
the one you used in your note when you push it.

Also, thanks for checking that calibration survives a firmware update. I removed that draft item
from the Windows and Linux guides.

Ron
