# Status — windows-new-hp

| | |
|--|--|
| **Host** | NEW-HP-LAPTOP (Windows) |
| **Checkout** | `C:\Users\n8vet\OneDrive\Documents\GitHub\mscc-station` |
| **Build** | waiting — **cmd-019** |
| **Last command id** | cmd-019 |
| **State** | pending |
| **Updated** | 2026-09-20 |

## ACK log

| command id | state | note |
|------------|-------|------|
| cmd-019 | pending | Reset Cal + Settings Reset → FW-major factory |
| cmd-018 | done | factory trees + first-boot seed |

## Notes

Settings Reset currently copies generic init-files iq/freq/power and blocks Factory_seed.
FREQ CAL Reset still Create_PPM from PCB. TX IQ Reset is SDRcore 0x8D defaults, not factory/iq.