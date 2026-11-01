# For Ron: QRP save step and 0x2D calibration status, done on Windows (ms-sdr 3.189, trans 3.154, client 11.1.0)

From Stew, 2026-11-01. This implements what we agreed in `NOTE-FOR-STEW-ALC-089-TWO-TONE-2026-11-01.md`.
Built on Shack, and tested by Stew on several radios. A fully calibrated radio showed green QRP CAL
lamps for its bands; a blank one showed all white. Your side is the Pi ms-sdr and trans. Everything
below is plain C with no Windows-only parts.

## Wire protocol (for the Pi ms-sdr / trans)

| What | Direction | Data |
|---|---|---|
| `CMD_SET_BAND_POWER_POWER` 0xA2 | client → ms-sdr → trans | low byte = step. **Bit 15 (`POWER_CAL_SAVE_FLAG` 0x8000) = save.** Without it the step is live only. |
| `CMD_SET_CAL_STATUS` **0x2D** | ms-sdr → client | int32 = kind (1 RX IQ, 2 TX IQ, 3 power) \| flags << 8 (bit 0 = the radio has a store) \| uint16 mask << 16 (bit N = band N, MSCC order: 0 = 10 m ... 9 = 160 m, 10 = 630 m, 11 = 2200 m). One message per kind, three in a row. |

`0x2E` and `0x2F` are still free in all six server trees and the WPF client. 2 TONE will use one
of them; I'll name it in that note.

## Windows ms-sdr 3.189 (`power_calibration.c`, `usbavrcmd.h`, `extern.h`, `main-controller.c`)

- **0xA2 handler:** `save = s_opcode_data & POWER_CAL_SAVE_FLAG`. It always passes the value on to
  trans (with the flag if set). `Radio_cal_store_power()` runs **only when save is set**.
- **`Radio_cal_load_power()`:** the startup copy from the radio to trans is now sent **with** the
  flag, so trans still writes `power_cal.ini`.
- **Mask cache:** `radio_cal_masks[6]` (same layout as type 5) and `radio_cal_store` (-1 = not read
  yet, 0 = no store, 1 = store). Both startup loads fill them. A failed type 5 read sets 0.
- **`Radio_cal_send_status()`:** sends the three 0x2D messages. It does nothing while
  `radio_cal_store` is -1. Called from:
  - the end of `Radio_cal_load_iq()` (also when there is no store, so the client knows);
  - `Session_Claim()` (client connects after the startup read);
  - `Radio_cal_save_tick()` after any write to the radio: it rereads type 5 first
    (`Radio_cal_read_masks()`).

## Windows trans 3.154 (`udp_thread.c`)

- `Set_QRP_calibration(index, value, save)`: the RAM value and the drive recompute as before. The
  `power_cal.ini` save countdown starts **only when save is set**.
- The 0xA2 case reads `save = buf[2] & 0x80` (bit 15 of the data). Older ms-sdr never sets it, so
  with an old ms-sdr every step still saves, as before.
- **Your Pi trans already has the 0.89 ALC.** Only this 0xA2 change is needed there.

## Client 11.1.0 (WPF; Avalonia gets the same in its catch-up)

- **QRP CAL:** turning CALIBRATE off asks "SAVE THIS CALIBRATION?" with the step and band.
  - **Yes:** 0xA2 with 0x8000.
  - **No:** the previous value, live (nothing saved).
  - **Reject is gone.** Leaving the tab during CALIBRATE also restores the previous value, live.
- **Lamps follow the power mask** (kind 3) as soon as a 0x2D arrives with the store flag. Without
  a store, and after Stop, they come from `client-settings.ini` as before.
- **RX / TX IQ:** APPLY is renamed SAVE (button, prompt, status lines).
- **Text fixes from your guide review:**
  - Start tooltip: only the operator speaker is needed; the COM port is optional.
  - QRP CAL and TX IQ no longer say "press Save settings".
  - Save settings tooltip: AMP CAL and radios without a store.
  - POTENTIA / QSK tooltip: "Potentia 50/100 attached: amp line and PA keyed together".
  - "Full Power" renamed "AMP".
- Release note needed: **ms-sdr 3.189 needs client 11.1.0.** An old client never sends the save
  flag, so its QRP calibration would not be saved (option (b), as you chose).

## Found on the way

- **RX IQ "APPLY ALL"** (0x55 RX + 0x8D, copy the offset to every band) writes only the PC files,
  not the radio. Bands the radio holds win at the next start. It is unchanged for now; its tooltip
  says so. Should it write every band into the radio, or go away?
- **RX IQ and TX IQ masks** arrive in the client too (logged). Lamps on those tabs can come later.

Next on the Windows side: the per-kind resets (they need your type 6 handler with the kind in
wIndex, in the PSoC lines, Geminus and the Black Pill), then 2 TONE.
