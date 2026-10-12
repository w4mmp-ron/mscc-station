# For Stew — S/W window: REFRESH becomes SMOOTHING

From Ron, 2026-10-12. Client only, your call on how and when.

## What Ron would like

Ron is running WPF 10.11.3 against the Pi and it works well. One request: the REFRESH control
in the S/W window should set the server smoothing (`PANADAPTER_AVERAGE`) and be labelled
**SMOOTHING**.

Today the only way to change the smoothing is to edit `PANADAPTER_AVERAGE` in
`user_controls.ini` on the host and restart. Ron uses 9 with REFRESH 1.

## What the value does

Recv keeps a running average per display point (`panadapter.c`, the same file on Windows and
the Pi, compared 2026-10-12):

- 0 = this frame + the previous one / 2 (the default).
- 1 to 100 = time constant of value x 0.025 s (9 = 0.225 s, 20 = 0.5 s). Above 100 is
  clamped to 100.

Ron found 20 too slow, so a range of 0 to about 30 covers what is useful.

## What is already there

No new opcode and no server change, on Windows or the Pi:

- `CMD_GET_SET_PANADAPTER_SMOOTHING` = 0xDA (`usbavrcmd.h`). Data = the value as it is, 0 to
  100.
- ms-sdr `user_controls.c` `User_Controls_Process`: stores it as `PANADAPTER_AVERAGE`, saves
  it in `user_controls.ini`, and sends it to recv. It also sends it to recv at start, and it
  is in the settings report to the client (`PANADAPTER_AVERAGE=n;`).
- Recv applies it on the next frame, no restart.

## Client side, as it is in 10.11.3

- `MSCC.Core/Protocol/Opcodes.cs`: has no constant for 0xDA.
- `MSCC.Wpf/PanadapterControlsWindow.xaml` line 135: the REFRESH label and its list (1 to 10).
- `MSCC.Wpf/ViewModels/MainViewModel.cs` about line 2100 (`SpectrumUpdated`,
  `_spectrumFrameCounter`): draws 1 frame in N and drops the rest. Nothing is sent to the
  server.
- `MSCC.Wpf/SpectrumWaterfallSettings.cs`: `SpectrumRefresh`, default 4, ini key
  `SPECTRUM_REFRESH`.
- Nothing in the client reads `PANADAPTER_AVERAGE` from the settings report.

## One way to do it

1. Add the 0xDA constant and a send method.
2. Label SMOOTHING, range 0 to about 30. A change sends 0xDA with the value.
3. Show the value from the settings report at connect, so the host stays the one place it is
   kept (not the client ini).
4. Take out the frame dropping, or fix it at 1, so the spectrum and waterfall always run at
   the full 15.6 frames per second.

## Things to know

- The waterfall is drawn from the same frames, so it is smoothed too.
- If the frame dropping goes, the CPU / GPU saving it was written for goes with it. Ron sees
  no load problem at REFRESH 1 on his PC; yours to judge for slower machines.
- Avalonia has the same frame dropping (`MainViewModel.cs`, `SpectrumRefreshDivisor`).
