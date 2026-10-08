# For Stew — spectrum dB labels: the scale is squeezed 2:1

From Ron, 2026-10-07. Pi host (mscc 1.0.56), WPF client, dB labels on. Found by reading the
code. Ron's rough test of 2026-10-08 confirms the squeeze but not the exact factor (see
"Ron's test"); a clean step test is asked of you below.

## What Ron sees

WWV on 10 MHz, CW mode. The S meter reads about S9, which Ron believes (S9 = -73 dBm). The
peak on the spectrum, read off the dB labels, is nowhere near -73: far too low.

What Ron wants: the peak of a signal much higher on the scale, WITHOUT raising the
apparent noise floor.

## Cause

The two readings use different log laws.

- S meter, `sdrcore.c` (recv): `peakRxSignalDbm = 20 * log10(peakmag) - 20`. True dB.
- Spectrum, `dsputils.c` `doPanadapter` (recv): `mag = 10 * log10(mag)`, where `mag` is the
  FFT magnitude (a voltage, `sqrt(re² + im²)`), then `+ 40`, then `* 150` to make Y.
  10 * log10 of a voltage is half of true dB.
- Client, `MSCC.Core/Services/UdpRadioService.cs` `RawYToDb`: `relDb = Y / 150 - 40`. It
  undoes the packing exactly, so it hands the half-dB number to the display, which adds one
  fixed offset (dB CAL, centre -91.3, trim ±20) and labels the result as dB.

So a real 20 dB change moves the trace 10 dB. One offset can make the labels right at one
level only; everything above that level reads too low, everything below it too high.
Example: if the noise floor is labelled correctly at -120, an S9 signal 47 dB above it is
drawn about 23 dB above it, at about -96.

All three servers have the same line (Pi `rpi/SDRcore-recv-linux`, Ubuntu
`linux/SDRcore-recv-linux`, Windows `mscc-ui/windows-work-tree/SDRcore-recv`, all
`sources/dsputils.c`).

## Fix — client only, no server change

`(10 * log10(mag) + 40) * 150` is the same number as `(20 * log10(mag) + 80) * 75`. The data
on the wire is already fine; only the client reads it with the wrong scale. In `RawYToDb`:

```
const float scale = 75f;   // was 150
const float bias  = 80f;   // was 40
```

`relDb` then comes out as true dB (exactly twice the old value). It works unchanged with the
Pi, Windows and Ubuntu servers, old and new. Nothing to deploy on the server side; the
comments in the three `dsputils.c` files that say "10*log10 ... bias 40" could be updated to
match, no code change.

Things that move with it:

1. **Clamp in `RawYToDb`:** Y goes up to 16000, so `relDb` now reaches about +133. The upper
   clamp (`> 80f`) has to go up (140 or so). The lower one (-200) is fine: Y = 0 gives -80.
2. **dB CAL centre (`SpectrumDbCalCenter` -91.3, WPF `SpectrumColorSettings.cs`; Avalonia
   `DbCalCenterAbsolute`, `SpectrumDisplaySettings.cs`):** must be found again by
   measurement, it does not carry over. To keep the noise floor where it is drawn today the
   new offset is `old offset - old relDb of the noise`; for a floor now labelled -120 at the
   default offset that works out near -63, outside today's range of -111.3 to -71.3. Better:
   set it with a known level (signal generator at -73 dBm = S9) and check that the floor and
   a second level 20 or 40 dB away also read right. With the right law, one offset now fits
   all levels.
3. **Saved settings:** `SPECTRUM_DB_OFFSET`, `_HF`, `_LF` in `MSCC_Client.ini` (Avalonia
   `DB_CAL_REL`) hold values made for the old law. The load code already resets an
   out-of-range offset to the centre; old in-range values need the same treatment once.
4. **Grid window (GRID MAX / MIN, defaults -20 / -125):** signals now reach twice as far
   above the floor, so check that strong signals still fit under GRID MAX.
5. **Waterfall and peak marker:** same data, so the waterfall contrast doubles (colour
   settings may want a touch) and the peak marker dB reads true.
6. **`sMeter` fill in `FlushPanFrame`** (`(peak + 140) / 10`): fed from the same `relDb`;
   marked unused for the face S meter, check nothing else depends on it.

Avalonia shares `MSCC.Core`, so one change to `RawYToDb` covers both clients; the centre
constant is separate in each.

## Ron's test, 2026-10-08 (today's client, before any change)

Multus SDR SMSG signal generator into the rig, 20 m, CW, 800 points, window wide.

| Generator setting | S meter | Spectrum peak | Noise floor |
|---|---|---|---|
| S9 | S9 | about -87 (between -85 and -89) | |
| S1 | S3 | about -100 | almost -120 |
| off | S2 | | |

The squeeze is proven: the signal dropped 36 dB by the S meter (48 dB by the generator's
settings) and the trace moved 13 dB.

The exact 2:1 is NOT proven: 13 dB on screen is 26 dB by the code, less than either ruler
says. The S meter reads S2 with the generator off, so at S3 it is one S-unit above its own
noise and cannot be trusted there; the generator has only these two levels and may leak at
S1. The server and client drawing path was checked end to end (max over bins, Y packing,
frame averaging, `RawYToDb`, offset, trace and label mapping): all linear, nothing else
bends the scale. So either the rulers are off or there is a second cause not found yet.

## To do: a clean step test (you have the gear)

Two levels that are both well above the noise, a known step apart: for example -73 dBm and
-93 dBm through a step attenuator. Read the spectrum peak at each.

- Before the change: expect the peak to move 10 dB for a 20 dB step. If it moves less,
  there is a second cause; please say so before changing anything.
- After the change: the peak moves 20 dB and the noise floor stays put.

## Related, already with you

- Client draws one data point per pixel (`NOTE-FOR-STEW-SPECTRUM-CARRIER-2026-10-05.md`):
  until that is fixed a carrier's peak reads low and flickers, so do the level check with
  the window wide or at 800 points.
- cmd-064 (full FFT, DC blocker) shifted the spectrum level a little; dB CAL was not redone
  after it. Redoing the centre here covers that too.
