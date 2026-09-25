# Status - windows-new-hp

| | |
|--|--|
| **Host** | NEW-HP-LAPTOP (Windows) |
| **Checkout** | `C:\Users\n8vet\OneDrive\Documents\GitHub\mscc-station` |
| **Build** | **cmd-041** (WPF) + **cmd-042** (Windows recv) / **cmd-038** (WPF) |
| **Last command id** | cmd-042 (orders) / cmd-041 (orders) / cmd-038 (WPF pending) |
| **State** | pending |
| **Updated** | 2026-09-25 |

## ACK log

| command id | state | note |
|------------|-------|------|
| cmd-042 | orders | Windows part: SDRcore-recv 0xD1 case 5=1400 / 6=1000, recv 3.141, mscc-recv.exe -> Release/windows-wpf + C:\mscc-net9. Build with cmd-041. Ubuntu/Pi parts follow after Stew pushes. |
| cmd-041 | orders | WPF DIG-U Hi adds 1.4k/1.0k (idx 5/6) + 0xDD default-echo guard. Stew chose A (paired with cmd-042). |
| cmd-040 | orders | Avalonia remote-audio parity orders + brief authored here (canonical yaml). Build target: ubuntu-stew then rpi. No Avalonia code on this host for 040. |
| cmd-038 | pending | Orders + brief: finer Mic TX EVENT logging (diagnostic only). Awaiting Build ACK. |
| cmd-037 | done | WPF 9.23.1, 60 ms mic cushion. Commit 5f39a16. Stew: looks great. One JT65 bump in 60 s. WPF drops=0 underruns=0. Not pushed. |
| cmd-036 | done | WPF 9.23.0 paced mic send. Commit d26c7a7. Stew: huge improvement. FT8 CQ ~1-2 bumps; JT9/JT65/WSPR intermittent, JT65 widest. Not pushed. |
| cmd-035 | done | Local DIGITAL/OPERATOR 35 ms key-up gate. Trans 141. Commit 47e336c. Stew: one bump on the first TX, then gone across MSCC restarts and a radio power cycle. Not pushed. |
| cmd-033 | done | 0xBC ownership only; Remote vs local audio; WPF 9.22.0 (calendar bump from 9.21.7) |
| cmd-030 | done | Remote Digital mic slider; REMOTE_DIGI_MIC_VOL default 100; WPF 9.21.7 |
| cmd-026 | done | band last-used wins over LAST_HF; WPF 9.21.6 |
| cmd-025 | done | DIG-U overlay after FrequencyReported; WPF 9.21.5 |
| cmd-024 | done | idle VFO 0 / no band-mode; last-used gated; WPF 9.21.4 |
| cmd-022 | done | WindowTitle product line |

## Notes

cmd-040 orders: `.mscc-coord/COMMANDS.yaml` + `briefs/cmd-040.md` + OVERSEER. Build on ubuntu-stew (then rpi kit). cmd-038 still pending WPF logging. cmd-034 Pi on hold.
