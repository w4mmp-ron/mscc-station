# Status - windows-new-hp

| | |
|--|--|
| **Host** | NEW-HP-LAPTOP (Windows) |
| **Checkout** | `C:\Users\n8vet\OneDrive\Documents\GitHub\mscc-station` |
| **Build** | **cmd-045b** (WPF + ms-sdr done) / **cmd-038** (WPF pending) |
| **Last command id** | cmd-045b (done) / cmd-038 (WPF pending) |
| **State** | done |
| **Updated** | 2026-09-26 |

## ACK log

| command id | state | note |
|------------|-------|------|
| cmd-045b | done | WPF 9.26.6 and ms-sdr 3.176 in C:\mscc-net9. STOP matches the other FREQ CAL buttons. Abort drain uses opcode 0x1F: 1 = drain done, 2 = start refused. A second finish is sent only while the sweep is still stepping. Live radio smoke not run. Not pushed. |
| cmd-045a | done | WPF 9.26.5 and ms-sdr 3.175 in C:\mscc-net9. FREQ CAL STOP uses CMD_SET_CAL_ABORT 0x1F. Late sweep replies are dropped so PPM is not written. Live radio smoke not run. Not pushed. |
| cmd-045 | done | WPF 9.26.4 in C:\mscc-net9. VFO B stored in MSCC_LastUsed_VFOB.ini (VFOB_FREQ, VFOB_MODE, VFOB_BAND). Close during AUTO or CHECK restores mode and does not abort the cal. QRP, Full Power, and AMP share AmpOn. Radio-model button removed. DIG-U slider uses Tune power. ms-sdr unchanged at 3.174. Not pushed. |
| cmd-044 | done | WPF 9.26.3 + ms-sdr 3.174 in C:\mscc-net9. FREQ CAL success text: AUTO "Was N Hz off, now corrected. Run CHECK."; CHECK "Error now N Hz" plus " (good)" when abs(N) <= 5. Client only. Not pushed. |
| cmd-042 | done | Windows only. SDRcore-recv 0xD1 case 5=1400 / case 6=1000, recv 3.141. mscc-recv.exe in Release/windows-wpf and C:\mscc-net9. New exe has the 1400 Hz constant (previous exe had none). Live listen not run: MSCC was not running, radio not started. Ubuntu/Pi parts wait until Stew pushes. |
| cmd-041 | done | WPF 9.25.0. DIG-U Hi idx 5=1.4k / 6=1.0k. Other modes stay 0..4. 0xDD idx>4 ignored. Deployed to C:\mscc-net9. Not pushed. |
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

cmd-045b: WPF 9.26.6 and ms-sdr 3.176. FREQ CAL STOP uses the same button face as LOOSE, AUTO, MANUAL, CHECK, and RESET. Those buttons stay on the default face across Setup colour schemes. After STOP the server drops late sweep replies and refuses a new AUTO or CHECK until the finish reply or 35 s, then sends 0x1F value 1. Value 2 means the start was refused. A finish command goes to recv only while the sweep is still stepping. Closing during AUTO or CHECK aborts the run, then restores the mode. cmd-038 still pending.
