# For Stew: the WiFi SWR meter trips during auto-tune. We need the meter firmware source in the tree

From Ron / Claude, 2026-11-01 (evening).

## The short version

When Ron starts an auto-tune, the WiFi SWR meter latches a fault about one second into the tune.
That is before the tuner has had time to find a match. The meter's "Fault Trip Count" setting is
already at its maximum, and even the maximum is too short for a slow auto-tuner. This is a limit in
the meter firmware. Nothing in the servers or the client is causing it.

To fix it we need the meter firmware source. It is not in the tree; only the `swr-meter/` Python
app and a summary are there. **Could you please add the firmware source to the tree?** Once it is
there, either of us can make the change described at the end of this note.

## How the meter, ms-sdr and the client fit together at Ron's station

Ron runs the Windows client with the servers on the Pi. The meter sends its readings over UDP
unicast to one address only, and at Ron's station that address is the Pi. So ms-sdr receives the
readings and passes them on to the client. That includes the fault, using the new extended
opcode 0x0F that you added to client 11.1.5.

That part is now working. Ron installed client 11.1.5 tonight and tested it:

- With a fault latched in the meter, the client showed the fault and the RESET button.
- Ron pressed RESET, which uses the client's direct HTTP reset because Ron is on the meter's
  network. The meter cleared, and ms-sdr passed the cleared state on to the client.
- During the trips described below, the client turned TX off **2 ms** after ms-sdr passed on the
  fault. So the protection itself works very well. The problem is only that it trips when it
  shouldn't.

## What happened tonight: the measurements

All times are from the Pi's ms-sdr log. "Fault after" is the time from TUN on to the meter's fault.

| Band | Tune power | Tune length | Fault? | Fault after | FWD | REF | SWR at the fault |
|---|---|---|---|---|---|---|---|
| 20 m (14.074) | 43 | 4.4 s | no | - | - | - | - |
| 30 m (10.136) | 43 | 7.4 s | no | - | - | - | - |
| 15 m (21.074) | 43 | - | **yes** | **1.0 s** | 61 W | **25.00 W** | 4.55 |
| 30 m (10.136) | 34 | 5.5 s | no | - | - | - | - |
| 15 m (21.074) | 34 | - | **yes** | **0.86 s** | 38 W | **25.00 W** | 9.82 |

What this tells us:

1. **The trips come at the very start of a tune.** At that moment the tuner is still searching, so
   a high SWR for a short time is normal. It is exactly what an auto-tuner does while it hunts for
   a match.
2. **It depends on the band.** 30 m and 20 m got through. 15 m tripped both times. Ron's tuner has
   to hunt harder on 15 m, so the high-SWR stretch lasts longer than the meter allows.
3. **Lowering the tune power did not help.** Going from 43 to 34 cut the forward power from 61 W to
   38 W, but the meter still tripped at the same point.
4. **REF read exactly 25.00 W both times.** Two identical readings to the hundredth of a watt are
   very unlikely by chance. Our guess is that 25 W is either the top of the meter's
   reflected-power scale or a fixed level in the trip logic. We can't tell which without the
   source. It is worth checking when we look at the code, because if the reading is clipped at
   25 W the SWR the meter calculates at that moment is also wrong.

## Ron's diagnosis

The meter has a setting called **Fault Trip Count**. As we understand it, this is how many
high-SWR readings in a row the meter accepts before it latches a fault. In other words, it is how
long the meter is willing to wait. Ron already has it set to the **maximum**. Even at the maximum
the wait is only about one second, and a slow tuner on a hard band needs longer than that.

So the fix belongs in the meter firmware. We should not work around it in ms-sdr or the client.
For example, we could have ms-sdr ignore meter faults while TUN is on, but that would remove the
protection at exactly the moment when the SWR is highest. We don't want that.

## What we would like to change in the firmware (once the source is in the tree)

Either of these would solve it. We'd like your opinion on which you prefer.

**Option A: allow a longer trip count.** Raise the maximum of the Fault Trip Count setting so the
meter can wait several seconds instead of about one. This is the simplest change. The downside is
that it also makes the meter slower to react during normal transmitting, not just during a tune.

**Option B: a separate, longer allowance at the start of each transmission (Ron and Claude's
preference).** The meter can see when power appears after a period with no power: forward power
rising from near zero above its minimum-power level. For the first few seconds after that, it
would use a longer allowance, for example 5 to 10 seconds, adjustable, so the tuner can finish
its search. After that it would go back to the normal fast Fault Trip Count. Normal operation keeps
its fast protection, and auto-tuning gets the time it needs.

Either way, we should also look at the 25.00 W reflected reading (point 4 above) while we are in
the code.

## Questions for you

1. Can you add the meter firmware source to the tree, and tell us which folder it is in?
2. Do you agree the fix belongs in the firmware, and do you prefer option A or option B?
3. Do you know the meter's real reflected-power limit? Is 25 W the top of its scale?
4. Who should make the firmware change once the source is in the tree, you or Ron and Claude?
   And how do we load new firmware into the meter: over WiFi, or by USB?

## Not related to the trip count, but still open

- **RESET through ms-sdr (0x2E):** done on the Pi in ms-sdr 3.189, as you proposed. It is waiting
  for your client side, which sends 0x2E when the meter state came from ms-sdr (0x0F). See
  `NOTE-FOR-STEW-2TONE-PI-AND-0x2E-2026-11-01.md`.
- The client still has no handler for the "SWR METER FAULT" text message (0x30). The 0x0F fault
  works without it, so this is low priority.
