# Note for Stew: Pi server changes the Windows servers don't have yet (2026-11-18)

Stew, while you work on the remote audio problem, Ron is changing only the Pi servers. We are not
touching the Windows servers or the WPF client, so nothing gets in your way. This note lists
what the Pi servers have that Windows doesn't, so the two can be brought back in step later.

Each release with Pi server changes will add to this list.

## R11-18-1: Pi ms-sdr 3.211

**What it does:** tells the operator when a Black Pill radio had to fix its receive by itself.
The client shows a pop-up:

    RECEIVE FIXED ITSELF
    The radio's receive was upset 3 times (921 frame errors) and the radio fixed it by itself.
    Most likely RF from the transmitter is getting into the radio.
    Check the SWR, the antenna and the station grounding.

**Why:** pill 3.22 fixes the receive when RF from the transmitter upsets it (see
`NOTE-FOR-STEW-DAUGHTER-BOARD-CLOCK-RESISTORS-2026-11-18.md`). The fix is so quick that the operator
never notices, so without this message nobody would know RF was getting into the radio.

**How it works:**
- Every 3 seconds, in the temperature loop (STM32 radios only), ms-sdr reads the pill's fix
  counter: radio command 0xC9 (CMD_AUDIO_STATS), wValue 0, wIndex 4, 32 bytes. That's the same
  read as `audio-stats.py`. Word 0 = layout (1 | 8 << 16), word 2 = fix count, word 4 = frame errors.
- When the count goes up, it waits until the radio has been receiving for 10 seconds, then sends
  one message with `Gui_Add_Message()`. It sends at most one every 10 minutes, and fixes in
  between are added to the next message.
- A pill older than 3.18 answers with a different layout word: one log line, and no messages.
  PSoC radios are skipped (STM32 only).
- Each fix also goes into the ms-sdr log as an `Rx_resync.` line.

**File:** `rpi/ms-sdr-linux/source/proficio-temperature.c`, the new function `Rx_resync_tick()`
just above `Check_temperature()`, and one call in the STM32 branch of the loop.

**Windows port:** the same function in the Windows ms-sdr `proficio-temperature.c`, using its own
USB read call. Both clients already show server messages, so neither needs a change.

## FYI: Black Pill 3.23 uses your USB names

The Black Pill now uses the same USB names as your PSoC images: "Multus IQ", "Multus IQ Sound",
"Multus IQ Receive", "Multus IQ Transmit". The servers find the radio by "Multus", so the
Windows servers need no change for it.
