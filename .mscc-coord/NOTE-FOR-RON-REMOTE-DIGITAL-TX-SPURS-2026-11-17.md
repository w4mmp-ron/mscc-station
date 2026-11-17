# Note for Ron: Remote Digital TX spurs (WSJT-X tune), 2026-11-17

From Stew (n8vet), written with Build Commander. Questions for Ron are at the bottom.

## Summary
WSJT-X TUNE over **Remote Digital** puts out spurs around the carrier on the spectrum analyzer.
It happens on every server we tried, Windows and Linux. Local TX is clean, TUN from MSCC is clean, and remote phone is clean.
About 3-4 weeks ago (before the CW delay / PSoC / server change run, late Oct to Nov 10) Stew had this fully tested in every configuration and mode with no spurs. So something in that window broke it.

## What the SA shows
Pictures are in `.mscc-coord/remote audio SA sweeps/`. All are 20 m, 100 kHz span, RBW 100 Hz.

1. **First TX right after picking Remote Digital:** a comb of spikes about 1 kHz apart across the whole 100 kHz span (a jamming-type signal). Shown here from Windows client NEW-HP to the Pi.

   ![Comb on first WSJT-X tune](remote%20audio%20SA%20sweeps/comb-first-tune.png)

2. **After a Digital > Phones > Digital switch:** the comb goes away and random horizontal bursts show next to the carrier, with no fixed timing.

   Windows client (NEW-HP) to Pi server, spurs mostly on the low side:

   ![Windows to Pi](remote%20audio%20SA%20sweeps/windows-to-rpi.png)

   Windows client (Shack) to Windows server (NEW-HP), spurs on both sides and worse:

   ![Windows to Windows](remote%20audio%20SA%20sweeps/windows-to-windows.png)

## Test matrix (all 20 m, WSJT-X tune, 2026-11-17)
| Client | Server | Result |
|---|---|---|
| NEW-HP (Win11 WPF) | stew-HP (Ubuntu) | comb, then low-side spurs |
| NEW-HP (Win11 WPF) | Pi 5 (mscc 1.0.96, trans 3.160) | comb, then low-side spurs |
| Shack (Win11 WPF, R11-16-2) | NEW-HP (Windows server) | comb, then spurs on both sides (worse) |

Virtual cables on the clients were set to 2 ch / 16 bit / 48000 Hz for the last two runs (they were 44100 before). No change.
The VB cable is not the Windows default device, and "Listen to this device" is off.

## What the server logs rule out
- **Network:** every `remote_mic` packet arrived (`bad=0`), and there were no `under`/`overflow` counts during any TX.
- **Clipping:** peak during tune is about 8700-14400 out of 32767.
- **Buffer / rate adapter:** on the Windows server TX at 15:22:27-15:22:46Z, `occ` was steady at about 4630-4650 with `step` steady at 0.499996, and there were no gaps. The spurs were still there. So the buffer catch-up is not the cause, at least on Windows.

## The clue
While WSJT-X is **idle** (not transmitting), the server still gets audio on the remote mic stream. `peak` is about 300-1700 (about 1-5% full scale) between TX periods, on both the Pi (from NEW-HP) and the Windows server (from Shack). Earlier on the Pi, idle `peak` was 0.
Stew watched the VB cable recording meter on Shack with WSJT-X idle. It blinks every so often, at random.
That random low-level audio goes out with the tune tone, and it matches the random spurs. We have not found where it comes from on the client side.

## Commits in the window (candidates, not proven)
- `d5aec9f6` 10-20: Legacy Proficio 1.232, non-QSK key delay, key-click shaping
- `b5074a07` 10-24: Pi ms-sdr lighter PTT polling
- `a6024651` / `025439b5` / `be4a7dcf` 10-30: trans ALC without per-sample gain steps, TX audio low-pass, RX stream kept at PTT
- `e7e40565` 10-31: trans 3.153 full digital TX power, no key-up bump
- `6d6820da` 10-31: ALC ceiling 0.45 to 0.89
- `3a1a3206` 11-02: DAC muted in non-QSK RX, 12 kHz comb findings
- `f78d1416` 11-05: trans operator mic fix
- `c6e6be27` 11-08: TX underruns traced to trans stream reopen on every mode command (Digital), no change made
- `75acb507` / `57db097b` 11-10: servers made the same on Pi and Windows (trans 3.157/3.158)

## Questions for Ron
1. Did anything in the remote mic / Remote Digital path change in this window, on the client send side or in trans (gate, fade, ALC, low-pass, resample)?
2. Should the client send remote mic audio while not in TX? If yes, is it gated in trans so only TX audio reaches the modulator?
3. What in the first TX after Remote Digital is picked would make a 1 kHz comb? Is it the stream reopen in `c6e6be27`?
4. Can you run WSJT-X tune over Remote Digital at your end and check the SA? If you see it too, which build would you call the last good one, so Stew can bisect?

## Other Pi items from today (separate, lower priority)
- Pi trans hung opening the operator mic when MSCC Init had set it to `pipewire`. `Init_Power_All()` never ran and the drive stayed at 0 (no RF on TX). Fixed by picking the Sound Blaster mic. Trans should load power cal even if the operator mic fails to open.
- MSCC Init did not list the Sound Blaster as a mic the first time, and offered `pipewire`.
- I/Q OpenStream failed once (`-9999` on hw:2,0) right after a stream stop from an early client `CMD_SET_AUDIO_DEVICE DIGITAL`.

