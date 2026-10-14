# For Stew — client "leaving" message so ms-sdr frees the session at once

From Ron, 2026-10-14. An option for you; your choice whether and when.

## What Ron sees

After Stop in WPF (connected to the Pi, Launch Servers off), a new Start has to wait until
ms-sdr times the old session out (about 45 s, client keep-alive check in `Flusher_thread`).
Until then the new connection gets "SESSION IN USE".

Reason: in connect-only mode WPF's Stop sends nothing (`UdpRadioService.cs`, CMD_SET_STOP is
skipped on purpose so the servers keep running), so ms-sdr only finds out from the missing
keep-alive.

## Idea: reuse CMD_CHECK_GUI_STATUS (0xFE) with value 0

- 0xFE value 1 = client ready / claims the session (as today).
- 0xFE value 0 = client leaving. ms-sdr releases the session at once if the sender is the
  session owner; from anyone else it is ignored.

No new opcode. An ms-sdr without this change already ignores 0xFE 0 (it only logs
"MSCC Not Initialized"), so a new client with an old ms-sdr is fine, and an old client with
a new ms-sdr behaves as today.

## Pi ms-sdr (done by Ron)

`rpi/ms-sdr-linux/source/main-controller.c`, `case CMD_CHECK_GUI_STATUS`, new branch:

```c
else if (t_opcode_data == 0 && G_client_session_active) {
    print_time(0);
    if (Session_Is_Owner(&si_other)) {
        fprintf(G_fp_logfile, "... CMD_CHECK_GUI_STATUS 0: client leaving, releasing session\n", ...);
        Session_Release();
    } else {
        fprintf(G_fp_logfile, "... CMD_CHECK_GUI_STATUS 0 from %s: not the session owner, ignored\n", ...);
    }
}
```

## If you want it on your side

- **WPF (and Avalonia if you like):** in Stop, send 0xFE with 0 on every path (connect-only
  and launched), before CMD_SET_STOP when that is sent. Also on window close.
- **Windows / Ubuntu ms-sdr:** the same few lines, if they have the same session lock.
