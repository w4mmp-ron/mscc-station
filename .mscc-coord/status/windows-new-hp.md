# Status - windows-new-hp

| | |
|--|--|
| **Host** | NEW-HP-LAPTOP (Windows) |
| **Checkout** | `C:\Users\n8vet\OneDrive\Documents\GitHub\mscc-station` |
| **Build** | **cmd-037** |
| **Last command id** | cmd-037 |
| **State** | pending |
| **Updated** | 2026-09-23 |

## ACK log

| command id | state | note |
|------------|-------|------|
| cmd-037 | pending | Orders + brief: 60 ms RemoteMicSender prefill cushion. Awaiting Build ACK. |
| cmd-036 | done | WPF 9.23.0 paced mic send. Commit d26c7a7. Stew: huge improvement. FT8 CQ ~1-2 bumps; JT9/JT65/WSPR intermittent, JT65 widest. Not pushed. |
| cmd-035 | done | Local DIGITAL/OPERATOR 35 ms key-up gate. Trans 141. Commit 47e336c. Stew: one bump on the first TX, then gone across MSCC restarts and a radio power cycle. Not pushed. |
| cmd-033 | done | 0xBC ownership only; Remote vs local audio; WPF 9.22.0 (calendar bump from 9.21.7) |
| cmd-030 | done | Remote Digital mic slider; REMOTE_DIGI_MIC_VOL default 100; WPF 9.21.7 |
| cmd-026 | done | band last-used wins over LAST_HF; WPF 9.21.6 |
| cmd-025 | done | DIG-U overlay after FrequencyReported; WPF 9.21.5 |
| cmd-024 | done | idle VFO 0 / no band-mode; last-used gated; WPF 9.21.4 |
| cmd-022 | done | WindowTitle product line |

## Notes

cmd-037: WPF client only (`MSCC.Wpf/RemoteAudio/RemoteMicSender.cs`). Brief: `.mscc-coord/briefs/cmd-037.md`. cmd-034 Pi on hold. cmd-035 local gate stays as-is. cmd-036 done (`d26c7a7`).