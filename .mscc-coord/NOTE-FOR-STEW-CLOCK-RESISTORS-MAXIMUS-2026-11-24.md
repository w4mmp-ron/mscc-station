# Note for Stew: series resistors on the codec lines (2026-11-24)

Stew,

This answers your hardware recommendation in the 2026-11-24 afternoon update of
`QUESTIONS-FOR-RON-blackpill-usb-audio.md`.

**Agreed: put the 47 ohm series resistors in the Maximus**, and in any pill daughter-board
respin. Your scope result (over 4 Vpp ringing without them, clean at about 3.5 Vpp with
them) is the proof that the 2026-11-18 note
(`NOTE-FOR-STEW-DAUGHTER-BOARD-CLOCK-RESISTORS-2026-11-18.md`) was missing.

## Which lines

Put 47 ohm in series on each line below. Place it at the STM32 pin, the source end, not at
the codec.

| Signal | STM32 pin | Codec pin | Why |
| --- | --- | --- | --- |
| BCK (BCK1 / BCK2) | PB13 | BCK1 / BCK2 | Your measurement. The receive slips after a high-SWR TUNE pointed here. |
| LRCK (LRCK1 / LRCK2) | PB12 | LRCK1 / LRCK2 | Your measurement. |
| MCLK | PA3 | SCKI (pins 6 / 9) | **Yes, not just "consider".** At 24.6 MHz it is the fastest clock. It already rang at the codec, which is why the firmware runs this pin with softer edges. |
| Transmit data (DIN) | PB15 | DIN (pin 12) | Same driver, same edges and same run to the codec as the clocks. |

The receive data line runs the other way, from the codec to the STM32. If it needs a
resistor, the resistor goes at the codec end. Your scope can tell whether it rings. I have
no reading for it.

## Firmware: slower pin edges (later, pill 3.28)

The pill drives LRCK, BCK and DIN at the STM32's fastest pin speed setting ("very high").
BCK only runs at a few MHz, so a slower setting is plenty, and slower edges ring less. This
would help every board already built, yours included, with no hardware change.

I will do it as Black Pill **3.28** later. I'll check it on the scope at the codec pins and
with the high-SWR TUNE test, and tell you when it is ready. It is an extra, not a
replacement: the Maximus should still get the resistors.

Ron
