# Note for Ron: QRP CAL 0xAC and TUNE mode (2026-11-15)

From Stew (written with Grok Bot). Optional server backup only. We are fixing this on the clients first.

## What we saw

QRP CAL CALIBRATE / TX sends `CMD_CALIBRATION_TUNE` (0xAC). That runs `Cal_tune_button` in `linux/ms-sdr-linux/source/power_calibration.c` (about 567-588; Pi tree is the same pattern). It keys trans/recv (`CMD_SET_TX_ON`) and calls `Tune_button`, but it does **not** send `CMD_SET_MAIN_MODE` / `MODE_TUNE`.

`CMD_SET_BAND_POWER_BAND` (0xA1, about 632-654 in the same file) does send `MODE_TUNE` to recv and trans ("Set the SDRcore processes into TUNE mode"). So the first key-up after you pick a QRP band is a tune carrier. If the operator then changes mode on the main tab (USB / LSB / DIG-U) and presses CALIBRATE or TX without picking the band again, 0xAC keys in that voice/digital mode — phone audio on the SA, not a carrier. Re-selecting a QRP band (0xA1) puts the cores back in TUNE and looks like a fix.

ms-sdr `G_mode` is also left on the client's last mode (`Tune_button` logged `G_mode: U` while trans had been set to TUNE by 0xA1).

## Client fix (in progress)

Avalonia (stew-HP, cmd-081 Part B) will set mode TUNE and 0xA6 before 0xAC, and restore the previous mode on stop, leaving the tab, and disconnect — same pattern as AMP CAL / TX IQ / the main TUN button. That covers the bug whether or not the server changes. WPF has the same 0xAC-only gap; we will fix that next on Shack.

Version stays Avalonia 1.7.03 (unreleased). This note is uncommitted for now; it will go in with the cmd-081 commit.

## Optional server backup

If you agree, have `Cal_tune_button` also send `MODE_TUNE` to recv and trans, the same way 0xA1 already does, in both the Pi and Linux ms-sdr trees. Then a bare 0xAC cannot key in USB/phones. Not required once the clients are fixed; it would be a useful safety net for an older client.
