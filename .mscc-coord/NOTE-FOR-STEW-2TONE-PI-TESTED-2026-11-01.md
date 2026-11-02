# For Stew: 2 TONE works on the Pi, and the SWR fault works with client 11.1.5

From Ron / Claude, 2026-11-01 (evening). This follows `NOTE-FOR-STEW-2TONE-PI-AND-0x2E-2026-11-01.md`,
where we ported your 2 TONE to the Pi trans (3.151) and added 0x2E to the Pi ms-sdr (3.189).

## 1. 2 TONE on the Pi: tested on the air, it works

Ron installed client 11.1.5, restarted the Pi servers (trans 3.151, ms-sdr 3.189) and ran 2 TONE
on 20 m (14.074 MHz) for about 6 seconds.

What the logs show, in order:

- The client sent 2 TONE on (0x88 = 1). ms-sdr passed it to trans, and trans set two-tone on.
- The client keyed TX. ms-sdr put the radio in transmit in USB, and trans was in USB (opmode 2),
  so the tones replaced the mic audio and went out through the normal voice path.
- After about 6 seconds the client turned TX off, then sent 2 TONE off (0x88 = 0).
- No errors anywhere. The ALC limit stayed where it was before the test.

On the radio side, Ron saw **full power** during the test.

What we could **not** check: the shape of the signal. The MSCC panadapter stops during transmit
(it is fed by the radio's own receiver), so the two tones and the IMD products can't be seen
there. To see them you need a second receiver, a nearby SDR or a spectrum analyzer like your
Signal Hound. If you get the chance to look at a Pi station's 2 TONE on your analyzer, it would be
good to confirm it looks the same as your Windows result: each tone a little under -8 dBc against
the TUN carrier, and IMD3 about 34 dB below the tones.

One small thing for the guide: the 2 TONE button is on the **RX/TX tab** (TX Options, the 2×2
group QRP | AMP / Tune | 2 TONE), not on the main window. Ron looked on the main window first and
couldn't find it. Either a line in the guide or a 2 TONE button near TUN on the main window would
help other users.

## 2. SWR meter fault with client 11.1.5: tested, it works

- With a fault latched in the meter, the client showed the fault and the RESET button.
- Ron pressed RESET (the client's direct HTTP reset, since he is on the meter's network). The
  meter cleared, and ms-sdr passed the cleared state on to the client.
- During real trips in an auto-tune, the client turned TX off **2 ms** after ms-sdr passed on the
  fault.

The trips themselves happen too early in an auto-tune. That is the meter's Fault Trip Count, and
it is in its own note: `NOTE-FOR-STEW-SWR-METER-TRIP-COUNT-2026-11-01.md`. We need the meter
firmware source in the tree to fix it.

## Still waiting on your side

- The client side of 0x2E (RESET through ms-sdr, for clients that are not on the meter's network).
- The meter firmware source in the tree.
- A client handler for the "SWR METER FAULT" text message (0x30). Low priority, since 0x0F works.
