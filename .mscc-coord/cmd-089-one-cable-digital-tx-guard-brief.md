# cmd-089: one-cable digital transmit guard (WPF client, Shack)

Date: 2026-11-19. Host: **Shack**. Follow repo-root `CLAUDE.md` shared rules. WPF client only.
Do not touch `rpi/`, `Solidus/`, `installers/rpi/`, or Avalonia. Do not commit until Stew says "finished".

## Why

In remote **Digital** mode the client plays receive audio into a virtual cable for WSJT-X and records
WSJT-X's transmit audio from a virtual cable. When the user sets up **one** VB-Cable for both directions
(MSCC play = `CABLE Input`, MSCC mic = `CABLE Output`, WSJT-X the same), the receive audio loops straight
back into the transmit audio. Seen on NEW-HP on 2026-11-19: during plain receive the remote mic peak was
about 900 to 2000 instead of 0, and at key-down a phone-like wideband burst went out for about 338 ms
before the WSJT-X tone took over. Two cables (play on Cable A) fixed it, but one cable is a normal setup
and must work.

Measured on the SA (Stew, 2026-11-19): key-down to signal is about 473 ms on WSJT-X TUNE and about 709 ms
on FT8. The burst is about 338 ms.

## What to change

Only while the remote audio path is **Digital** (R-Digital, audio device 3). Phones (voice) remote is
unchanged.

1. **No receive audio into the cable while transmitting.** While the radio is keyed (PTT on from any
   source: WSJT-X CAT key, the PTT/MOX button, or TUN), the receive playback sends silence instead of
   the received audio. It goes back to normal at key-up. Do this at the playback side
   (`RemotePhonePlayer` / `NetworkWaveProvider` read path) with a simple flag. Keep the jitter buffer
   running so there is no restart or gap in receive at key-up.
2. **Dump the mic audio for 400 ms after key-down.** For 400 ms after PTT goes on, `RemoteMicSender`
   sends silence (zeros) in place of the captured audio. Keep sending packets at the normal rate with
   normal sequence numbers; only the sample values are zeroed. After 400 ms, normal audio.
   The mic is silent in Digital while not transmitting, so the Pi's transmit buffer holds only silence at key-down.
   When that silence ends, including the end of the 400 ms dump, the mic ramps from 0 to full over 5 ms (240 samples at 48 kHz, raised cosine) instead of stepping.
3. Feed both from the client's existing keyed state in `MainViewModel` (the same place that logs
   `PttOn set:`), plus TUN. One `RemoteAfEngine` method such as `SetTransmitting(bool)` is enough.
4. Log one line per key-down in Digital: `Remote AF: TX guard — RX playback muted, mic dump 400 ms`, and one
   at key-up: `Remote AF: TX guard off`.

Keep it small. No new settings, no UI.

## Version and docs

- ClientVersion 11.19.0 to **11.19.1** (Ron's R11-19-0 already took 11.19.0; find the current value with a search, do not assume).
- CHANGELOG and Versions Addendum: "Remote digital: a one-cable VB-Cable setup no longer sends a short
  burst of received audio at key-down. While keyed, MSCC stops playing receive audio into the cable, and
  it drops the first 400 ms of transmit audio after key-down."
- windows-work-tree `CLAUDE.md`: one line under remote audio.
- Drop-in build to `C:\mscc-net9\MSCC.Wpf.exe` as usual. No installer unless Stew asks.

## Test (Stew, on the SA)

- [ ] NEW-HP set back to **one** cable: MSCC play `CABLE Input`, mic `CABLE Output`; WSJT-X input
      `CABLE Output`, output `CABLE Input`
- [ ] WSJT-X TUNE: no wideband burst at key-down; carrier clean
- [ ] FT8 transmission: no burst; tone clean for the whole transmission
- [ ] Receive into WSJT-X decodes normally between transmissions (no gap after key-up)
- [ ] Two-cable setup (Cable A for play) still clean
- [ ] Phones (voice) remote unchanged: first syllable not clipped
- [ ] Log shows the TX guard lines at key-down and key-up

## Note for the Remote guide (Phones, no code change)

Stew, 2026-11-19: on a laptop, the built-in mic hears the built-in speakers. In remote Phones the receive audio from the speakers then goes out at the start of each transmission (about a second). Muting the speakers, or using a headset, fixes it. Add this as a tip in the Windows Remote guide. cmd-089d (Phones guard) was dropped; Phones is unchanged.
