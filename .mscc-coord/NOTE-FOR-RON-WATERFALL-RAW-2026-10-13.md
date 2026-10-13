# For Ron: waterfall needs raw data and smooth scrolling (WF SPEED dropped)

From Stew, 2026-10-13. Follow-up to `NOTE-FOR-STEW-WATERFALL-ANSWER-2026-10-13.md`.

## What was tried

cmd-077 (`.mscc-coord/cmd-077-brief.md`) built WF SPEED as you agreed (option (a)) in WPF 10.13.3:
- 1-5, default 3.
- Each waterfall row is the per-point maximum of N frames.
- In a small window, each pixel takes the maximum of the rows it covers.

Stew tested it on NEW-HP. It was **not committed and has been reverted**.

## Stew's findings

1. **Waterfall and spectrum have to stay linked** (same frames, same rate) unless the waterfall is built from the **raw, unsmoothed** full data. With smoothed frames, slowing the waterfall down only makes the smoothing trade-off worse.
2. **At slow speeds, sent CW becomes a blob.** The characters finish before the waterfall has moved enough to separate the elements, so dits, dahs and gaps merge into one block.
3. **The waterfall jumps at each row update.** That's very visible at slow speeds and still there at fast ones. It needs **smooth scrolling** (interpolating the scroll between rows) rather than stepping a whole row at a time.

## Conclusion

WF SPEED is only workable together with:
- **option (b): the host sends unsmoothed data for the waterfall.** That's a recv change and a protocol/client change on all platforms (Windows, Ubuntu, Pi recv; WPF and Avalonia), with compatibility for older clients and servers, **plus**
- **smooth scroll interpolation in the client.**

## Current state

- **WPF 10.13.2 (R10-13-2).** The waterfall runs at the full rate (15.6 rows per second) from the same smoothed frames as the spectrum.
- Keep SMOOTHING low (about 3) for a sharp waterfall; you saw no smear at 3.
- No WF SPEED control.

## Question for Ron

How would you like to proceed?
- Is (b), raw waterfall data from recv, worth doing? If so, would you do the Pi recv side first, and in what form: a second stream, a flag, or interleaved frames?
- Or leave the waterfall as it is in 10.13.2 for now?
