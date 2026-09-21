# Status - windows-new-hp

| | |
|--|--|
| **Host** | NEW-HP-LAPTOP (Windows) |
| **Checkout** | `C:\Users\n8vet\OneDrive\Documents\GitHub\mscc-station` |
| **Build** | **cmd-026** |
| **Last command id** | cmd-026 |
| **State** | pending |
| **Updated** | 2026-09-21 |

## ACK log

| command id | state | note |
|------------|-------|------|
| cmd-026 | pending | band last-used wins over LAST_HF for DIG-U overlay |
| cmd-025 | done | DIG-U overlay after FrequencyReported; WPF 9.21.5 — LAST_HF could override USB |
| cmd-024 | done | idle VFO 0 / no band-mode; last-used gated; WPF 9.21.4 |
| cmd-023 | reviewed | 9.21.3; DIG-U stick blocked by idle 40m/USB default |
| cmd-022 | done | WindowTitle product line |

## Notes

cmd-026: DIG-U keep/restore uses per-band mode when present; LAST_HF/LF only if band mode empty. Also sync LAST_* on live mode change. Stew pushes.
