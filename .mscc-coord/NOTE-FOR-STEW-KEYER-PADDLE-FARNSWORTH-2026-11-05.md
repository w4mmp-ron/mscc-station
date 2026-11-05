# For Stew: Farnsworth on the paddles, and a client checkbox for it

From Ron, 2026-11-05. A new keyer feature is working on my rig. The client part is yours, so
these are suggestions only. Change anything you like, or leave it for later. No rush.

## What it does

You key at whatever speed feels comfortable and hear your own sidetone straight away. The keyer
holds each finished letter in a buffer and puts it on the air with Farnsworth spacing: letters at
SPEED, gaps stretched to the FARNSWORTH speed. So you can send 18 wpm letters and the other
station hears 15 wpm overall, and is less likely to come back too fast.

If you get 20 letters ahead, the keyer sends STP on the sidetone (only you hear it), ignores the
paddles until the buffer has gone out, then sends RES and you carry on.

It is switched by a new opcode, **0x9D SET_PADDLE_FARNSWORTH**: 0 = off (the keyer works as it
always has), 1 = on. 0x9D was free in every layer (servers, client, pill, PSoC, PIC).

## Done on my side

- **PIC keyer:** new firmware in `worktrees/keyer-buffer` (`Release/Keyer-MCU-buffer-20261105.hex`).
  The timing now runs on a 1 ms timer, so the speed is exact (dot = 1200 / wpm ms). The old
  loop timing did not match the setting closely, so 18 may sound a little different from before.
  CQ memories, paddle abort, straight key and the EEPROM layout are unchanged.
- **Pi ms-sdr:** reads `CW_Paddle_Farnsworth=` from `cw.ini`, sends 0x9D to the radio at keyer
  start-up, and accepts 0x9D from the client over UDP (saves it, sends it on). It also reports
  0x9D (0 or 1) to the client at connect, next to 0x76. Today's client just logs "No handler for
  opcode 0x9D" and carries on.
- **Pill 3.13:** passes 0x9D to the PIC, the same way as 0x76.

## Suggested client change (your call)

A checkbox labelled **PADDLE** next to the FARNSWORTH control on the CW tab:

    FARNSWORTH  [ 15 ]  [x] PADDLE

Suggested tooltip:

> Farnsworth on the paddles: key at your own speed. The keyer sends your letters at SPEED with
> gaps set by FARNSWORTH. If you get 20 letters ahead you hear STP; the paddles pause until it
> catches up, then RES.

The FARNSWORTH tooltip says "for CQ memory play only … No effect on paddle" (`MainWindow.xaml`
around line 861). That is no longer true when PADDLE is ticked.

Sending it would work like the FARNSWORTH value (`SetKeyerMemTextWpmAsync` in
`UdpRadioService.cs`): `_transport.SendAsync(0x9D, (short)(on ? 1 : 0))`. The Pi ms-sdr already
handles that.

To show the saved state at connect, handle 0x9D where 0x76 is handled (`UdpRadioService.cs`,
`case Opcodes.SET_MEM_TEXT_WPM` around line 2194): the payload is 0 or 1. The Pi ms-sdr already
sends it.

## Not done, if you want it

- **Windows ms-sdr:** has no 0x9D yet. The Pi changes are small, in `ms-sdr-linux/source`:
  `usbavrcmd.h` (the define), `extern.h` (`cw_record.paddle_farns`), `main-controller.c` (cw.ini
  read / write, `initialize_keyer`, the UDP case, and a rejected 0x9D is only logged, so an
  older radio does not stop the comms).
- **PSoC boards:** pass 0x9D to the PIC like 0x76 in `Configure_CW`. Without that, the feature
  works only with the Pi and the pill.
