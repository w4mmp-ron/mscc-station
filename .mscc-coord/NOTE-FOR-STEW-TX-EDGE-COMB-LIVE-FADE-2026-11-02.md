# For Stew: your live-slot fade, go ahead

From Ron, 2026-11-02. Reply to `NOTE-FOR-RON-TX-EDGE-COMB-ANSWERS-2026-11-02.md`.

Thanks, Stew. Good catch on both edges in CW.

**Your live-slot fade is better than my stored carrier.** One piece of code covers every mode, with
nothing stored and no host changes. Let's go that way.

**No rush.** Since it can't be heard on the air, this is a cleanliness fix. Fit it in when there's
time.

## Your questions

1. **8051 time:** it should fit. Use your idea: an 8-bit gain table with 8x16 multiplies. The 8051
   only has an 8x8 multiply, so that's the cheaper way, and 256 steps is plenty for a raised
   cosine.
2. **When to do the scaling:** the main loop is risky. `Audio_Main` is polled, and
   `PCM3060_SetRegister()` busy-waits on I2C. A late 8051 means a half-written slot gets played.
   Safer options:
   - do the scaling in an interrupt off the USB DMA "done" signal, or
   - keep the dummy-Zero change, which gives a full extra ms.

   Also read the slot number once at the start. If the start-of-frame ticks during the work,
   `SyncSOF_USB_Buffer()` changes, and `SetTxBufAddress()` would point the wrong slot.
3. **Fade length:** yes, 5 ms raised cosine.

## Who does it

The PSoC is yours: the firmware change and the analyzer check. I'm working on other things.
