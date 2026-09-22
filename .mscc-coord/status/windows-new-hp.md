# Status - windows-new-hp

| | |
|--|--|
| **Host** | NEW-HP-LAPTOP (Windows) |
| **Checkout** | `C:\Users\n8vet\OneDrive\Documents\GitHub\mscc-station` |
| **Build** | **cmd-033** |
| **Last command id** | cmd-033 |
| **State** | pending |
| **Updated** | 2026-09-22 |

## ACK log

| command id | state | note |
|------------|-------|------|
| cmd-033 | pending | 0xBC: TxSetByServer only (no PttOn/TuneMode); Remote vs local Phones/Digital independence; WPF 9.21.8 |
| cmd-030 | done | Remote Digital mic slider; REMOTE_DIGI_MIC_VOL default 100; WPF 9.21.7 |
| cmd-026 | done | band last-used wins over LAST_HF; WPF 9.21.6 |
| cmd-025 | done | DIG-U overlay after FrequencyReported; WPF 9.21.5 |
| cmd-024 | done | idle VFO 0 / no band-mode; last-used gated; WPF 9.21.4 |
| cmd-022 | done | WindowTitle product line |

## Notes

Ron testing bugs.docx items 2+5. Digi TX on Pi must not light client TUN/PTT. Remote must not overload local Phones/Digital.
