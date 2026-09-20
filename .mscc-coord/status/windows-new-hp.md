# Status — windows-new-hp

| | |
|--|--|
| **Host** | NEW-HP-LAPTOP (Windows) |
| **Checkout** | `C:\Users\n8vet\OneDrive\Documents\GitHub\mscc-station` |
| **Build** | **cmd-017b** |
| **Last command id** | cmd-017b |
| **State** | done |
| **Updated** | 2026-09-20 |

## ACK log

| command id | state | note |
|------------|-------|------|
| cmd-017b | done | WPF 9.20.4 FrequencyReported → same RetuneIfIllegal; no RememberLast on report |
| cmd-017 | done | Full HF/LF S/W banks + LAST_HF/LF; FW-only retune |
| cmd-016 | done | S/W follows band; 2200M/630M last-used kept |

## Notes

FrequencyReportedForConnectSafety → MainWindow. Skips if FirmwareVersion "--". Same RetuneIfIllegalForRadioPersonality (1.8 MHz / LAST_*). No RememberLast on server freq report.

WPF **9.20.4** in `C:\mscc-net9`.
