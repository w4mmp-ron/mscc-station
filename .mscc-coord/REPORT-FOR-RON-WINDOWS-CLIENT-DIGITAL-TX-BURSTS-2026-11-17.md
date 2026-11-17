# Report for Ron: Digital TX bursts come from the Windows client side (2026-11-17)

From Stew (n8vet) with Build Commander. This replaces the guesses in
`NOTE-FOR-RON-REMOTE-DIGITAL-TX-SPURS-2026-11-17.md` (speech processor, ALC, idle noise). Those are ruled out below.

## What Stew sees on the SA
Short bursts of horizontal spikes either side of the carrier during WSJT-X TUNE or an FT8 send.
They come on their own in remote, and moving the WSJT-X drive slider or the Digital/remote MIC gain sets them off.
SSB drive does not. MSCC TUN is clean. They show at WSJT-X -45 dB and under 0.3 W, so nothing is overdriven.

| Client | Server | Result |
|---|---|---|
| Win11 Shack | NEW-HP (Win11) | bursts (worst) |
| Win11 | Windows / RPi / Linux | bursts |
| stew-HP Ubuntu | RPi | clean (only a small pulse at key-down/key-up) |
| stew-HP Ubuntu | NEW-HP | clean (same small key-down/key-up pulse) |
| Standalone Windows | | small bursts when sliders move |

Same VB-Cables as a month ago, when everything tested clean. Stew's PCs are Win11 with feature updates. Ron's Win10 only gets security updates, and Ron has no SA to see this.

## What we proved
1. **The bursts are in the audio before the modulator.** A debug build of trans recorded 20 s of the audio
   going into `ssb_modulate` plus the I/Q out (Shack client to NEW-HP, WSJT-X TUNE, tone 1348 Hz).
   The tone is clean except for bursts of about 80 ms. Inside each burst the tone phase jumps at every 10 ms boundary
   (about one half cycle, 3.0 rad, with a ±0.25 rad flip-flop on each 10 ms block). The level stays steady.
   That is whole 10 ms chunks out of order or repeated/skipped: 480 frames at 48 kHz, one MSA1 packet / one capture buffer.
   The I/Q bursts line up with these, and the times match Stew's SA time marks to the 0.1 s.
2. **The speech processor and ALC are not it.** A test build that skips `doMicProc` in digital (cmd-087) changed nothing.
3. **The network and the server are not it.** A test build (cmd-087c) checks the MSA1 sequence number on every packet.
   Through a full TUNE that had a burst at 16:38:35 ET there were zero missing, late or repeated packets, no ring skips and no underruns.
   So the packets arrive in order, and trans plays them in order.
4. **So the chunks are already scrambled when the Windows client reads the mic device**, or inside the client before
   `SendPacket`. We read `MSCC.Wpf/RemoteAudio/RemoteMicSender.cs`: the ring and `TryTake`
   keep order, and the only drop path (full ring) would show as a gap, not a reorder. Nothing there reorders.

## Most likely cause (not proven yet)
The Windows capture path between WSJT-X and MSCC: WSJT-X out to CABLE-B, then MME `WaveInEvent` (10 ms buffers) through the
Win11 audio engine. A Win11 update changing audio engine/MME timing would explain why the same cables were clean a month ago
and why Ron's Win10 is not seeing it. Linux clients send in step with the device clock and are clean.

## Questions for Ron
1. Can you check your Win10 client on an SA or a waterfall (a 2nd receiver/WebSDR watching the tune tone)? Clicks every 10 ms show as faint wide splatter.
2. Would you be OK with the WPF client capturing the mic with WASAPI (shared or exclusive) instead of MME `WaveInEvent`, at least as an option?
3. Any change in the WPF mic capture/send or WSJT-X/VB-Cable setup since the clean tests a month ago that we missed?

## Leftovers on NEW-HP (uncommitted, Stew is leaving them)
- cmd-087 (skip speech processor in digital): no effect, will be backed out.
- cmd-087b (`txdump.on` file at key-down records audio + I/Q to `%LOCALAPPDATA%\MSCC-NET9\txdump-*.f32`): debug only.
- cmd-087c (MSA1 seq check logging, trans 3.163): debug only.
Briefs are in `.mscc-coord/` on NEW-HP if you want either tool.

## Separate, smaller items
- Startup comb in Remote Digital that goes away after switching to Phones and back: the Phones EQ (`RemoteAfWindow.ApplyEq` in OnLoaded) is applied while in Digital at startup.
- The small noise pulse at key-down/key-up seen on all setups.
