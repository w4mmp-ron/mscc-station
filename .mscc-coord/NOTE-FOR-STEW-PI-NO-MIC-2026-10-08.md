# For Stew — Pi: cmd-067 B1 (trans starts with no operator mic) is ported

From Ron, 2026-10-08. Nothing for you to do; this is for your records.

The Pi trans had the same gap as Ubuntu before cmd-067: with no operator mic it logged
"NO MICROPHONE DEVICE FOUND" and exited.

- **What was ported:** your Ubuntu hunks from `22f6f5b`, applied unchanged to
  `rpi/SDRcore-trans-linux/sources/main.c` and `udp_thread.c`. The Pi code in both places was
  the same as Ubuntu's before your change (`udp_thread.c` 4 lines higher).
  - `main.c`: no operator mic = log "No operator mic set: TX voice off", open the I/Q output
    only (`manage_stream(1, -1, 2)`); the digital-mic fallback copies the operator record only
    when the operator index is valid.
  - `udp_thread.c`: `CMD_SET_AUDIO_DEVICE` with no operator mic goes to output-only I/Q
    (DIGITAL, OPERATOR, REMOTE) instead of "abort switch".
- **Version:** Pi sdrcore-trans `VERSION_MINOR` 140 -> 142, same as Ubuntu.
- **Not ported, not needed on the Pi:** recv and ms-sdr (no change in your Part B either);
  B2 / B2b are Avalonia only.
- **Status:** built on the Pi 2026-10-08, works (Ron). Commit `afc9af9`.
- **Package:** `installers/rpi/mscc_1.0.57_arm64.deb` (1.0.56 removed). Same 110 files and
  modes as 1.0.56; only `sdrcore-trans`, the control version and `factory/README.md` differ.
