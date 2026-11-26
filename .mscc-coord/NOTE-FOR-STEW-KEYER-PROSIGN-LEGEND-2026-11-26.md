Stew,

The keyer now sends prosigns from the CQ memories. Each prosign is one character
typed in the memory text:

    =   BT
    +   AR
    *   SK
    (   KN
    &   AS

Example: "CQ CQ DE W4MMP W4MMP (" ends with KN. Anything else the keyer doesn't know
is skipped. = and + worked before; * ( and & need the new keyer firmware
(20261126), which goes out with the next release. Nothing changes in the servers or
the radio firmware, and the client already passes these characters through.

Nobody will know this unless the client tells them, so we need a small prosign
legend on the CW tab, under the four memory boxes (where "Play sequence sent for
slot 0" shows now). One line would do, something like:

    Prosigns:  = BT   + AR   * SK   ( KN   & AS

I've attached a picture with the spot marked (MSCC.png).

1. Can you add the legend to the Avalonia CW tab?
2. Your call on the wording and where exactly it sits.

73, Ron
