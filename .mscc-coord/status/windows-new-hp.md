# Status - windows-new-hp

| | |
|--|--|
| **Host** | NEW-HP-LAPTOP (Windows) |
| **Checkout** | `C:\Users\n8vet\OneDrive\Documents\GitHub\mscc-station` |
| **Build** | **cmd-064** (done recv 3.143) / **cmd-062** (done WPF 10.2.0) / **cmd-038** (WPF pending) |
| **Last command id** | cmd-064 (done) / cmd-062 (done) / cmd-038 (pending) |
| **State** | done |
| **Updated** | 2026-10-04 |

## ACK log

| command id | state | note |
|------------|-------|------|
| cmd-064 | done | Windows SDRcore-recv 3.143 in C:\mscc-net9. DC blocker 0.98 on raw I/Q, spectrum FFT uses the last 4096 samples, cmd-053 pixel notch removed. TX monitor blanking stays. Ubuntu linux/ not edited. Live 800/1600/3200, audio, FT8, and FREQ CAL not run (no radio). Bench of the Hamming FFT on the client dB scale: carrier +3.01, noise +1.51. Same shift at every resolution. Not pushed. |
| cmd-062 | done | WPF 10.2.0 (R10-2-0) and ms-sdr 3.181 in C:\mscc-net9. Save settings sends CMD_SET_PARK_CAL_SETTINGS 0x29. Core 0x29 was already present. SaveLiveToParked is gone. Tooltip is "Ask the host to park live cal for this radio." Unknown major warns and does not park. Live Save and the warning dialog were not run (no radio). Trans stays 3.145. Not pushed. |
| cmd-059 | done | WPF 10.1.2 (R10-1-2), SDRcore-trans 3.145, ms-sdr 3.180 in C:\mscc-net9. Removed uncalled Factory_mirror_live_to_cal and Create_power_cal_file. QRP CAL, AMP CAL, and TX IQ show "When done, press Save settings." Save settings tooltip is "Copy live cal to parked for this radio." Park and swap unchanged. Tabs not opened on the radio. Not pushed. |
| cmd-055 | done | WPF 10.1.1 (R10-1-1), SDRcore-trans 3.144, ms-sdr 3.179 in C:\mscc-net9. Same radio leaves live iq.ini, power_cal.ini, and amplifier_cal.ini. Save settings is under LOG on the right panel and copies those three into cal\<line>\. A different radio stashes the old line, then loads parked or factory. A missing power file is the per-line factory table. No live-to-park mirror on IQ save. Freq and RX IQ stay unparked. Stew: cal behavior works, and Save settings under LOG looks good. Not pushed. |
| cmd-048 | done | Windows SDRcore-trans 3.143 and ms-sdr 3.178 in C:\mscc-net9. Trans owns power_cal.ini and amplifier_cal.ini. Slider steps update RAM and the file is written about 0.5 s later. ms-sdr only reads and forwards. amplifier.ini is no longer created. QRP reset 0xAA is ignored. The live-to-park mirror on QRP save is gone. Park and Save settings stay in cmd-055. Stew: tests 1-5 work. Tests 6-7 not run. Not pushed. |
| cmd-054 | done | WPF 9.30.4 (R9-30-4) in C:\mscc-net9. S/W label is SPECTRUM RESOLUTION. Tooltip says remote spectrum stutters. Resolution list and PAN_RESOLUTION ini unchanged. Live window not opened. Not pushed. |
| cmd-053 | done | Windows SDRcore-recv 3.142 in C:\mscc-net9. The -12 kHz spur notch is 6 FFT bins each side and the fill is the smoothed neighbour texture from the Pi. Ubuntu linux/ waits for the next pass. Live spectrum smoke not run. Not pushed. |
| cmd-057 | done | WPF 9.30.2 (R9-30-2) in C:\mscc-net9. CW tab PHONES checkbox removed. POTENTIA / QSK remains, still sends, tooltip "Amplifier PIN diode T/R switching". WPF no longer sends 0x70. Opcode constant kept. SetCwPhonesAsync stays in Core because Avalonia still calls it. Live hover and QSK toggle not run. Not pushed. |
| cmd-056 | done | Windows ms-sdr 3.177 in C:\mscc-net9. A second STOP while the abort drain is pending logs "nothing new owed, drain still pending" and does not finish the drain. Idle STOP still sends drain done. Live radio smoke not run. Not pushed. |
| cmd-046 | done | WPF 9.27.0 and SDRcore-trans 3.142 in C:\mscc-net9. TUNE power drives TUNE only. USB and LSB, including DIG-U and digital audio, use the SSB bank. Client version is 9.27.0 because the auto-bump is month.day.iteration and this build is on the 27th. Live radio smoke not run. Not pushed. |
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

cmd-064: done. Windows recv 3.143. The -12 kHz spur fix is in the DSP. The pixel notch is gone. cmd-038 still pending.
cmd-062: done. WPF 10.2.0, ms-sdr 3.181. Host park is opcode 0x29. The client no longer copies cal files.
cmd-059: WPF 10.1.2, trans 3.145, ms-sdr 3.180. Short on-tab Save settings line. Dead cal helpers removed.