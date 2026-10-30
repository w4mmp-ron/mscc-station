# For Ron: TX edge fixes (firmware 3.242, Windows trans/recv 3.148) and the open SSB splatter

From Stew, 2026-10-29. This covers the TUN press/release splatter work on the Proficio MKII PTT
with the Win11 client and Windows servers, plus a new SSB voice problem found while testing.
Bench: Signal Hound SA (14.074, RBW 100 Hz, 100 kHz span), scope (CH1 RF out, CH2 TX LED),
Rev 7 schematic. Follows on from your `NOTE-FOR-STEW-WINDOWS-SERVERS-CATCHUP-2026-10-28.md`.

## Summary

| Problem | Cause | Fix | Status |
|---|---|---|---|
| TUN press: hump of about +/-2.7 kHz | trans starts the TUNE carrier at full level in one step | 5 ms raised-cosine ramp in trans | Windows trans 3.148, **confirmed** |
| TUN release: broadband splatter across the span | TX_HI drops while the carrier is still at full level in the DAC (hard cut after the filter) | Firmware DAC soft mute before PA off (non-CW path) | PSoC MKII PTT 3.242, **confirmed** |
| Audio flips from Digital to speakers after TUN | recv reopened the speaker on TX off in CW mode, ignoring the Digital setting | recv reopens Digital when selected | Windows recv 3.148, **not yet confirmed** |
| SSB voice: broadband streaks, worst early in an over | Most likely trans `doALC()` gain chatter | Proposed, not written | **Open, needs your view** |
| LSB opposite-sideband image only ~35 dB down (USB ~60) | No TX I/Q phase correction on Windows | Port IQ_PHASE from the Pi | **Open** |

## 1. Firmware: Proficio MKII PTT 3.240 → 3.241 → 3.242

**Why.** TX_LED is driven from TX_HI through U22 (ULN2003, sheet 8), so the LED shows TX_HI.
On TUN release the scope showed the RF cut off at the instant the LED went off. TX_HI (the
firmware's `CONTROL_RX` bit, negative logic) switches PA, bias and T/R: bias off in ~0.1 ms,
T/R in < 0.4 ms. The carrier was still at full level, so the SA showed splatter with no
filter edges across the whole 100 kHz span.

**3.241** (non-CW path, kept in 3.242):
- `cw.c`: `void TX_DAC_Mute(uint8 mute)` wraps `cw_dac_mute()` and shares `dac_muted` with
  CW. Main loop only (I2C).
- `tx.c`: `state` made file-static; new `TX_Reset()`.
  - Non-QSK key: DAC mute, AMP on, key delay (>= 20 ms), PA on (TX_HI), unmute.
  - Release: DAC mute, `TX_DAC_RAMP_MS` (18 ms) ramp, PA off, `AMP_RELEASE_MS` (10 ms),
    AMP off, unmute.
  - Re-key during the ramp: unmute, back to TX.
  - QSK path unchanged.
- `main.c`: entering-CW block calls `TX_Reset()`.
- `basic-plus.h`: `TX_DAC_RAMP_MS 18u`, prototypes.

3.241 alone did not help on TUN. The non-CW mode-change block in `main.c` (beater case 1)
hard-dropped the PA on **any** mode change, and the client sends the mode change before
TUN off, so the hard drop won the race.

**3.242:**
- `main.c` case 1: a change between two non-CW modes (previous mode not 'C' and not 0) only
  updates `previous_host_mode`. The hard drop and `CW_Key_Delay_Stop()` now run only when
  leaving CW or on the first pass after power-up.
- `main.c`, entering CW: if `!TX_Idle()`, set `TX_Request = 0` and keep calling `TX_Main()`
  until idle (ramp, PA off, AMP off, unmute), with a `MODE_DRAIN_MAX_MS` (60 ms) fallback to
  the old hard drop. `Manage_Paddles_Port()` waits until CW entry is done. This keeps your
  3.233 CW + TUN stuck-TX safety fix, but with a clean edge.
- `tx.c`: `uint8 TX_Idle(void)` (state == 0).
- `basic-plus.h`: version 242, `MODE_DRAIN_MAX_MS 60u`, prototype.
- Build on Shack: 0 errors, flash 85.9 %.

**Results on 3.242:**
- ✅ TUN release on the SA: splatter gone.
- ✅ Scope: RF ends ~20 ms before the TX LED goes off, as designed.
- Still to run: SSB PTT on/off edges; quick PTT tap and re-key during the 18 ms ramp (no
  stuck TX, DAC unmuted after); CW non-QSK and QSK unchanged; TUN from CW mode; SSB ↔ CW
  idle and keyed; external amp gap (AMP on → RF, RF gone before AMP off).

**Project settings changed on Shack (not code):**
- Bootloadable → Dependencies now
  `..\..\Proficio-bootloader\bootloader.cydsn\DP8051\DP8051_Keil_951\Release\bootloader.{hex,elf}`
  (the old `..\bootloader\...` path does not exist in this repo).
- Post Build (Debug) user command cleared (it pointed at your PC).
- The other radio lines probably have the same two paths.

**Side items for you:**
- `si5351a.c` retunes the LO (adds RIT) when `TX_Request` drops. With the 18 ms ramp the PA
  is still on at that moment. Fine with RIT at 0; needs a fix if RIT is used in TX.
- Leading edge: TX_HI still comes on ~35 ms before RF (trans + USB audio latency). The PA
  starts into silence, so it looks harmless.
- QSK amps still get the old hard edge from the firmware. trans would need its own ramp at TX
  off to cover them. Do you want that?
- Not ported yet to the other 7 lines. Plan: Ultimus MKII PTT first (straight copy), then
  the ATU lines. OK with you?

## 2. Windows servers 3.148 (built on NEW-HP, VS 2022)

Source: `mscc-ui/windows-work-tree/SDRcore-{trans,recv}/sources`.

- **trans 3.148:** 5 ms raised-cosine ramp on entry to TUNE in `dsputils.c`
  `tune_modulate`, armed in `udp_thread.c` `CMD_SET_MAIN_MODE` case 4. The TUN-press hump
  is gone on the SA.
- **recv 3.148:** `udp_thread.c` `CMD_SET_TX_ON`, TX off: when `G_mode == 'C'` it used to
  reopen the operator speaker regardless of `G_recv_audio_mode`. On TUN release the client
  sends the mode change first, so recv was already in CW and the Digital output was lost.
  Now it reopens the digital device when DIGITAL / REMOTE_DIGITAL is selected (log line
  `CW TX off: reopen DIGITAL`).
- Pi and Ubuntu: the same two changes are still to do. Please check whether the Pi recv has
  the same branch.
- Your list of Pi-only server changes (PTT `r == 1`, IQ_PHASE, 0xC8 cal, etc.) is **not**
  ported yet.

## 3. Open: SSB voice splatter (USB and LSB, FW 3.242, Windows trans)

**Seen:** in USB and LSB voice, broadband horizontal streaks across the whole span, worst in
the first half of an over, then fewer. Max-hold skirt about -60 dBc at +/-1-2 kHz falling to
about -100 dBm at +/-50 kHz. Mic gain makes no difference. TUN and CW are clean.

**Most likely cause:** `doALC()` in `dsputils.c`, called from `fastconv()` (`sdrcoretx.c`)
**after** the SSB filter.
- It runs only for AM / LSB / USB, which fits TUN and CW being clean.
- The envelope is `fabs(I)` only, not |I + jQ|. After the filter the signal sits at a
  9-15 kHz IF, so |I| goes to zero every ~35-40 µs.
- Envelope decay (`envramp`) and gain attack (`negramp`) are both 1 ms to 40 dB, about
  0.4 dB per sample at 96 kHz. Near the target level the gain chatters at about twice the
  IF (20-30 kHz). That amplitude-modulates the filtered signal and spreads it across the
  span.
- Release is 3000 ms, so after the first loud peaks the gain stays low and the chatter
  mostly stops. That fits "worse in the first half, then settles".
- `doMicProc` normalizes the mic level ahead of this, which is why mic gain has no effect.

**Confirming test (not run yet):** lower PWR until the ALC never acts. If the streaks go,
this is the cause.

**Proposed fix (trans, shared by all platforms):** envelope from `sqrtf(I*I + Q*Q)` (smooth
for an analytic SSB signal), peak hold with ~50 ms decay, gain smoothed toward
`target / envelope` with a few-ms attack and the existing slow release. The gain then moves
only at audio-envelope rates and the splatter stays near the signal. Since you said ALC is
always on, we would like your OK before changing its behavior.

## 4. Open: LSB image ~35 dB down (USB ~60 dB)

- Wanted 14.071-14.074, image at 14.0508-14.0532. LO leakage at 14.0625 (trans IF is 12 kHz,
  so hardware LO = dial - 12 kHz).
- Windows trans applies only a per-band gain balance at the end (`iMult` / `qMult` from
  `IQ_OFFSET`). No phase term (item 2 on your 2026-10-28 list). The same correction is used
  for USB and LSB after the modulator, so it is not a sideband sign bug.
- USB sits at IF +12.3..+14.7 kHz and LSB at +9.3..+11.7 kHz. A frequency-dependent phase
  error in the analog I/Q path would let a gain-only balance look good in one part of the IF
  and poor 3 kHz lower.
- **Test to separate the two:** single tones at 500 and 2500 Hz in USB (IF 12.5 / 14.5 kHz)
  and LSB (IF 11.5 / 9.5 kHz), image level against IF offset. One smooth curve → analog
  path, fix is the IQ_PHASE port (maybe with a frequency term). LSB worse at the same IF →
  software bug.

## Questions for Ron

1. OK to change `doALC()` as in section 3, or do you know a reason it uses `fabs(I)`?
2. Trans ramp at TX off for QSK amps: wanted?
3. Port order for 3.241/3.242 to the other lines: Ultimus MKII PTT first, then?
4. RIT retune during the 18 ms ramp (`si5351a.c`): fix now or leave?
5. Does the Pi trans show the same LSB/USB image difference with IQ_PHASE set?

## What is in this push

- **Is in this push:** Windows trans/recv 3.148 source (`mscc-ui/windows-work-tree/SDRcore-{trans,recv}`) and
  this note.
- **Not in this push:** the 3.241/3.242 firmware source. It was edited and built on Shack; the copy in this checkout
  (`radio-psoc-firmware/Proficio-MKII-PTT`) is still 3.240 (files dated 2026-10-22). It
  needs to be committed from Shack, with the `.cyacd/.hex` copied to
  `release/Proficio-MKII-PTT/` with the date.
