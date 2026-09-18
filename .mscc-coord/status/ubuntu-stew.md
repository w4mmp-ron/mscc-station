# Status — ubuntu-stew

| | |
|--|--|
| **Host** | stew-HP-Notebook |
| **Checkout** | `/home/stew/Documents/GitHub/mscc-station` |
| **Build** | cmd-008 |
| **Last command id** | cmd-008 |
| **State** | done |
| **Updated** | 2026-09-18 |

## ACK log

| command id | state | note |
|------------|-------|------|
| cmd-008 | done | `mscc-ui_0.6.56_amd64.deb` in `installers/linux/`. rpi `mscc-ui_0.6.56_arm64.deb` left in place. |

## Notes

- Forced fresh `linux-x64` publish (0.6.56 in DLL). `dpkg-deb -I`: Version 0.6.56, Architecture **amd64**.
- `installers/linux/`: `mscc-ui_0.6.56_amd64.deb`, `mscc_1.0.43_amd64.deb`, `mscc-portaudio_19.8.2_amd64.deb`, `mscc-init-gui_1.0.13_all.deb`.
- `installers/rpi/mscc-ui_0.6.56_arm64.deb` unchanged (Sep 18 14:38).
- Packaging template control still Architecture arm64 (Pi); amd64 script sed's it at pack time.
- Did not install `/opt` this pass. Stew pushes.

## Blocked

(none)
