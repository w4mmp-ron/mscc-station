# Status - windows-new-hp

| | |
|--|--|
| **Host** | NEW-HP-LAPTOP (Windows) |
| **Checkout** | `C:\Users\n8vet\OneDrive\Documents\GitHub\mscc-station` |
| **Build** | **cmd-033** |
| **Last command id** | cmd-033 |
| **State** | done |
| **Updated** | 2026-09-22 |

## ACK log

| command id | state | note |
|------------|-------|------|
| cmd-033 | done | 0xBC ownership only; Remote vs local audio; WPF 9.22.0 (calendar bump from 9.21.7) |
| cmd-030 | done | Remote Digital mic slider; REMOTE_DIGI_MIC_VOL default 100; WPF 9.21.7 |
| cmd-026 | done | band last-used wins over LAST_HF; WPF 9.21.6 |
| cmd-025 | done | DIG-U overlay after FrequencyReported; WPF 9.21.5 |
| cmd-024 | done | idle VFO 0 / no band-mode; last-used gated; WPF 9.21.4 |
| cmd-022 | done | WindowTitle product line |

## Notes

A) 0xBC sets TxSetByServer only — no PttOn/TuneMode, no TUN/TX_ON/tune-power from that path.
B) RemoteDigitalAudio independent of local IsDigitalAudio. Main Phones/Digital dim while Remote; click exits Remote and selects that local path.
Stew pushes. ClientVersion 9.22.0 (month.day; cmd asked 9.21.8).
