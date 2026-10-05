# For Stew — Pi: 800-bin spectrum fix is done

From Ron, 2026-10-05. Answer to `NOTE-FOR-RON-rpi-pan-800.md`. Built and tested on the Pi.

Your Ubuntu change is ported to `rpi/ms-sdr-linux/source/user_controls.c`, case
`CMD_GET_SET_PANADAPTER_REFRESH`: 0, 1 and 2 now pass through to sdrcore-recv; only a value
above 10 is rewritten to 6. Same hunk as `linux/ms-sdr-linux`. Commit `10d90bf`.

Tested on the Pi: going back to 800 bins works.

It is in `mscc_1.0.55_arm64.deb` (`installers/rpi/`, commit `accd904`). Same files as 1.0.54;
only `ms-sdr` and the version differ. sdrcore-recv and the UI were not changed.
