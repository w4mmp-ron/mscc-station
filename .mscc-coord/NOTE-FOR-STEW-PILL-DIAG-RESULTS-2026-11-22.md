# Your Black Pill test results (2026-11-22)

Stew,

Thanks for running the test on your Black Pill. Here is what it shows.

**The good news:** the Black Pill itself is working. It talks to the PC
properly, the sound chip on the board is set up correctly, and the data
coming from it looks right. The transmit test ran with no gaps.

**The problem:** almost no signal reaches the sound chip. Your receive
level is about 16 dB lower than mine with a dummy load, and even the
background noise is about 15 dB lower. What the pill hears is mostly the
sound chip's own hiss and nothing from the radio side.

So the weak receive is happening before the sound chip: in the receive
mixer, the oscillator signal that drives it, the amplifiers after it, or
the connector between the daughter board and the main board. It is not
the Black Pill firmware or MSCC.

**Could you tell me three things?**

1. What was on the antenna socket during the test: a dummy load, an
   antenna, or nothing?
2. What did your spectrum analyser show during the transmit test: one
   clean signal, or the comb?
3. Could you run the same test on your second Black Pill and send me
   that zip too?

**Worth checking on the board:** with the radio on, is the oscillator
signal actually getting to the receive mixer? The pill reports that the
oscillator chip answers, but that only means it is talking, not that its
output reaches the mixer.

73, Ron
