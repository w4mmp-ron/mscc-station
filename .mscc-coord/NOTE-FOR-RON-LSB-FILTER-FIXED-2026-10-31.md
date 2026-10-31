# For Ron: LSB filter buttons fixed in the WPF client (10.31.1)

From Stew, 2026-10-31. Reply to `NOTE-FOR-STEW-LSB-FILTER-BUTTONS-2026-10-31.md`. Thanks, your
diagnosis was exactly right.

## What was changed (client only, no server change)

- New service calls `SetLowCutIndexAsync` / `SetHighCutIndexAsync` send the LO CUT / HI CUT
  button index as is (`CMD_SET_BW_LOCUT` 0-4, `CMD_SET_BW_HICUT` 0-6).
- In **LSB, USB, DIG-U and AM** the button handlers, `ApplyModeFilterProfile`, the VFO
  restore and the filter-changed hook send the indexes. The Hz edges are only used to draw
  the passband (`OnActiveFilterPropertyChanged` no longer sends for LowHz / HighHz in those
  modes). CW and FM unchanged.
- **Also found:** ms-sdr restores the last cut indexes it received (`user_controls.ini`). The
  old client had left index 0 there, so a session started at 500/5500 Hz until the first mode
  change, while the client showed 75 Hz / 4.0 kHz. The client now pushes its LO/HI CUT once
  per session when the startup frequency arrives ("Filter cuts pushed at connect").

## Tested on Stew's MKII (Windows recv 3.150)

- LSB LO CUT 75 / 100 / 200 / 300 / 500 and HI CUT 2400 / 2700 / 3000 / 4000 / 5500 all
  arrive in recv; LSB and USB with the same buttons give the same `lastHR=[...]`.
- AM receive (with your AM lowpass + DC block): "quite good".
- LSB vs USB on an AM carrier are now close; the remaining difference is the signal
  (sideband fading, tuning offset, RX I/Q at 0), not the filter.

## Not done

- Avalonia client: probably the same code path. Not looked at yet.

Also in this push: TX I/Q phase on Windows (trans 3.150, ms-sdr 3.186, client PHASE slider),
see `mscc-ui/windows-work-tree/CLAUDE.md`. Stew's MKII: 40 m 0, 20 m +1, 15 m -4, 10 m -8
steps (your MKII 10 m was -6).
