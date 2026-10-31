# For Stew: LSB filter buttons send the wrong values (WPF client bug)

From Ron, 2026-10-31. **Please read first: this one affects everyday listening.**

## What Ron heard

On an AM broadcast station (WRMI 5850 kHz), listening in LSB and USB tuned exactly on the
carrier: LSB sounds hollow and higher pitched than USB. Pressing the LO CUT button in LSB
changes nothing.

**Why it matters beyond voice:** FreeDV (700D, RADE) puts signal down to a few hundred Hz.
Received in LSB, a 500 Hz low cut takes off the bottom of it, so it decodes worse or not at
all. Ron says FreeDV is still used on LSB on 40 m and below. Until the fix, LSB always has
a 500 Hz low cut, whatever the LO CUT button shows.

## What the server log shows (Pi sdrcore-recv)

    USB:  FIR=[75,5500] Hz     (lo cut 75 Hz, as set)
    LSB:  FIR=[500,5500] Hz    (lo cut 500 Hz, every time)

Ten presses of LO CUT in LSB all arrived as **HI CUT index 0** (5500 Hz), so nothing changed.

## Why

sdrcore-recv expects plain button indexes, the same in every mode:

- `CMD_SET_BW_LOCUT`: 0-4 = 500, 300, 200, 100, 75 Hz
- `CMD_SET_BW_HICUT`: 0-6 = 5500, 4000, 3000, 2700, 2400, 1400, 1000 Hz

The DSP (`setFilterOffsets`) puts the passband on the right side for LSB or USB by itself.

The client does not send the index. It turns the index into the RF filter edges in Hz
(`Filter.LowHz` / `Filter.HighHz`), and `OnActiveFilterPropertyChanged` turns those back
into an index with `LowCutHzToIndex` / `HighCutHzToIndex` (`UdpRadioService.cs`, about line
697). In USB the edges are positive and the round trip works. In LSB they are negative and
swapped, so the values do not match and fall to the default (index 0):

- Switching to LSB (`ApplyModeFilterProfile`, about line 4593): `LowHz` = -5500 is sent as
  the low cut; 5500 is not a low-cut value, so it goes out as index 0 = **500 Hz**.
- LO CUT in LSB (`OnLowCutIndexChanged`, about line 4983): sets `HighHz` = -75, sent as the
  high cut; -75 is not a high-cut value, so it goes out as index 0 = **5500 Hz**.
- HI CUT in LSB (`OnHighCutIndexChanged`, about line 5016): sets `LowHz` = -2400, sent as
  the low cut; 2400 is not a low-cut value, so index 0 = **500 Hz**. So HI CUT in LSB
  probably does not work either, and forces the low cut to 500 Hz.
- AM looks affected the same way: HI CUT sets `LowHz` = -hz, sent as low cut index 0 = 500
  Hz. That fits the Windows recv log seen earlier, AM `lastHR=[500,4000]`. Not tested here.

(Line numbers are in `MSCC.Wpf/ViewModels/MainViewModel.cs` unless noted.)

## Suggested fix

Send `LowCutIndex` and `HighCutIndex` to the server as they are, in every mode
(`CMD_SET_BW_LOCUT` / `CMD_SET_BW_HICUT`), from the button handlers and from
`ApplyModeFilterProfile`. Keep the Hz edges only for drawing the passband on the spectrum.
Do not send from `OnActiveFilterPropertyChanged` for LowHz / HighHz.

The Avalonia client may have the same code; worth a look.

## Check after the fix

In LSB on any signal, press LO CUT through its values. The recv log should show
`CMD_SET_BW_LOCUT: New filter: Low: 75` (and 100, 200, 300, 500), and switching LSB / USB
with the same settings should show the same `FIR=[...]` and sound the same.

No server change needed. Not tested here: client changes are yours.
