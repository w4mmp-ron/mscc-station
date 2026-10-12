# For Stew — RX IQ tab: four small things in the client

From Ron, 2026-10-12. Client only, your call on how and when. Nothing here stops the
calibration from working.

## Background

Ron went through a receive I/Q calibration on 20 m with WPF 10.11.3 against the Pi, and it
works: with UP 24 kHz ticked, the in-passband image of an S9 carrier is 50 dB down at the
saved value of 58 (36 dB down at 0), the slider acts at once, and APPLY writes the value to
`recv-iq.ini` for that band. Four things on the tab confused him on the way.

## 1. RESET ALL on the RX IQ tab: label

The button does not reset anything. It writes the current slider value to every band
(client sends 0x52, 0x55 RX, 0x8D; recv `IQ_Set_All_Band`). Its own confirmation box says
"THIS APPLIES I/Q SLIDER BAR VALUE TO ALL BANDS".

Ron would like it labelled **APPLY ALL**. `MSCC.Wpf/MainWindow.xaml` line 1679.

The RESET ALL button on the TX IQ tab (line 1859) is a real reset to the factory values, so
that one is right as it is.

## 2. START never shows ACTIVE

The style trigger sets the button content to "ACTIVE" while a session runs
(`MainWindow.xaml` line 1661), but the button also has `Content="START"` set directly on
it (line 1649), and in WPF a value set on the element wins over a style trigger. So the
button only changes colour and still reads START.

One way: take `Content="START"` off the button and put it in the style as a normal setter,
so the trigger can replace it.

## 3. APPLY confirmation defaults to No

"APPLY THE CURRENT I/Q VALUE?" has No as the default button (`MainViewModel.cs`
`ApplyRxIq`, about line 4081). Enter cancels, and nothing on the tab says so: the status
line stays at "ACTIVE ... then APPLY". Ron thought he had saved.

Options: show "APPLY cancelled" in the status line, or make Yes the default.

## 4. START should begin with UP 24 kHz on

With the box unticked the carrier is in the passband and its image is 25 kHz below, where
it only shows on the spectrum and never reaches the audio. The image that matters is the
one that lands in the passband, and that is what UP 24 kHz shows. The two do not null at the
same slider value: on Ron's rig, 20 m, the far image nulls at about 87 and the in-passband
image at 58. Nulling the far one made the in-passband image 10 dB worse (40 dB down
against 50).

Ron would like START to tick UP 24 kHz by default (`MainViewModel.cs` `StartRxIq` sets
`RxIqUp24k = false` today).

Two things that would go well with it, your call:

- LO FINE starting at twice the CW pitch (+1200 at 600 Hz), so the image sits on the pitch
  and the S meter reads it. Checked in CW only; other modes not worked out.
- Ending the session puts the dial back where it was before START. Today it is left with
  the 24 kHz and the LO FINE offset in it.

## Things to know

- "APPLY/ZERO succeeded" is shown when ms-sdr answers, and ms-sdr answers after 1 s whether
  or not recv wrote the file. Ron's save was checked in `recv-iq.ini` and the recv log.
- Avalonia was not looked at.
