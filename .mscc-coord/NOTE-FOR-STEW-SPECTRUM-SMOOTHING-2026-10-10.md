# For Stew — Pi sdrcore-recv: spectrum smoothing (running average) and the LO point without a hole

From Ron, 2026-10-10.

## What Ron saw

With the client REFRESH at 1 the spectrum trace is nervous; at higher values it gets choppy.
Screen recording measured frame by frame: redraws per second = 15.6 / REFRESH (8 at 2, 5 at 3,
4 at 4, down to 1.5 at 10).

## Why

- REFRESH is client only: `MainViewModel.cs` (`SpectrumUpdated`, `_spectrumFrameCounter`) draws
  1 frame in N and drops the rest. Nothing is sent to the server, which keeps sending 15.6
  frames per second. The waterfall gets one row per drawn frame, so REFRESH is also the
  waterfall speed.
- The server smoothing was never adjustable in practice: no client sends
  `CMD_GET_SET_PANADAPTER_SMOOTHING`, so ms-sdr sends its stored `PANADAPTER_AVERAGE` (0) at
  start and recv always averaged 2 frames (0.13 s).

So the only way to calm the trace was to draw fewer pictures.

## What changed on the Pi

`rpi/SDRcore-recv-linux/sources/panadapter.c` only. The 4-frame history average is replaced:

- `PANADAPTER_AVERAGE` = 0: this frame + the previous one / 2. Same as before, and still the
  default, so nothing changes until the value is set.
- `PANADAPTER_AVERAGE` = 1 to 100: running average per display point,
  `avg += (new - avg) * alpha`, time constant = value x 0.025 s
  (`alpha = 1 - exp(-frame time / time constant)`, frame time from `G_Panadapter_Blocks`).
- After a history clear (band change, tuning pause, resolution change) the first real frame
  loads the average directly, so the trace does not ramp up from zero.

The value still arrives as before (`G_Smoothing` = opcode data + 2); ms-sdr is unchanged, it
reads the key from `user_controls.ini` with no range check and forwards it.

Ron runs REFRESH 1 with `PANADAPTER_AVERAGE=9` (0.225 s): full frame rate, calm trace.
20 (0.5 s) was too slow for him.

Known trade: the waterfall is drawn from the same frames, so it is smoothed too.

## Second change: no hole at the LO point

The DC blocker on the raw I/Q (`dsputils.c` `framesToComplex`, `DC_BLOCK_A` 0.98, the one in
cmd-064) removed the spur at VFO -12 kHz but left a hole about 1 kHz wide in the trace and a
dark stripe down the waterfall (measured in Ron's recording; 0.98 is -3 dB at 300 Hz).

Now, `rpi/SDRcore-recv-linux/sources/dsputils.c`:

- `DC_BLOCK_A` 0.98 -> 0.9999 (-3 dB at 1.5 Hz): removes the true DC only.
- New in `doPanadapter`, after the shuffle and before the display loop: the 16 FFT bins each
  side of the LO point (375 Hz, `PAN_DC_HALF_BINS`) are scaled down so that each bin's slow
  average (`PAN_DC_ALPHA` 0.01 per FFT, about 2 s) equals that of the 16 bins just outside
  (`PAN_DC_REF_BINS`, the quieter side). Bins are never scaled up. The low-frequency noise
  hump goes, the normal noise texture stays: no spike, no hole.
- A steady signal within 375 Hz of the LO point is levelled into the floor (it was hidden
  before as well). Display only; audio is 12 kHz away.

Built on the Pi 2026-10-10, Ron: "Super".

Follow-up the same day (in mscc 1.0.62): after a transmission the slice showed as a dip that
took many seconds to fade. The low-frequency thump on return to receive had driven the per-bin
averages far up. Now the averages do not learn for about 1 s after transmit
(`PAN_DC_HOLD_FFTS`) or during a tuning pause, one FFT counts as at most 4 x the current
average (`PAN_DC_MAX_STEP`), and the first second is a plain average (`PAN_DC_PRIME_FFTS`).
Built on the Pi, Ron: "much better". Take the `dsputils.c` from this commit, not the earlier one.

## For you

- If you have ported cmd-064 already: take this `dsputils.c` change with it (same file).
- Port the `panadapter.c` change to Windows and Ubuntu recv when convenient. With the default 0
  it behaves as it does today.
- Client, your call: a control in the S/W window that sends `CMD_GET_SET_PANADAPTER_SMOOTHING`
  (0 to 100) would make this adjustable without editing the ini. REFRESH could then stay at 1,
  or be changed so it no longer drops frames.

## Status

Built on the Pi 2026-10-10, works (Ron). Package: `installers/rpi/mscc_1.0.62_arm64.deb`
(1.0.61 removed); same 110 entries and modes as 1.0.60, only `sdrcore-recv` (both changes
and the follow-up) and the control version differ.
