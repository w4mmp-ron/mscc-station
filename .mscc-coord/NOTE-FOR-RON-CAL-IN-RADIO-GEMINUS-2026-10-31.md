# For Ron: calibration in the radio (TX/RX I/Q), Geminus 2.152/2.153, ms-sdr 3.188

From Stew, 2026-10-31. Windows servers + firmware, tested on Stew's Proficio MKII PTT and a
customer Geminus MKII (moved to a fresh PC: its calibration loaded from the radio).

## 1. I/Q in the 0xC8 store (ms-sdr 3.187/3.188, trans/recv 3.151)

- Slot layout we used (please confirm for the Black Pill): **TX I/Q and RX I/Q = int16 gain +
  int16 phase, little endian**. Gain = IQ_OFFSET steps (the iq.ini value), phase = IQ_PHASE
  steps (0.05 deg; 0 for RX). Power = uint16 as before. Bands in MSCC record order.
- APPLY (0x57) writes the band to iq.ini as before and also queues it for the radio; the
  Flusher sends it when out of TX.
- Start: after the power load, ms-sdr reads the valid masks, then each band the radio holds,
  and sends it to the core with a new opcode **0x2C CMD_SET_IQ_RECORD** (ms-sdr -> trans for
  TX, -> recv for RX): int32 = record (bits 0-7) | offset int12 (bits 8-19) | phase int12
  (bits 20-31). The core updates its ini and applies it if that band is in use. Bands the
  radio does not hold keep the ini value (same rule as power since 3.185).
- If you would rather use a different opcode number on the Pi, tell us before we commit to it.

## 2. TX I/Q from CW mode (ms-sdr 3.188)

During calibration ModeChanged() does not send the mode to the radio (power cal owns it), and
TX I/Q relied on that too. Started from CW, the radio stayed in CW, which ignores TX on (on
the Geminus TX_Main only runs out of CW), so TX I/Q never transmitted. Now a flag set by
IQ_CALIBRATION_RX_TX (cleared by CMD_SET_MAIN_FREQ and by power cal) lets mode changes reach
the radio during an I/Q session. Pi ms-sdr probably has the same issue.

## 3. Geminus MKII firmware

- **2.152**: your Proficio calibration.c (3.238/3.239 behaviour, no clear) appended to
  settings01.c, 0xC8 handlers in usbvend01.c. EEPROM rows 8-19 (Geminus only uses row 0).
- **2.153**: edge shaping as Proficio 3.240-3.242 / your Legacy relay design. The Geminus has
  no 1 ms timer ISR, so the 1 ms tick comes from STATUS_BEAT in the main loop.
  - TUN/PTT: DIN blank + DAC mute, T/R + BS2 + AMP on, 25 ms, DIN on, unmute; release: mute,
    18 ms, AMP + BS2 off, 10 ms, T/R off. Mode-change drain as 3.242.
  - CW: relays latched per session (unchanged), elements keyed by the DAC soft mute with the
    key-delay buffer. Two differences from your Legacy copy: fixed 30 ms delay (not TX hold /
    16), and DIN stays open for the session (DAC mute only between elements).
  - Before 2.153 the Geminus switched the relays with RF and keyed CW with DIN only: broadband
    noise on TUN release and key clicks. Now clean on the Signal Hound; scope ~34 ms LED on to
    full carrier.

## Questions

1. Slot layout OK for the Black Pill?
2. Why was the 0xC8 clear (type 6) removed in 3.239? We will need it for "reset to factory".
3. Worth a hardware 1 ms timer on the Geminus, or is the main-loop tick fine?
