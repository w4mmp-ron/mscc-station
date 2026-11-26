Stew,

The keyer can now play a CQ memory over and over, with a pause between plays,
for calling CQ or as a beacon. It's tested on my rig and works well.

How it works on the air: the memory plays, the radio goes back to receive for
the pause, then it plays again. A tap on either paddle stops it, during the
message or during the pause, and the tap itself sends nothing. Pressing P again
or storing a memory also stops it. There is no automatic stop; tapping the key is
easy enough.

It needs the new keyer firmware (20261126), which goes out with the next release.
Nothing changes in the servers or the radio firmware; they pass it straight through.

What the client sends, all as the usual keyer memory command (0x9C), one value per
packet like the other memory commands:

    3, slot      select the slot (same as now)
    4, pause     play repeating; pause is 1 to 30 seconds

So instead of "3, slot, 0" for a single play, it's "3, slot, 4, pause". An older
keyer ignores the 4, so nothing plays.

In the client I'd suggest a REPEAT tick box and a PAUSE box (seconds, 5 is a good
start) under the memories. With REPEAT ticked, P starts repeating, and the status
line can say "tap the paddle to stop". Keep both settings between runs.

1. Can you add repeat play to the Avalonia CW tab?
2. Your call on the layout and wording.

73, Ron
