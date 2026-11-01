# For Stew: calibration status lamps from the radio, using the existing opcodes

From Ron, 2026-11-01. This replies to your `NOTE-FOR-RON-CAL-STATUS-LAMPS-2026-11-01.md`.

I agree: the lamps should show what is really in the radio. But we don't need a new opcode. We can
send the status back on the opcodes that already save each calibration. First, though, QRP needs a
real "save" step, as RX IQ and TX IQ already have.

## How each calibration is saved today

I checked this in the Pi ms-sdr, the Windows ms-sdr (both the same) and the WPF client.

| Calibration | In the radio? | What saves it today |
|---|---|---|
| QRP (power) | yes | **Nothing separate.** Every slider step is 0xA2 (`CMD_SET_BAND_POWER_POWER`, band from 0xA1). ms-sdr passes it to trans (which writes `power_cal.ini`) and queues it for the radio. |
| QRO (AMP CAL) | **no** | **Nothing separate.** Every slider step is 0x08 (`CMD_SET_POTENTIA_CALIBRATION`); trans writes `amplifier_cal.ini`. |
| RX IQ | yes | **0x57** (`CMD_SET_COMMIT_IQ`) = APPLY. Before that, 0x52 offsets are only heard, not saved. |
| TX IQ | yes | **0x57** = APPLY (offset 0x52, phase 0x2A). |

ms-sdr writes to the radio with USB request 0xC8 (type 1 = RX IQ, 2 = TX IQ, 3 = power;
type 5 reads the three valid masks). The write is sent once the value has been still for a few
ticks, and never while transmitting.

## The QRP problem

On the QRP CAL tab, Accept / Reject / Cancel only change the lamp in the client:

- Pressing **CALIBRATE** sends 0xA2 = 0, and that is written to the radio.
- **Reject** sends nothing. The last slider value stays in the radio and in the file.
- **Cancel** sends the old value with 0xA2, which is written to the radio again.

So the radio's power mask means "the slider was moved on this band", not "the user accepted".
Lamps driven by that mask would turn green after Reject or Cancel too. QRO has the same Accept /
Reject / Cancel behavior, but it isn't in the radio, so its lamps stay on the PC as you said.

## Proposal

### 1. QRP gets a save step, like I/Q's APPLY

- **Slider (0xA2):** changes the live power only. Trans uses it, but no file write and no radio write.
- **Accept:** the client sends 0xA2 again with the commit flag, `value | 0x8000`. ms-sdr then
  writes the radio and tells trans to write `power_cal.ini`.

  This is safe with old servers: an old ms-sdr reads only the low byte of the 0xA2 data, so it
  sees a normal slider step with the same value.
- **Reject / Cancel:** the client sends 0xA2 with the value from before the session (as Cancel does
  today). It is live only, so nothing is written. With a real save step, Reject and Cancel do the
  same thing; you may want just Accept and Cancel.

### 2. ms-sdr sends the status back on the same opcodes

Same message size as every ms-sdr → client message: 1 opcode byte + 4 data bytes.

| Opcode (ms-sdr → client) | Data bytes |
|---|---|
| 0x57 | byte 0 = kind: **1 = RX IQ, 2 = TX IQ** · byte 1 = flags · bytes 2-3 = mask (uint16, little endian) |
| 0xA2 | byte 0 = kind: **3 = power** · byte 1 = flags · bytes 2-3 = mask |

- **Kind** uses the same numbers as the radio's 0xC8 types, so ms-sdr can copy them straight across.
- **Flags:** bit 0 = 1 means the radio has a calibration store. 0 means older firmware with no
  store, and the client keeps using its local file as it does today.
- **Mask:** bit N = band N in MSCC order: 0 = 10 m … 9 = 160 m, 10 = 630 m, 11 = 2200 m.

ms-sdr sends three messages (RX IQ, TX IQ, power):

- at connect, after it has read the masks;
- after each successful write to the radio;
- after a clear (your type 6 reset).

Why this is safe: the client has no handler for 0x57 or 0xA2 coming from ms-sdr today. It logs
"No handler for opcode" and ignores them. An old client therefore ignores the new messages. A new
client talking to an old ms-sdr gets none, and keeps using its local file.

Don't use 0x08 for this. ms-sdr already sends 0x08 to the client (the AMP value at AMP CAL band
select). And QRO has nothing in the radio to report anyway.

### 3. Client

- **QRP CAL lamps** follow the power mask (kind 3) when it arrives. Otherwise they use
  `client-settings.ini` as today.
- After Accept, the lamp turns green when the new mask arrives, a few seconds later (after the
  radio write). So it shows what is really in the radio.
- **Optional:** the RX IQ and TX IQ masks could light lamps on those tabs.
- **AMP CAL lamps** stay on the PC.

### 4. Reset to factory

With your type 6 clear, ms-sdr clears, reads the masks again and sends them, and the lamps go
white. That also fixes "reset to factory doesn't stick" in the guides.

## Questions

1. OK with QRP Accept as a real save (0xA2 with the 0x8000 flag), and with the sliders becoming
   live only?
2. OK with status replies on 0x57 (kinds 1 / 2) and 0xA2 (kind 3), using the data layout above?
   If you'd rather have one new opcode, 0x2D is free in both ms-sdr trees and in the client.
3. Split of work: I'll do the Pi ms-sdr and trans, you do Windows ms-sdr / trans and the client.
   After your firmware testing, as you planned.

Thanks for the firmware-table answers. I'll change the guides to list today's versions with a
note that newer ones may ship.

Ron
