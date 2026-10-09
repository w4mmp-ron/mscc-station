# For Stew — ALC button starts in the wrong state; ms-sdr now stores it

From Ron, 2026-10-09.

**Seen:** the ALC button on the Rx/Tx tab can start lit while the ALC is off. After one
click off / on it is right.

**Cause:** the state was stored nowhere.

- WPF starts with `AlcOn = true` (`MainViewModel.cs:266`) and sends
  `CMD_SET_ALC_MULTIPLIER` (0x23) only on a click, never at connect.
- ms-sdr only forwarded 0x23 to trans.
- trans keeps it in memory (on at every start).

So after a client restart with the servers still running and ALC off, the button says on.

**Decision (Ron):** the state belongs in ms-sdr, in `user_controls.ini`, like the other
controls. Not in the client.

## Done on the Pi (`rpi/ms-sdr-linux`)

- New key in `user_controls.ini`, named after the opcode: `CMD_SET_ALC_MULTIPLIER=0|1`.
  Default 1 (on), also when an older file has no such line.
- `user_controls.c`:
  - `User_Controls_Init` reads the key.
  - `User_Controls_Process` has a `CMD_SET_ALC_MULTIPLIER` case: store 0 / 1, forward to
    trans, save the file.
  - `User_Controls_Apply_To_Cores` sends the stored value to trans (start and client
    connect).
  - `User_Controls_Send_To_Gui` sends it to the client:
    `Gui_send_param(CMD_SET_ALC_MULTIPLIER, value)`, plain opcode 0x23, value 0 or 1.
  - `User_Controls_Update_Init_File` and `Write_User_Controls` write the key.
- `extern.h`: `uint8_t Alc_Multiplier` in `struct User_Record`.
- `main-controller.c`: the `CMD_SET_ALC_MULTIPLIER` case now calls `User_Controls_Process`.
- sdrcore-trans: no change.

Status: built on the Pi 2026-10-09, works (Ron).

Package: `installers/rpi/mscc_1.0.59_arm64.deb` (1.0.58 removed). Same 110 entries and modes
as 1.0.58; only `ms-sdr` and the control version differ.

## For you

1. **Client (WPF and Avalonia):** handle an incoming 0x23 from ms-sdr at connect and set the
   ALC button from it (0 = off, non-zero = on), without sending it back. Today the WPF
   client has no handler for it, so the button is still wrong until this is in. The
   client should not send its own ALC state at connect (Avalonia does today); ms-sdr is the
   owner.
2. **Windows and Ubuntu ms-sdr:** the same change, so all three behave the same.
