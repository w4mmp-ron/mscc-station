# Status — rpi

| | |
|--|--|
| **Host** | raspberrypi |
| **Checkout** | `/home/pi/src/mscc-station` |
| **Build** | Grok Build |
| **Last command id** | cmd-039 |
| **State** | done |
| **Updated** | 2026-09-23 |

## ACK log

| command id | state | note |
|------------|-------|------|
| cmd-039 | done | Diagnostic EVENT logging in live `$HOME/mscc/sdrcore-trans` (2026-09-23 14:21). Fill unchanged. cmd-034 on hold. Kit stays 1.0.44. |
| cmd-032 | done | Debs in `installers/rpi/` and `rpi/Rpi-installers/` |
| cmd-031 | done | trans 17:56 stream reset |

## Notes

- Edit: `rpi/SDRcore-trans-linux/sources/remote_mic.c` only. `linux/` not touched.
- `strings $HOME/mscc/sdrcore-trans | grep 'remote_mic EVENT'` matches.
- EVENT body has `mono_ms=` (CLOCK_MONOTONIC) and `t=HH:MM:SS.mmmZ` (UTC). Repeat lines rate-limited ~100 ms. UDP gap threshold 50 ms (5× a 480-frame pack).
- Kinds: `hold_last`, `overflow dropped=`, `adaptive step=`, `low_occ` (occ < RING/8), `udp_gap`.
- Periodic `remote_mic: pkt ok=` kept; line also has `overflow=`, `step=`, `mono_ms=`.
- Restart of `sdrcore-trans` exited: no Multus I/Q USB device (`lsusb` shows Sound Blaster only). `ms-sdr` and `sdrcore-recv` still running. Power the transceiver and start `sdrcore-trans` before the JT65 smoke.
