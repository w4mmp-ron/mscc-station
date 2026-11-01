# For Stew: WiFi SWR meter fault now reaches the client from the Pi (client part is yours)

From Ron, 2026-11-01. In Pi ms-sdr 3.188, running on my Pi. The client needs a matching change.

## The problem

The WiFi SWR meter sends UDP **unicast**, so only one program receives it.

- **Windows host:** the client listens on 6999 itself (SETTINGS → SWR Meter). It sees the
  `fault` flag and forces RX, turns PTT off, blocks TX and shows the red RESET button. That works.
- **Pi host:** the meter sends to ms-sdr on the Pi, so the client hears nothing directly. ms-sdr
  relays SWR / FWD / REF (extended 0x0B, subs 0x0D / 0x0B / 0x0C, if `SWR_METER_TO_GUI=1`), but
  until now **not the fault**:
  - The relayed readings arrive in the client with `Fault = false`
    (`UdpRadioService.TryHandleExtendedSwrMeter`).
  - The "SWR METER FAULT" text that ms-sdr sends (extended sub 0x30, server error) has no handler
    in the client, so it is dropped.

  So at a Pi station a meter fault never shows in the client. The meter's own protection still
  works.

## What Pi ms-sdr 3.188 sends now

A new extended sub **0x0F** (`CMD_SET_SWR_FAULT`). It is free in both ms-sdr trees and in the
client's extended list. The packet is:

```
[0x0B][0x0F][fault 0/1][online 0/1][IP a][IP b][IP c][IP d]
```

- **fault:** the meter's `fault` field (1 = latched).
- **online:** 0 when the meter has gone quiet (no packet for `SWR_METER_TIMEOUT`, 3 s). Fault is
  0 then too.
- **IP:** the meter's address in normal order, a.b.c.d. 0.0.0.0 means unknown. The client can
  send its HTTP reset (`/reset`, as `SwrMeterService.ResetFaultAsync` does) straight to it.
- **When:** at once when the fault starts or clears (with the readings that caused it), with every
  meter update (at most 2 per second, so a client that connects during a fault still learns it),
  and once with "offline" when the meter stops.
- **Not gated by `SWR_METER_TO_GUI`.** A fault is a safety state. An old client ignores the unknown
  sub (it logs it only in verbose mode).

Code: `rpi/ms-sdr-linux/source/swr_wifi_meter.c` (`swr_wifi_send_fault`) and `usbavrcmd.h`
(`CMD_SET_SWR_FAULT 0x0F`).

## Client change (yours)

1. In `TryHandleExtendedSwrMeter` (or next to it), handle sub **0x0F**. Use the fault flag in place
   of the hard-coded `Fault = false`, so the existing rising / falling-edge code in
   `MainViewModel` runs: force RX, PTT and TUNE off, block TX, show the RESET button, clear on the
   falling edge.
2. **RESET:** when the meter IP from 0x0F is not 0.0.0.0, use it for the HTTP reset if
   `SwrMeterSettings.ActiveMeterIp` is empty. At a Pi station the client's own listener has never
   heard the meter, so it has no IP of its own.
3. **online = 0:** treat the meter as offline: zero the meters and clear the fault, as on the
   direct path.
4. Avalonia: the same.

The Windows ms-sdr has no WiFi meter code (the Windows client listens directly), so there is
nothing to port to it.

## Test (once your client handles it)

My Pi, with the meter sending to the Pi. Make the meter latch a fault, for example with a bad
load or by lowering its SWR threshold. The client should go to RX and show RESET, and RESET should
clear the meter. The ms-sdr log shows `Swr_wifi. FAULT latched` and `fault cleared`.

## Also: your round 1 is ported to the Pi

Pi ms-sdr 3.188 and trans 3.150 have the QRP save flag and the 0x2D status. They are the same as your
ms-sdr 3.189 and trans 3.154. They're running on my Pi with the old client (I'm not calibrating until I
have 11.1.0). The log shows `Radio_cal_send_status . store 1, RX IQ 0x0FFF, TX IQ 0x03FF, power 0x03FF`.
Please send me the 11.1.0 installer: it isn't in the repo (the newest there is R9-8-10).

One small thing in your trans 3.154: the comment on `Set_QRP_calibration` says an older ms-sdr still
saves every step. The code saves only when the flag is set, so the new trans with an old ms-sdr never
writes `power_cal.ini`. That's fine as long as ms-sdr and trans ship together; only the comment is wrong.

Ron
