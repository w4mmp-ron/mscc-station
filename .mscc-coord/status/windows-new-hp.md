# Status — windows-new-hp

| | |
|--|--|
| **Host** | NEW-HP-LAPTOP (Windows) |
| **Checkout** | `C:\Users\n8vet\OneDrive\Documents\GitHub\mscc-station` |
| **Build** | **cmd-018** |
| **Last command id** | cmd-018 |
| **State** | done |
| **Updated** | 2026-09-20 |

## ACK log

| command id | state | note |
|------------|-------|------|
| cmd-018 | done | factory/ trees + Windows ms-sdr seed by FW major; QRP−3; no PIN |
| cmd-017b | done | Connect-safety FrequencyReported |
| cmd-017 | done | HF/LF S/W banks + LAST_HF/LF |

## Notes

`factory/iq|freq|power/<line>/` for six lines. PIN nowhere in factory/. Ship QRP = measured−3. HF-only LF IQ/QRP = 0. Geminus LF IQ −31/−34.

Windows `Factory_seed_live_inis()` after srGetVersion: copy from `exe\factory\` into `%LocalAppData%\MSCC-NET9\` **only if live missing**. Map: 1 proficio-legacy, 2 geminus-mkii, 3/4 proficio-mkii, 5 geminus-legacy, 6 ultimus-legacy, 7/8 ultimus-mkii, else proficio-mkii.

ms-sdr 171 + `C:\mscc-net9\factory`. Linux/Pi seed later.
