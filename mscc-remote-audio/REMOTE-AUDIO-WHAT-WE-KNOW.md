# Remote audio: what we know

Ron, 2026-11-05. Stew, this is a short summary of where remote audio stands, so we're all
starting from the same place. If anything here is wrong, tell me and I'll fix it.

## First, what "remote" means

There are always two computers in a remote setup:

- The **host** is the one plugged into the radio. The servers run on it.
- The **client** is the one you're sitting at, with the MSCC window, your headset and WSJT-X.

If both are the same computer, that's local. If they're two computers talking over the
network, that's remote.

Saying "Windows to Linux" doesn't tell us much, because either one could be the host. So when
you describe a problem, please tell me:

- which computer has the radio (the host),
- which computer you're sitting at, and which client you're running (WPF or Avalonia),
- whether you're in Remote Phones or Remote Digital,
- what audio setup the client computer uses (VB-Audio, two USB adapters, VirtualA/VirtualB, or
  just a headset).

Most of our past mix-ups came from not knowing which way round things were.

## Remote audio already works

The servers handle remote audio the same way on the Pi, on Ubuntu and on Windows. It's the
same code everywhere. It has worked on the air:

- WPF on Windows, with an Ubuntu host: phones, WSJT-X, CAT and transmit.
- Avalonia on Ubuntu, with a Windows host: the same.
- WPF on Windows, with the Pi as host: works (Ron, today).

So there's no need to redesign anything. When something goes wrong, it's almost always a
setting, an audio level, or a small client issue.

## The two remote modes

You choose them in the client's Remote Audio window:

- **Remote Phones:** you talk into the client's mic and hear the radio in the client's
  headphones.
- **Remote Digital:** WSJT-X on the client computer hears the radio and sends its audio back
  for transmit.

Remote always starts off when MSCC starts, and pressing Phones or Digital in the main window
switches it off.

## Things we learned the hard way

- **Only one client at a time.** The radio accepts one client. If a second one tries, it's
  refused until the first one stops, or about 45 seconds after it drops off. On a Windows host,
  keep the local MSCC window closed, or it grabs the radio first.
- **Don't run local servers on the client computer** while you're working remote. They fight
  over the COM port and the audio devices.
- **WSJT-X runs on the client computer** and uses that computer's audio devices. For CAT, point
  it at the COM port set in the client's SETTINGS. The client pretends to be a Kenwood TS-2000.
- **No RF? Check the mic gain first.** A mic gain of zero sends silence. That was the whole
  cause of the long "no RF" hunt in September.
- **Transmit level in Remote Digital** follows the Digital mic gain in the main window, not the
  slider in the Remote window. VB-Audio passes the level through unchanged, so you may need
  more mic gain and more WSJT-X output than I use. My two USB adapters add some gain of their
  own.
- **Calibration stays on the host.** Don't copy calibration files from one computer to another.

## Audio setups: three different ones in use

- **Stew, Windows client:** VB-Audio. This works for you. Could you add exactly how you have it
  set up (one cable or two, and which way each one goes)? Then it's written down.
- **Ron, Windows client:** two USB audio adapters wired back to back.
- **Linux client (Avalonia on Ubuntu or the Pi):** two PipeWire / PulseAudio "null sinks", which
  do what VB-Audio does on Windows. Proposed names (not made yet):

  | Device | Internal name | Name shown in menus | Who plays into it | Who listens |
  |---|---|---|---|---|
  | Receive | `mscc_remote_rx` | MSCC Remote RX | the client | WSJT-X input = "Monitor of MSCC Remote RX" |
  | Transmit | `mscc_remote_tx` | MSCC Remote TX | WSJT-X output | client input = "Monitor of MSCC Remote TX" |

  RX and TX are from the radio's point of view. Each null sink is a fake speaker; what's played
  into it comes out of its "Monitor of ..." fake microphone. To make them by hand (48 kHz, like
  VirtualA and VirtualB; gone after a reboot unless added to the startup config):

  ```
  pactl load-module module-null-sink sink_name=mscc_remote_rx rate=48000 sink_properties='device.description="MSCC Remote RX"'
  pactl load-module module-null-sink sink_name=mscc_remote_tx rate=48000 sink_properties='device.description="MSCC Remote TX"'
  ```

A note on VirtualA and VirtualB, the virtual devices the Linux installer makes: they belong to
the **servers**. They let the servers talk to WSJT-X when it runs on the same computer as the
radio (local Digital). They are not the remote client's devices.

In remote mode the servers on the host send and receive audio over the network with the client,
and the client (with its Remote Audio window open) connects that audio to devices on its own
computer:

```
Remote Phones:   servers  <-network->  client  <->  real speaker and microphone

Remote Digital:  servers  <-network->  client  <->  virtual sound device (VB-Audio or similar)  <->  WSJT-X
```

For Remote Digital, the virtual sound device works in both directions:
- **Receive:** the client plays the radio's audio into the virtual device's speaker side, and
  WSJT-X takes its RX audio from there.
- **Transmit:** WSJT-X sends its TX audio into the virtual device's microphone side, and the
  client picks it up from there and sends it to the servers.

What's right for one of these is wrong for the others, so settings can't be carried across.

## What's still open

- In Avalonia, the Remote Audio device choices don't stay saved: they go back to one shared
  device, and the Phones choice isn't kept either. Remote Phones and Remote Digital each need
  their own devices, so the problem is the choices not being saved. The shared device might
  also be the noise you saw on the spectrum, but that's only a guess.

## More detail, if needed

In `mscc-remote-audio/`:
- `REMOTE-AUDIO-PUNCHLIST.md`: the full history.
- `REMOTE-WSJTX-CHECKLIST.md`: remote WSJT-X, step by step.
- `STEW-REMOTE-DIGITAL-ALC-GAIN.md`: transmit level with VB-Audio.
- `LINUX-REMOTE-MIC-TX-TROUBLESHOOT.md`: the September "no RF" case.

For the technical side: receive audio comes from the host on UDP 9100, transmit audio goes to
the host on UDP 9101, and control is on 8888 as usual.
