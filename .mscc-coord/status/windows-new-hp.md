# Status - windows-new-hp

| | |
|--|--|
| **Host** | NEW-HP-LAPTOP (Windows) |
| **Checkout** | `C:\Users\n8vet\OneDrive\Documents\GitHub\mscc-station` |
| **Build** | **cmd-026** |
| **Last command id** | cmd-026 |
| **State** | done |
| **Updated** | 2026-09-21 |

## ACK log

| command id | state | note |
|------------|-------|------|
| cmd-026 | done | band last-used wins over LAST_HF; WPF 9.21.6 |
| cmd-025 | done | DIG-U overlay after FrequencyReported; WPF 9.21.5 — LAST_HF could override USB |
| cmd-024 | done | idle VFO 0 / no band-mode; last-used gated; WPF 9.21.4 |
| cmd-023 | reviewed | 9.21.3; DIG-U stick blocked by idle 40m/USB default |
| cmd-022 | done | WindowTitle product line |

## Notes

Per-band last-used MODE decides DIG-U overlay when present. LAST_HF/LF only if band unknown or mode empty. Live mode change updates LAST_HF/LF. USB 0xB7 still deferred until freq known. Stew pushes.
