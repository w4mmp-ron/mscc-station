# Why the Black Pill stores the CW carrier

Stew,

A short explanation of the "stored CW carrier" in the Pi servers (R11-25-1).

## Why

The radio's USB audio runs two streams: receive I/Q from the radio and transmit I/Q to it. Today's
servers keep both running all the time; while receiving, the transmit stream just sends silence.

A Pi 4 or 5 copes. The **Pi Zero 2 W** doesn't: its one weak USB port can't carry both streams,
and it lost about 2 % of receive in bursts (about 14,000 packets in 10 minutes).

So the idea is to never run both streams together. Voice and digital are easy: stream transmit
only while transmitting. CW is the problem: the carrier comes over the transmit stream, and the
keyer can key down at any moment, so that stream had to run all the time. **Solution: the Black
Pill stores one millisecond of the carrier and plays it while the key is down.**

## How the servers work

1. **Start:** ms-sdr reads `CW_CARRIER=1;` in mscc.ini and asks the radio if it can store a
   carrier (Black Pill 3.25+). If not, everything works as before.
2. **CW:** trans records 1 ms of its own carrier (power level and I/Q correction included) and
   gives it to ms-sdr, which loads it into the radio. Then trans stops its transmit stream.
3. **Keying:** the radio plays the stored carrier. Keying shape and timing are unchanged; nothing
   goes over USB.
4. **Changes:** band, power or mode changes make trans record a new carrier. ms-sdr checks every
   5 s that the radio still has one, and clears it when the servers stop.
5. **Voice, digital, TUNE:** trans streams only while transmitting; recv pauses receive during
   transmit and resumes about a second after key-up (unless Monitor is on).

## Results

- Pi Zero 2 W: 0 lost receive packets in 10 minutes (about 14,000 without it).
- Pi 5: CW, band changes, TUNE and FT8 all tested fine.

## Notes

- Off by default; Black Pill only; Pi servers only (Windows and Ubuntu not ported, and they work
  as before without it).
- Black Pill 3.29 keeps the paused-receive time out of the pill's error counters.

Ron
