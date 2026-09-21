# Status - windows-new-hp

| | |
|--|--|
| **Host** | NEW-HP-LAPTOP (Windows) |
| **Checkout** | `C:\Users\n8vet\OneDrive\Documents\GitHub\mscc-station` |
| **Build** | **cmd-025** |
| **Last command id** | cmd-025 |
| **State** | done |
| **Updated** | 2026-09-21 |

## ACK log

| command id | state | note |
|------------|-------|------|
| cmd-025 | done | DIG-U overlay after FrequencyReported; WPF 9.21.5 |
| cmd-024 | done | idle VFO 0 / no band-mode; last-used gated; WPF 9.21.4 |
| cmd-023 | reviewed | 9.21.3; DIG-U stick blocked by idle 40m/USB default |
| cmd-022 | done | WindowTitle product line |

## Notes

ModeReported USB deferred until freq known. After FrequencyReported + band sync, restore DIG-U overlay from per-band last-used or LAST_HF/LF. No LoadLastUsed from reports. Stew pushes.