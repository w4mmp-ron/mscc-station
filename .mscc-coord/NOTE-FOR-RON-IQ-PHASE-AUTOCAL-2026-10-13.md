# For Ron: I/Q phase, automatic RX I/Q, automatic power cal (Stew's replies)

From Stew, 2026-10-13. Replies to `NOTE-FOR-STEW-RX-IQ-TAB-2026-10-12.md`,
`NOTE-FOR-STEW-RX-IQ-BAND-DEFAULTS-2026-10-12.md` and the matching `rpi/CLAUDE.md` items.
Nothing has been changed for points 1-4. Point 5 goes ahead as a client brief.

## Where the I/Q correction is applied today (amplitude only, no phase)

Read on the Windows and Ubuntu sources, 2026-10-13. Line numbers are from `867eab9`.

- **Recv (receive I/Q):**
  - The slider value (`IQ_OFFSET` per band, `recv-iq.ini`) goes to `IQ_calc(int)`:
    - Windows `mscc-ui/windows-work-tree/SDRcore-recv/sources/iq.c:386`
    - Ubuntu `linux/SDRcore-recv-linux/sources/iq.c:320`
  - That turns it into a gain on I or Q only: `cormult = 1 - 0.0005 * |value|` (5 % over 100 steps). A positive value scales I, a negative one scales Q.
  - It's applied to the raw samples in `framesToComplex`:
    - Windows `SDRcore-recv/sources/dsputils.c:478` (`real * mystate.iMult`) and `:481` (`imag * mystate.qMult`)
    - Ubuntu `linux/SDRcore-recv-linux/sources/dsputils.c:467` / `:470`
  - It's set from the band (`udp_thread.c`, Windows 1191 / 1211 / 1257, Ubuntu 1300 / 1328 / 1374).
- **Trans (transmit I/Q):**
  - Same scheme, `IQ_calc` at Windows `SDRcore-trans/sources/iq.c:231` and Ubuntu `linux/SDRcore-trans-linux/sources/iq.c:231`.
  - It's applied to the output samples:
    - Windows `SDRcore-trans/sources/main.c:201` / `:203` (`outcplx.real * iMult`, `.imag * qMult`), also `sdrcore-trans.c:169` / `:171`
    - Ubuntu `linux/SDRcore-trans-linux/sources/main.c:344` / `:346`
  - It's set from the band (`udp_thread.c`, Windows 855 / 868 / 902, Ubuntu 990 / 1011 / 1045).
- There is **no phase term** anywhere in either path. The Pi copies are the same as far as Stew knows; please confirm.

## 1. RX I/Q is set by eye today; the UP 24 kHz tick and an automatic routine

- Today RX I/Q is set by eye on the S meter or spectrum. The UP 24 kHz tick at START (your point 4) is really only needed for an **automatic** RX I/Q routine. For a manual check, Stew is fine either way.
- An automatic routine could come later. It would be **mostly server side**:
  - Recv measures the image on the raw I/Q, at the known image frequency of a strong carrier.
  - It steps the amplitude to the minimum image, then the phase (see point 3), and repeats once if needed.
  - It reports progress and the result.
- The client would only get a START AUTO button, a progress line and APPLY, the same way FREQ CAL auto works today.
- The value would be measured on the in-passband image (UP 24 kHz), as you showed: 58 against about 87 on your rig, 20 m.

## 2. Factory per-band RX I/Q defaults

- Stew isn't sure this is worth it.
- RX I/Q is generally good on the rigs he knows. The image only shows with very strong signals.
- He'd rather not measure every band on every model unless the unit-to-unit spread turns out to be small and an image actually bothers users.
- The `CAL=1` mark idea is fine if we ever do it. **Parked for now.**

## 3. Phase adjustment, TX and RX

- **TX needs phase.** The amplitude correction makes the upper bands acceptable but doesn't eliminate the image; the rest is phase error.
- **RX needs it too**, for the same reason, though it matters less.
- Proposal: **manual phase first, automatic on top later.**
  - **Server:**
    - A phase correction term in the same places as the amplitude.
    - For recv in `framesToComplex`, for example `Q' = Q + k·I` (small angle, k = sin φ), with the gain applied as today.
    - For trans on the output, `I' = I + k·Q`, or the equivalent rotation.
    - A per-band saved value next to `IQ_OFFSET` (e.g. `IQ_PHASE=`), read by field name so old files still load.
    - A new opcode (or sub-value) to set it live, like the amplitude.
  - **Client:** a PHASE slider on the RX IQ and TX IQ tabs beside the existing amplitude slider, with the same APPLY / APPLY ALL / RESET behaviour.
- This touches recv, trans and ms-sdr on all three platforms (Windows, Ubuntu, Pi), plus the client.
- **Question for Ron:** do you want to do the Pi side first, as with the smoothing, and we port it? What range and step would you suggest? For example ±5° in 0.05° steps.

## 4. Automatic power calibration (SWR / power meter)

- Stew remembers that your 100 W **Solidus** on the RPi, about six years ago, calibrated its power automatically. He hears you're looking at bringing Solidus back.
- **Proposal:**
  - The user enters the desired **QRP** and **QRO** output (e.g. 5 W and 100 W).
  - When forward/reflected power data are available (SWR/power meter present), MSCC calibrates each band automatically. It keys into a dummy load, steps the drive to reach each target, and saves the drive per band.
  - That replaces the manual per-band power cal.
- **Questions for Ron:**
  - How did Solidus do it (loop, safety limits, time per band)?
  - Can any of that code be reused?

## 5. RX I/Q tab fixes

Your points (a)-(c) are going ahead as a WPF client brief (APPLY ALL label, START shows ACTIVE, APPLY confirmation / cancelled message). Your other suggestions from that note will be looked at in the same brief.

## UP 24 kHz -> Jump to image (Stew 10-13)

**Today:**
- UP 24 kHz on the RX IQ tab **retunes the whole radio**. The client adds 24 kHz to the dial and sends a normal frequency set (`MainViewModel.cs` `ComputeRxIqTuneFreqHz` / `ApplyRxIqTuneFrequency`, about lines 3753-3776, `SetFrequencyAsync`).
- ms-sdr moves the LO with it.
- The signal then sits on the other side of the LO and its image falls in the passband. But because the LO and dial have moved, the picture on the spectrum changes: the image moves or vanishes from where the operator was looking.

**Correct behaviour (Stew):**
- The **LO and the dial stay fixed**. Example: LO 14.012, TG carrier at 14.024 (dial 14.024, passband at LO + 12 kHz).
- **Only the receive/audio window moves** to the mirror frequency: 2 × LO − signal = 14.000.
- The image can then be heard and nulled with the amplitude slider (and later phase) while the spectrum stays put.
- The button should be a **JUMP TO IMAGE** toggle. Pressing it again (or STOP) returns the window to the signal.

**What it needs (checked 2026-10-13, Windows sources):**
- The client has **no existing way** to move the receive window without moving the LO:
  - RIT (0x89 / 0x8A) is added to the tune frequency in ms-sdr `freq_queue.c:29-30`, so it retunes the LO as well.
  - The recv demodulator NCO is fixed at +12 kHz (`SDRcore-recv/sources/sdrcore.c:64` `csstate.loFreq = 12000.0f`) with no command to change it.
- So this needs a **small recv change**: a command (new opcode, or a sub-value) that sets `csstate.loFreq` to −12000 (the mirror of +12000) and recomputes `phaseInc`, plus the sideband handling if needed. The client then toggles it.
- That touches recv on Windows, Ubuntu and the Pi, plus a pass-through in ms-sdr.
- **Question for Ron:** would you do the Pi recv side first (as with smoothing)? Is −12000 the right mirror, or does the audio sideband also need flipping?

Until then, cmd-076 (WPF) leaves UP 24 kHz as it is and only fixes the other RX IQ tab items.
