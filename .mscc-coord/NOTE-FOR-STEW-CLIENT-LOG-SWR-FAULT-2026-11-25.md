# Note for Stew: SWR meter status floods the client log (2026-11-25)

Stew,

With the WiFi SWR meter on, the WPF client log fills with lines like this:

```
:1762 Udp packet received: opcode 0x0B (CMD_SET_EXTENDED_COMMAND) len=7 payload=0F-00-01-C0-A8-0C-DE
```

This is ms-sdr's meter status, extended sub-command `0x0F` (`EXT_CMD_SET_SWR_FAULT`):
no fault (`00`), meter online (`01`), meter IP 192.168.12.222. ms-sdr sends it together with
FWD / REV / SWR about every half second (`swr_wifi_meter.c`, `swr_wifi_send_fault(fault, 1)`).
Nothing is wrong. It is just noise in the log.

## Fix (client, yours)

The client already keeps the FWD / REV / SWR sub-commands out of the log. `0x0F` was not added
to that list. In `MSCC.Core/Services/UdpRadioService.cs`, `quietExtendedSwr` (about line 1697,
WPF and Avalonia):

```csharp
bool quietExtendedSwr = e.Opcode == Opcodes.CMD_SET_EXTENDED_COMMAND
    && e.Payload.Length >= 1
    && e.Payload[0] is Opcodes.EXT_CMD_SET_FORWARD_POWER
        or Opcodes.EXT_CMD_SET_REVERSE_POWER
        or Opcodes.EXT_CMD_SET_SWR
        or Opcodes.EXT_CMD_SET_SWR_FAULT;   // add
```

Suggestion: still log a `0x0F` with fault = 1 (payload byte 1), so a real meter fault shows in
the log.

Ron
