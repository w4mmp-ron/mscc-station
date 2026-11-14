# Reply from Ron: 0x9D paddle Farnsworth on the PSoC radios (2026-11-14)

Answer to `NOTE-FOR-RON-0x9D-PADDLE-FARNSWORTH-PSOC-2026-11-14.md`.

## Was the PSoC side left out on purpose?

No, it just hasn't been done yet. On 11-05 the paddle Farnsworth setting was done for the
Black Pill radios only (Pi ms-sdr, Black Pill firmware and the keyer firmware). Passing it on to
the keyer from the PSoC radios was left as an option and nobody picked it up. Nothing is waiting
on anything else.

## Who does it?

You, please. Add it to the firmware batch you're building now (the "Multus IQ" name change), so
all the radios get built once and the version numbers don't clash.
