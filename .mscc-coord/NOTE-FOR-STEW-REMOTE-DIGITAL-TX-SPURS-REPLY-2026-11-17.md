# Reply to your Remote Digital TX spurs note (2026-11-17)

Stew, thanks for the note and the pictures. Ron ran the same test here today,
and we recorded the signal inside the system to see where the spurs come from.
Short version: we can't make either problem happen at Ron's end. The comb most
likely comes from the radio (yours is not a Black Pill, Ron's is), and the
random bursts from stray sound on your client PCs.

## What we tested

Ron's setup: WPF client on his Windows PC, servers on the Pi (same versions as
yours: mscc 1.0.96, trans 3.160), Remote Digital, 20 m, Black Pill radio.

We recorded at two places at the same time:

- the audio arriving at the Pi from the client (network port 9101)
- the transmit I/Q going from the Pi to the radio over USB (the last step
  before RF)

and checked both for gaps, clicks, steadiness and purity.

## What we found

1. First transmission right after starting Remote Digital (main window
   Digital, then Remote, then WSJT-X Tune for about 15 seconds):
   - No comb. Everything more than 100 Hz from the tone was 38 dB down for the
     whole transmission. Your comb is only 10-20 dB down, so it would have
     stood out.
   - Steady level, no clicks, no gaps in the USB stream during the transmission.
   - The transmit stream to the radio did reopen when Remote Digital started
     (one 151 ms pause, before the transmission), with no harm.

2. Earlier FT8 transmissions over Remote Digital: same result, clean. The audio
   from the client had every packet, no clicks, and anything more than 100 Hz
   from the tone was 52 dB down.

3. With WSJT-X idle, Ron's client sends near silence (peak 1-3), not the
   300-1700 you see.

So at Ron's end the whole path, client to radio, is clean, both on the first
transmission and later ones.

## Answers to your questions

1. Did anything in the remote mic path change between late October and
   November 10?
   On the Pi, the trans remote mic code did not change in that window. On
   11-10 the Windows trans was given the same remote mic code as the Pi. No
   client remote-audio sending code changed in that window. The other trans
   changes in that window (ALC, low-pass) are shared with local Digital, which
   you say is clean.

2. Should the client send remote mic audio while not transmitting, and is it
   gated?
   The client sends all the time; that's how it was built. Trans mutes it at
   every key-up (mic volume to 0) and unmutes it at key-down, so only audio
   that arrives while transmitting reaches the modulator. Anything else
   playing into your virtual cable during a transmission goes out with the
   tone. Your idle peaks of 300-1700, and the VB cable meter blinking with
   WSJT-X idle, fit the random bursts: something on that PC is sending sound
   into the cable. Ron's PC doesn't.

3. What makes a 1 kHz comb on the first transmission?
   We can't reproduce it. A comb with 1 kHz spacing means something goes
   wrong once every 1 ms, which is the USB audio packet rate. So it points at
   the stream between the servers and the radio, not at the audio itself.
   The stream reopen happened at Ron's end too, with no comb. Same servers,
   different radio: so the radio's side of that stream is the main suspect.

4. Can Ron see it, and which build was last good?
   No, Ron sees neither the comb nor the bursts, so we can't name a last good
   build. Bisecting wouldn't help until it happens here too.

## Next steps

For the comb: your radio is not a Black Pill, so it is the radio's USB audio
handling we would look at next (the same kind of timing question as the
12 kHz CW comb). Please tell us the radio and its firmware version from the
test. One check that would help: on the first transmission after starting
Remote Digital, does a TUN from MSCC show the comb too, or only WSJT-X Tune?
TUN makes its carrier inside trans, without the remote audio, so that tells us
whether the remote audio stream is needed to make it happen.

For the random bursts, it would help to find what is playing into the VB cable
on Shack and NEW-HP while WSJT-X is idle. In Windows Sound settings, check
which programs use the cable, and whether any notification or system sounds go
to it.
