# For Stew — server text messages the client does not show

From Ron, 2026-10-19. Your call; this is an option, not a request.

## What we found

ms-sdr has always been able to send a text message to the client:

    [0x0B CMD_SET_EXTENDED_COMMAND] [0x30 CMD_SET_SERVER_ERROR] [text]

It uses this for its own warnings, for example "METER THREAD NOT RUNNING. MSCC WILL NOW
STOP", and some bias and I2C device checks.

The WPF client (10.13.2) drops these without showing anything. In
`MSCC.Core/Services/UdpRadioService.cs` (`case Opcodes.CMD_SET_EXTENDED_COMMAND`, about line
2183) only the SWR meter subs 0x0B / 0x0C / 0x0D are handled; sub 0x30 falls through and is
only written to the debug monitor when verbose spectrum logging is on.

## Why it matters now

On the Pi, sdrcore-recv and sdrcore-trans now pass warnings to ms-sdr, which sends them to
the client with the same packet. The text starts with the server name:

- `SDRcore-recv: DIGITAL SPEAKER NOT FOUND (digital-speaker.ini)` (digital modes will not work)
- recv or trans: I/Q device not found (the server stops)
- trans: operator mic not found; digital mic not found, using the operator mic; operator mic
  failed to open

Ron tested it by renaming `digital-speaker.ini`: the message reached ms-sdr (it is in
`ms-sdr.log`) but nothing appeared in the client.

After the ms-sdr change below (built and running, ms-sdr 3.174), the client does receive it.
Ron's client log (`%LOCALAPPDATA%\MSCC-NET9\logs\mscc.log`, 21:41:44, just after connect):

    Udp packet received: opcode 0x0B (CMD_SET_EXTENDED_COMMAND) len=90 payload=30-53-44-52-63-6F-72-65-2D-72-65-63-76-3A-20-44...

That is sub 0x30 then "SDRcore-recv: D..." in ASCII. The client logs it and drops it; only a
handler is missing.

## The packet

- Byte 0: `0x0B`, byte 1: `0x30`, then the text as plain ASCII. **No trailing null**; the
  text is the rest of the packet.
- It may contain `\n` (ms-sdr's own messages do).
- Sent once per message. A message waits in ms-sdr until a client session is open, so it
  arrives soon after connect.
- On the Pi side Ron also moved ms-sdr's message sender onto the same socket and session
  address as the meters and keep-alive (before, it used the older `gui_s` socket and sent a
  4 KB packet padded with zeros). Built and tested: see the client log above.

## Options

1. **Show it in the client**: for sub 0x30, take the text and show it (message box, status
   line or log window, whatever suits the client). This would also show ms-sdr's own
   warnings, which today nobody sees.
2. Nothing: the messages stay in `ms-sdr.log` on the Pi only.

Windows / Ubuntu servers: their cores do not send these messages, so nothing changes there
unless you want the same.
