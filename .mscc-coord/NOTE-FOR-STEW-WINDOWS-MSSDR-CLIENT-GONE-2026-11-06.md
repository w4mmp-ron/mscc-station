# For Stew: Windows ms-sdr stops when a remote client leaves

From Ron, 2026-11-06. An option for you; your choice whether and when.

## What I see

Windows host (servers on my PC), Avalonia on the Pi as the client. When I close the client,
about 45 seconds later all the Windows servers stop. To connect again I have to start them
again on the PC. With the Pi as the host, the servers keep running and the next client just
connects.

## Why

In `ms-sdr-MKII/source/main.c`, `Flusher_thread`, the client keep-alive check sets
`servers_failed = 1` ("Client FAILED"). Case 1 calls `Session_Release()`, which is right, but
then falls through to the same code as a dead recv or trans:

```c
Sleep(5000);
Stop_all(1, 0);
```

The Pi ms-sdr (`rpi/ms-sdr-linux/source/main.c`) frees the session and carries on:

```c
case 1:
    fprintf(G_fp_logfile, "[%d] Flusher_thread. Client FAILED: %d\n", line_number++, servers_failed);
    Session_Release();
    servers_failed = 0;
    break;
```

The Windows ms-sdr already has the other half of this: `CMD_CHECK_GUI_STATUS` 0 (client
leaving) releases the session at once.

## If you want it

Add `servers_failed = 0;` after `Session_Release();` in case 1, as on the Pi. Then the servers
stay up when a client leaves, and only a dead recv or trans stops everything.

If closing the client is meant to stop the servers when they all run on one PC, that still
works: the client sends CMD_SET_STOP when it started the servers itself.

## Also, while you're in there (no rush)

Remote Digital from the Pi client worked on the air today (two FT8 QSOs). I can't see what the
RF looks like. Could you have a look at it sometime?
