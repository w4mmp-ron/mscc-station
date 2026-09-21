# Status - windows-new-hp

| | |
|--|--|
| **Host** | NEW-HP-LAPTOP (Windows) |
| **Checkout** | `C:\Users\n8vet\OneDrive\Documents\GitHub\mscc-station` |
| **Build** | **cmd-023** |
| **Last command id** | cmd-023 |
| **State** | done |
| **Updated** | 2026-09-21 |

## ACK log

| command id | state | note |
|------------|-------|------|
| cmd-023 | done | LAST 14.074/474.2; DIG-U stick; FW ask; FreqCal reset; WPF 9.21.3 |
| cmd-022 | done | WindowTitle product line; verified |
| cmd-021 | done | last-used sanity + geminus IQ 1/4 |
| cmd-020 | done | cal/<line>/ cache |

## Notes

A) DefaultLastHfFreq=14074000 DefaultLastLfFreq=474200 (existing INI keys kept).
B) ModeReported USB keeps DIG-U if UI or last-used is DIG-U (radio RF is USB). LoadLastUsed still applies DIG-U audio D.
C) After Connect, 2.5s: if FW or Core still "--", prompt once; Yes=0xB2/0xB3/0xFE; No=keep last session.
D) Freq Cal Auto/Check/Reset zeros progress + status before a new run.
Stew pushes.