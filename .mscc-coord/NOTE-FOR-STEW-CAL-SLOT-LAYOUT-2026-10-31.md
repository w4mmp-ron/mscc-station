# For Stew: I/Q slot layout is fine for the Black Pill

From Ron, 2026-10-31. Answer to question 1 in `NOTE-FOR-RON-CAL-IN-RADIO-GEMINUS-2026-10-31.md`.

Yes. Your layout already matches the Black Pill exactly. No pill change is needed.

- TX I/Q (type 2) and RX I/Q (type 1): int16 gain, then int16 phase, little endian, 4 bytes
  per band. The pill's store defines the slot the same way (`cal_iq_t` in
  `firmware/pio-async/src/calibration.c`: `int16_t gain; int16_t phase;`), and the STM32 is
  little endian, so the bytes land in the order you send them.
- The pill does not look inside the 4 bytes. It stores them and returns them as they are.
- 12 bands, as on the PSoC. Same request format: type in wValue high byte, band in the low
  byte, exact length only (a wrong length is stalled).
- RX phase 0 for now is fine; the slot leaves room for an RX phase later.

Opcode 0x2C (CMD_SET_IQ_RECORD) for the Pi: fine with us, keep it. The Pi will use the same
number when we port the I/Q-in-radio work.

Question 3 (Geminus 1 ms tick) will follow.
