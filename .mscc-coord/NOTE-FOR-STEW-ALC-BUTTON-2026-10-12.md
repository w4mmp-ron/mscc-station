# For Stew — TX ALC button: please remove it

From Ron, 2026-10-12. Client only. Your call on when.

## What Ron decided

Transmit ALC stays on, always. The off switch was put in for testing years ago and is not
meant for users.

## Why now

Ron's Pi was left with ALC off after a client session. With ALC off, trans does no limiting
at all on AM, LSB and USB, and the ALC meter stops. Ron lost time finding out why he had no
ALC action in WSJT-X.

## What Ron is changing on the Pi

In `sdrcore-trans`, the ALC on/off command (`CMD_SET_ALC_MULTIPLIER`, 0x23) will keep ALC on
for any value it receives. The source is edited; it is not built or tested yet.

`ms-sdr` is not changed. It still stores the button value in `user_controls.ini` and reports
it to the client.

## What Ron asks

Simply remove the ALC button from the client. In WPF 10.11.3 it is `MainWindow.xaml`
line 1198 (`Content="ALC"`, `ToggleAlcCommand`). With the button gone the client no longer
needs to send 0x23.

## Things to know

- Until the button is removed it does nothing on the Pi, and it can show "off" while ALC is
  on.
- This replaces the earlier open point about the client taking the TX ALC button state from
  ms-sdr. With no button there is no state to show.
- Windows and Ubuntu trans are not changed by Ron. They still switch ALC off when the client
  sends 0. Whether they should also ignore it is yours to decide.
- Nothing in ms-sdr or the WPF client sends "off" except this button, and ALC only acts on
  AM, LSB and USB, so no calibration step depends on it.
- Avalonia was not looked at.
