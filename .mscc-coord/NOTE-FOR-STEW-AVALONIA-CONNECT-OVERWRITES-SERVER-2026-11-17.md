# Avalonia overwrites the server's settings at connect (2026-11-17)

Stew, Ron finds that after running the Avalonia client, settings are wrong when
he goes back to WPF. We compared how the two clients handle buttons and sliders
at connect. This is what we found. Nothing has been changed.

## Where the settings live

- Saved by the server (ms-sdr: user_controls.ini, cw.ini, power.ini,
  startup.ini): power levels, volumes, mic gains, AGC, CMP, NB, NR, AN, MON,
  AMP, the CW settings, band, frequency and mode. Both clients share these.
- Client only: each client has its own files (WPF MSCC_LastUsed.ini and the
  others in %LOCALAPPDATA%\MSCC-NET9; Avalonia mscc-avalonia.ini,
  mscc-lastused.ini and the others). They never touch each other, so they are
  not the problem.

## What happens at connect

Both clients send "GUI ready" (0xFE). ms-sdr answers with all its saved values
(User_Controls_Send_To_Gui, the CW values, band and frequency), and both
clients show them the same way.

After that:

- WPF only sends pan resolution, spectrum smoothing, the audio route and NB
  (MainViewModel.cs, the Start routine around line 6359).
- Avalonia sends about 35 of its own saved values straight away, plus its last
  frequency and mode (Avalonia MainViewModel.cs ConnectAsync around line 1869:
  PushActiveVfoToRadioAsync(force: true) and PushStickyOperateToRadioAsync,
  line 8046). That includes filters, step, phones and digital volume and mic
  gain, tune / CW / SSB / AM / FM power, CW speed, keyer mode, spacing, paddle,
  weight, pitch, hold and QSK, AGC and AGC fast release, CMP, NB, NR, AN, MON
  and AMP.

## Why that breaks things

1. Avalonia saves its file only while it runs. After Ron has used WPF, the
   Avalonia file holds old values.
2. ms-sdr handles commands in order. It first sends the client its current
   values (Ron's WPF settings), then applies Avalonia's push. So the old values
   become the server's saved settings.
3. Avalonia's screen shows the values the server reported, not what it just
   pushed (the report handlers suppress sending), so the screen and the server
   disagree.
4. When Ron starts WPF again, it reads the old values from the server. The
   frequency and mode also jump back to where Avalonia was last.

## Smaller differences

- NB limits differ inside Avalonia: pulse 10-510 in the sticky push but 50-510
  elsewhere, threshold 1-1009 vs 1-1000.
- Avalonia does not listen for the transverter report (WPF does).

## Suggested fix (Avalonia only)

Make Avalonia's connect work like WPF: take the server's values, and only send
what WPF sends (pan resolution, smoothing, audio route, NB). That means
removing the sticky push and the forced VFO push at connect.

Before removing anything, check which of those values the server never reports
(step, CW pitch, QSK and FM power look like candidates) and do what WPF does for
them, so nothing is left unset.

How to see it: set a power level or the CW speed in WPF, close WPF, start
Avalonia and connect, then compare its log ("reported" lines) with
~/.local/mscc/user_controls.ini on the host afterwards.
