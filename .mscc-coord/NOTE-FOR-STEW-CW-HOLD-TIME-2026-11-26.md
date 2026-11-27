Stew,

Three client things from today's keyer work, for the Avalonia CW tab. The first two
already have their own notes (prosign legend, repeat play). This one is about HOLD.

The HOLD number on the CW tab is labelled "ms", but the radio counts it in 10 ms
steps. Both the Black Pill and the PSoC MKII do it that way (the PSoC timer runs at
about 10 kHz and the period is the value times 100). So:

    client shows 100   ->  the radio holds 1.0 second
    client shows 10    ->  100 ms
    client shows 1     ->  10 ms on the PSoC, 50 ms on the Black Pill (its shortest)

The value also goes to the radio as one byte, so anything over 255 wraps round:
260 comes out as about 50 ms, and 500 as 2.44 seconds. The client lets you go up
to 500 today.

What I'd suggest for Avalonia:

- Show the real time: the stored value times 10, labelled ms. Keep sending the
  same value to the radio, so nothing changes in the servers, the radio or the
  saved settings.
- Let - / + move in 50 ms steps, always landing on whole steps, from 50 ms to
  2550 ms (255), so it can't wrap.
- Optional, but nice: an AUTO tick box. It sets the hold from SPEED and
  FARNSWORTH, halfway between the keyer's letter gap and word gap, so the radio
  stays in transmit through the letters and drops to receive between words. With
  the keyer's own Farnsworth sums that is 660 ms at 18/13, 330 ms at 18 wpm with
  Farnsworth off, and 560 ms at 20/15. It follows any speed change. When AUTO is
  unticked, put back the hold the operator had before.

For reference, the keyer's gaps (ARRL / PARIS Farnsworth, from the keyer code):

    dot = 1200 / character wpm   (ms)
    Farnsworth on (text wpm below character wpm):
        S = 50 x char - 31 x text
        letter gap = 3 x S x dot / (19 x text), at least 3 dots
        word gap   = letter gap + 4 x S x dot / (19 x text) (that part at least 4 dots)
    Farnsworth off: letter gap 3 dots, word gap 7 dots

1. Can you fix the HOLD label and range in Avalonia?
2. AUTO is your call.

73, Ron
