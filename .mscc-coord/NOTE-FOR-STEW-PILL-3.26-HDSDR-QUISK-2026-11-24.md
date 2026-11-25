# Black Pill 3.26: HDSDR and Quisk fixes (2026-11-24)

Stew,

Thanks for tracking these down. Your note
(`QUESTIONS-FOR-RON-blackpill-softrock-quisk-hdsdr.md`) was right on all
three. They are fixed in Black Pill firmware **3.26**. It is flashed in my
rig, and MSCC works the same as before.

**What changed in 3.26**

1. **`CMD_GET_XTAL`** now returns 112.64 MHz instead of 0, the same fake
   crystal value the PSoC uses. This is the fix for HDSDR's "Xtal 0.000000
   not in range" message.
2. **`CMD_SET_FREQ`** now follows the mode the program asks for, as the
   PSoC does. The SoftRock mode used by HDSDR and Quisk (`SI570_DLL`) gets
   the SoftRock frequency format. The MSCC mode (`SI5351_DLL`) stays plain
   Hz. `CMD_GET_FREQ` reads back in the same format. This is the fix for
   Quisk tuning to about 118 MHz.
3. **`CMD_GET_STARTUP`** now sends its bytes in the right order
   (14.08 MHz).
4. **The pill now starts in SoftRock mode, like the PSoC.** Programs that
   never ask for a mode get the SoftRock behaviour. MSCC is not affected,
   because ms-sdr always switches the radio to its own mode at startup.

I did not do the two minor items (`CMD_GET_SI570` and
`CMD_SET_FREQ_REG`). If HDSDR still complains after 3.26, tell me and I
will add them.

**No server changes are needed.** I checked the Pi servers and ms-sdr on
Windows. They only use `CMD_SET_FREQ` / `CMD_GET_FREQ`, and they always set
the MSCC mode first, so they see no difference.

**Please test**

1. Flash 3.27, which includes these fixes (see the 3.27 note).
2. Check that MSCC works as before.
3. Close MSCC and the servers, then try HDSDR and Quisk.
   - HDSDR: pick the Multus I/Q sound devices again.
   - Quisk: your "Proficio Pill" profile should work as it is.
4. If you go back to MSCC after HDSDR or Quisk, start MSCC fresh. It
   switches the radio back to its own mode.

**Reminder:** don't press "Factory default" in HDSDR on a Multus radio. It
overwrites the frequency calibration.

The USB audio questions (Line instead of Microphone/Speaker, and UAC2 vs
UAC1) are answered in `NOTE-FOR-STEW-PILL-3.27-USB-AUDIO-2026-11-24.md`.

Ron
