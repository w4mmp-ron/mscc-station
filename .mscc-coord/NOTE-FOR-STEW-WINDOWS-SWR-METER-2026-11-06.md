# For Stew: the Windows ms-sdr doesn't relay the WiFi SWR meter

From Ron, 2026-11-06. An option for you; your choice whether and when.

## What I see

With the servers on a Windows PC, the client gets no SWR, forward or reflected power, and no
SWR fault. With the Pi as the host, the WiFi SWR meter's readings show in the client.

## Why

- The Windows ms-sdr has no `swr_wifi_meter.c`.
- `meter.c` is in the folder but not in `ms-sdr.vcxproj`, and `main.c` doesn't start
  `Meter_main`, so nothing sends `CMD_SET_SWR` / `CMD_SET_FORWARD_POWER` / reverse power.
- The extended subs 0x0F (SWR fault) and 0x2E (SWR meter reset) are Pi ms-sdr only.

The CAT meter reply to WSJT-X is a fixed value on both, not a real reading.

## If you want it

The Pi's `rpi/ms-sdr-linux/source/swr_wifi_meter.c` could be ported to the Windows ms-sdr
(UDP JSON from the meter, the same GUI meter opcodes, the 0x0F fault and the 0x2E reset), with
the same `SWR_*` settings in the ini. I can do the port if you'd like.
