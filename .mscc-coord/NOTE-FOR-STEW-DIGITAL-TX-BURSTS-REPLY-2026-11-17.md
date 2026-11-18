# Reply to Stew: Windows client digital TX bursts (2026-11-17)

Stew, thanks for the report
(`REPORT-FOR-RON-WINDOWS-CLIENT-DIGITAL-TX-BURSTS-2026-11-17.md`). The sequence check settles it:
the packets are fine, so the 10 ms chunks are mixed up on the client before they are sent.

## First, please check the CAT PTT poll (10 ms)

The bursts are made of 10 ms chunks, and since 11-13 the WPF client has a new 10 ms poll.
R11-13-4 (`KenwoodCatPort.cs` / `NativeComPort.cs`) added the `PIN=` setting in
`comm-port.ini`, so WSJT-X can key PTT on CTS or DCD. With `PIN=1` or `PIN=2` the CAT read
thread wakes every 10 ms to check the line. With `PIN=0` (the default) it waits 200 ms as before.

10 ms is also the capture buffer and the MSA1 packet size, so this may be chance, and the poll
does not touch the audio, and Ron's Win10 client runs it with no bursts (see question 1 below).
But it is new, Windows only, and came after your clean tests, so it is worth ruling out on Win11:

1. **Is `PIN=1` or `PIN=2` set** in `comm-port.ini` on the Win11 clients? If not, the poll is not
   running and it is not the cause.
2. **If it is set, test with `PIN=0`** (CAT PTT in WSJT-X). If the bursts stop, that is the cause.
3. **Or change the poll time.** It is fixed in `KenwoodCatPort.cs`:
   `port.Open(name, baud, _pin != 0 ? 10u : 200u)`. Try 25 ms, or 7 ms (not a multiple of 10).
   If the bursts change with it, the poll is the cause. 25 ms is still fast enough for PTT.

## Your questions

1. **Ron's Win10 client.** Ron has no SA. This morning we recorded his Win10 WPF client into the
   Pi (WSJT-X into Remote Digital): the MSA1 mic packets on the Pi and the USB TX I/Q to the radio.
   We found no bursts in either. That fits your finding that only Win11 clients show it.
   Two differences from your setup: Ron's client runs with `PIN=1` (so the 10 ms poll above was
   on during that test), and his digital audio device is a USB sound card, not a VB-Cable. So his
   result does not yet say whether VB-Cable is fine on Win10.

2. **WASAPI capture in the WPF client.** Your call. Trying it as an option, with MME kept as it is,
   looks like a sensible next test.

3. **Changes in the last month.** The WPF mic capture and send code (`RemoteMicSender.cs`, the ring,
   `SendPacket`) has not changed. The only change in the `RemoteAudio` folder is the CAT PTT
   poll above.

## One more clue

You found that moving the WSJT-X drive slider or the MSCC mic gain sets the bursts off. That
points to the capture side being held up for a moment by work on the client (UI or CPU), so the
virtual cable repeats or drops a 10 ms buffer to catch up. That fits your Win11 audio engine
idea, and a larger capture buffer (WASAPI or MME) may hide it.

## The smaller items

Noted: the startup comb in Remote Digital (Phones EQ applied while in Digital at startup) and the
small pulse at key-down and key-up. Nothing to add from here.
