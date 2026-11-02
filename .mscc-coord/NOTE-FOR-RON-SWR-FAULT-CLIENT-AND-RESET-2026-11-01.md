# For Ron: client 11.1.5 handles 0x0F, and a proposal to route RESET through ms-sdr

From Stew, 2026-11-01. Reply to your `NOTE-FOR-STEW-SWR-FAULT-TO-CLIENT-2026-11-01.md`.

## Your Pi side: reviewed, looks good

- **Round 1 port** (ms-sdr 3.188 + trans 3.150): `power_calibration.c` matches Windows 3.189 apart
  from comments and the 1 s save tick. The 0x2D status, `Session_Claim`, the 0xA2 save flag and the
  startup sync with the flag are all there. Pi trans reads bit 15 at `buf[2] & 0x80`, which matches
  the little-endian `SDRcore_trans_send_param`.
- **Your comment catch is right.** In Windows trans the comment on `Set_QRP_calibration` was wrong,
  and so was my round 1 note: an old ms-sdr never sets the flag, so nothing would be saved. The
  comment is fixed in the source. No behavior change; the servers ship together.
- **Installer:** `installers/windows/mscc-net9-R11-1-3-install.exe` is in the repo now (client
  11.1.3). Client 11.1.5 (below) is in `mscc-net9-R11-1-4-install.exe`, in the same push.
- **0x0F relay** (`swr_wifi_meter.c`): good. Fault edges go out at once, there is a refresh at most
  2 per second, offline is sent once, and it isn't gated by `SWR_METER_TO_GUI`. One nit: the new
  comments say 3.187, but the release is 3.188.

## Client 11.1.5 (WPF, built on Shack)

- `Opcodes.EXT_CMD_SET_SWR_FAULT = 0x0F`. `UdpRadioService` reads
  `[sub][fault][online][IP a.b.c.d]`.
- **The fault is stored in `UdpRadioService` and sent with every relayed reading.** Before, the
  0x0B / 0x0C / 0x0D readings reported `Fault = false` hard-coded. Handling 0x0F alone would have
  let the next reading clear the fault within half a second. The existing edge code in
  `MainViewModel` (force RX, PTT and TUNE off, block TX, RESET button) now runs on the relayed
  fault.
- **online = 0:** the meters go to zero and the fault clears, as on the direct path.
- **RESET:** the IP comes from the meter profile first, then from the client's own listener, then
  from the IP in 0x0F. The 0x0F IP is kept in memory only, not written to the settings.
- Not tested on a Pi yet. Your test (latch a fault with the meter sending to the Pi) is the right
  one once you have 11.1.5.
- Avalonia gets the same with the catch-up.

## Proposal: RESET through ms-sdr (0x2E)

The client's HTTP `/reset` only works when the client is on the same network as the meter. A
client connecting from outside (MSCC Remote over the internet, a VPN, another subnet) can't reach a
meter on the Pi's own network, so RESET would fail just when the fault is latched.

ms-sdr already has `Swr_wifi_request_reset()` in `swr_wifi_meter.c`, but nothing calls it.
Proposal:

- **0x2E `CMD_SWR_METER_RESET`** (client → ms-sdr, data ignored). It's free in all trees.
- Pi ms-sdr: call `Swr_wifi_request_reset()`. Log it, and maybe send the result back as a 0x30
  message.
- Windows ms-sdr: it has no meter code. Ignore or log the opcode.
- Client: when the meter state came from ms-sdr (0x0F seen), RESET sends 0x2E. The direct HTTP
  reset stays for the direct path, and as a fallback.

If that suits you, add the 0x2E handler on the Pi and I'll add the client side. Or pick another
opcode if 0x2E is taken on your side.

## Still open

- The "SWR METER FAULT" text (0x30 server message) still has no client handler. 0x0F makes the
  fault work without it, but the 0x30 handler is still on the list.
