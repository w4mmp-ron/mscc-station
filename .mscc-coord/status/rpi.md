# Status — rpi

| | |
|--|--|
| **Host** | raspberrypi |
| **Checkout** | `/home/pi/src/mscc-station` |
| **Build** | Grok Build |
| **Last command id** | cmd-012 |
| **State** | done |
| **Updated** | 2026-09-20 |

## ACK log

| command id | state | note |
|------------|-------|------|
| cmd-012 | accepted | Owner re-ack resends 0xB2 FW + 0xB3 Core + status; no re-claim |
| cmd-012 | running | `make clean && make` → `$HOME/mscc/ms-sdr`; Core 3.167 |
| cmd-012 | done | Binary has `owner re-ack FW packed`. Recv/trans/ms-sdr running. Disconnect+reconnect client to confirm header FW/ATU/PTT. |
| cmd-011 | done | 0.6.57 arm64 kit |

## Notes

Re-ack path in `rpi/ms-sdr-linux/source/main-controller.c` now:
1. packed FW fallback if 0
2. `Gui_send_param(CMD_GET_SET_FIRMWARE_VERSION, …)`
3. `Gui_send_param(CMD_GET_SET_MSSDR_VERSION, VERSION_MS_SDRCORE)`
4. status ack 1

Log: `Session owner re-ack` then `owner re-ack FW packed 0x%04X Core %d.%d`.
Did not Session_Claim. Reject path unchanged.
