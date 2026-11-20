# For Stew: power settings, the server is the master (from Ron, 2026-11-20)

Ron's decision: for user settings like the power levels, the server is the master, the same rule
as the filters (`NOTE-FOR-STEW-WPF-STARTUP-FILTER-PUSH-2026-11-20.md`) and the Avalonia 1.7.08
connect fix. The client shows what the server uses.

## What Ron saw

On the Pi Zero 2 W test box, CW gave about 40 W and TUNE (100 %) 90+ W, while the client's CW
power showed 100. The trans log showed the reason: CW drive 0.1147, half the TUNE drive 0.2294,
because that Pi's `~/.local/mscc/power.ini` says `CW_POWER=50` (an old file).

The client never sent its 100: it sends `CMD_SET_CW_POWER` (0xE4) only when the slider moves,
and at connect the server's value stays. So the client showed 100 while the radio ran at 50.

## Today

- The power levels live in sdrcore-trans (`power.ini`: USB, LSB, AM, CW, TUNE, FM power).
  ms-sdr passes the client's slider values on to trans (`CMD_SET_MAIN_POWER` 0xE2,
  `CMD_SET_AM_POWER` 0xE3, `CMD_SET_CW_POWER` 0xE4, `CMD_SET_TUNE_POWER` 0xE9,
  `CMD_SET_FM_POWER` 0x9E) and keeps none of them itself.
- trans never reports them back, so neither ms-sdr nor the client can show them.
- For the filters there is already a server -> client path at connect: ms-sdr sends its saved
  indexes as `CMD_SET_BW_LOCUT_DEFAULT` / `CMD_SET_BW_HICUT_DEFAULT` / `CMD_SET_CW_BW_DEFAULT`
  (Pi `user_controls.c`, around line 666).

## What it needs (options, your call on the details)

1. Servers: at client connect, trans reports its power levels to ms-sdr, and ms-sdr passes them
   to the client, like the filter defaults. Either new "_DEFAULT"-style messages or the same
   opcodes sent server -> client. The Pi side can be done on Ron's side (Pi servers); the
   Windows servers need the same.
2. Clients (WPF and Avalonia): at connect, take the power values from the server and show them;
   don't keep a saved client value that differs. A slider move still sends the new value as
   today.

Test: set CW power to 50 on the server side (or in `power.ini`), connect a client: the slider
shows 50. Move it to 100: CW and TUNE at 100 give about the same power.
