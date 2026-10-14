# For Stew — CW + TUN left the rig stuck in TX (cause in the radio firmware)

From Ron, 2026-10-14. For your information; the Pi has a workaround.

## What Ron saw

In CW mode, pressing TUN and then releasing it left the rig transmitting. It only came out
of TX when another mode (USB, LSB...) was picked. In USB, TUN worked normally.

## Cause

It is in the radio firmware (`radio-psoc-firmware/Proficio-MKII-PTT/Proficio-MKII-PTT.cydsn/main.c`,
main loop, about lines 159-173):

- The firmware only runs its TX handler (`TX_Main`, which acts on the PTT command from the
  host) when the mode is **not** CW. In CW, TX is expected to come from the key or paddles.
- On TUN release the client sends "mode CW" first and "TUNE off" second (the same order as
  in USB). So the radio is switched to CW while still transmitting, and the PTT off that
  follows is never acted on. The PA stays on.
- Leaving CW runs a block that turns the PA off and clears the TX request, which is why
  picking USB or LSB freed it.

The ms-sdr log on the Pi showed exactly this order, in both USB and CW.

## Workaround on the Pi (built and tested by Ron)

`rpi/ms-sdr-linux/source/main-controller.c`, at `CMD_SET_MAIN_MODE`, before `ModeChanged(mode)`:

```c
if (G_Tune && mode != 'T') {
    Tune_button(FALSE, FALSE);
    SDRcore_trans_send_param(CMD_SET_TX_ON, FALSE);
    SDRcore_recv_send_param(CMD_SET_TX_ON, FALSE);
    Sleep(20);
}
```

So TX is dropped while the radio is still in TUNE mode, and only then does the mode change
to CW. The client's own TUNE off arrives afterwards and does no harm. With this, CW TUN
drops out of TX normally.

## Things to know

- **Windows and Ubuntu ms-sdr probably have the same problem**, since the cause is in the
  firmware and the client sends the same order. The same few lines would fix it there.
  Ron has not tested it on those platforms.
- **The real fix belongs in the firmware.** The ms-sdr change only covers TUNE. A mode
  change to CW during any other transmit (PTT, CAT, a digital program) would still leave the
  PA on. In the firmware, entering CW could turn the PA off and clear the TX request, the
  same way leaving CW already does. That would cover every path and every host, but each
  rig would need a reflash. Nothing has been changed in the firmware.
- A client-side alternative would be to send TUNE off before the mode change. Your choice
  whether that is worth doing as well.
