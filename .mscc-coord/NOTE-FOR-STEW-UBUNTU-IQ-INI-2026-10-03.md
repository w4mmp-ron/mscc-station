# For Stew — Ubuntu: reload after a radio swap looks like it deletes iq.ini

From Ron, 2026-10-03. Read from the source, not run.

After a radio swap, ms-sdr `Factory_seed_reload_servers()` sends `CMD_SET_IQ_DEFAULTS` to
sdrcore-trans so it re-reads the `iq.ini` that was just put in place.

- **Windows trans** (`udp_thread.c`, since cmd-019): re-reads the live file. Good.
- **Ubuntu trans** (`linux/SDRcore-trans-linux/sources/udp_thread.c`, case
  `CMD_SET_IQ_DEFAULTS`): still calls `delete_iq_ini_file()` first, then rebuilds the built-in
  defaults. So the parked or factory `iq.ini` that ms-sdr just loaded is thrown away.
  Same for TX IQ Reset All: the factory file is copied in, then deleted.

Fix: make the Ubuntu case the same as Windows (no delete; `Check_iq_ini_file()` then
`init_IQ_structure()`, so the built-in file is created only if `iq.ini` is missing).

The Pi had the same code and now matches Windows; the Pi hunk is in
`rpi/SDRcore-trans-linux/sources/udp_thread.c`, commit `c3c48a5`.
