# Note for Ron — Pi ms-sdr drops 800-bin spectrum

2026-10-04, Stew / Build Commander

Hi Ron,

The three spectrum widths stay (800 / 1600 / 3200). Ubuntu ms-sdr is fixed locally. The Pi still has the bug. Stew saw it today by remoting to the Pi.

## What happens

Asking for 800 bins sends pan command 0x5F with value 0. In `rpi/ms-sdr-linux/source/user_controls.c`, `CMD_GET_SET_PANADAPTER_REFRESH` treats any value below 1 as an invalid refresh rate and rewrites it to 6 before forwarding to sdrcore-recv.

Values 1 and 2 pass, so 1600 and 3200 stick. Going back to 800 does not. Recv stays at the wider width. The client then draws the first 800 bins of that wider span across the whole 72 kHz window, which is the lower half. A signal that was centered jumps to the right edge. The frequency readout does not change. The new DC null (12 kHz below the VFO) is not the cause. It only made the shift easy to see.

Ubuntu log proof, 2026-10-04: `ms-sdr.log` lines "CMD_GET_SET_PANADAPTER_REFRESH: 0 invalid — using 6" at 3:30:55 PM and 3:31:37 PM ET. Recv logged refresh blocks 6 then 3, and never "PAN RESOLUTION CHANGED → 800".

## Fix

Same change Stew just proved on Ubuntu `linux/ms-sdr-linux/source/user_controls.c`:

- Let 0, 1, and 2 through to sdrcore-recv unchanged (800 / 1600 / 3200 bins).
- Rewrite to 6 only when the value is not 0, 1, or 2 and not a real refresh rate 3 through 10.

Please port that to `rpi/ms-sdr-linux` and ship it in the next Pi mscc package. Recv and the UI do not need a change for this.

Thanks,
Stew
