# Status - windows-new-hp

| | |
|--|--|
| **Host** | NEW-HP-LAPTOP (Windows) |
| **Checkout** | `C:\Users\n8vet\OneDrive\Documents\GitHub\mscc-station` |
| **Build** | **cmd-076** (client 10.13.2, WPF only, not committed) / **cmd-038** (WPF pending) |
| **Last command id** | cmd-076 (running, stopped before commit) / cmd-038 (pending) |
| **State** | code ready, stopped before commit |
| **Updated** | 2026-10-13 |

## ACK log

| command id | state | note |
|------------|-------|------|
| cmd-076 | running | WPF only. Client 10.13.2 (FileVersion 10.13.2.0, 222208 bytes, 2026-10-13 13:51:30) in C:\mscc-net9. RX IQ RESET ALL is labelled APPLY ALL. START shows ACTIVE while a session runs. Cancelling APPLY or APPLY ALL sets a status line. Servers not rebuilt (trans 3.147, ms-sdr 3.183, recv 3.147). Smoke 1-5 not run (no radio). Not committed. No push. |
| cmd-075 | running | WPF only. Client 10.13.1 (FileVersion 10.13.1.0, 222208 bytes, 2026-10-13 12:16:36) in C:\mscc-net9. S/W SMOOTHING sends 0xDA (default 3) and frame dropping is gone. Waterfall default is Falling, once. BASELINE and waterfall levels are per band. Servers not rebuilt (trans 3.147, ms-sdr 3.183, recv 3.147). Smoke 1-6 not run (no radio). Not committed. No push. |
| cmd-074 | running | Part A. WPF TX ALC button removed. Trans 3.147 ignores ALC-off (244736 bytes, 2026-10-13 10:39:56, same hash as Release). ms-sdr 3.183 saves volume and mic gain a few seconds after the last step (341504 bytes, 2026-10-13 10:40:08, same hash). Client 10.13.0 (FileVersion 10.13.0.0) in C:\mscc-net9. Recv 3.147 unchanged (243200 bytes, 2026-10-11 15:03:23). Smoke 1-5 not run (no radio). Not committed. No push. |
| cmd-073b | done | Windows recv 3.147 in C:\mscc-net9 (243200 bytes, 2026-10-11 15:03:23, same hash as Release). Average-power S-meter, nearest dB. WPF needle is fractional S units; readout stays whole units. Client 10.11.3 (FileVersion 10.11.3.0). trans 3.146 and ms-sdr 3.182 unchanged (2026-10-08). Smoke pass (Stew 2026-10-11). Committed. |
| cmd-071 | done | Windows recv 3.146 in C:\mscc-net9 (242176 bytes, 2026-10-11 12:31:38). panadapter.c copied from the Pi. LO-point levelling and DC_BLOCK_A 0.9999. Client 10.11.2 is a version-only Release (FileVersion 10.11.2.0). trans 3.146 and ms-sdr 3.182 unchanged (2026-10-08). Smoke pass (Stew 2026-10-11); LO-point levelling pumps near centre, paused - see NOTE-FOR-RON-LO-POINT-LEVELLING-2026-10-11.md. Committed. |
| cmd-070 | done | WPF only. Client 10.11.1 in C:\mscc-net9. S-meter readout click toggles S units and dBm (SMETER_DBM). S units round to the nearest unit, ties to the higher unit. Spectrum centre -83.6, GRID MIN -155, waterfall shifted about -9. Servers not rebuilt (recv 3.145, trans 3.146, ms-sdr 3.182). Smoke 1-7 pass (Stew 2026-10-11, incl. Geminus LF/MF). Committed. |
| cmd-069 | done | Windows recv 3.145 and WPF 10.9.1 in C:\mscc-net9. Spectrum averaging no longer adds gain. Client reads true dB with centre -75. First start resets dB CAL, GRID MIN, and waterfall once (SPECTRUM_DB_LAW=20). cmd-068 max-per-pixel and hidden resolution selector stay. TG step test pass (Stew 2026-10-11, trim -8.6 at -75 centre -> centre -83.6 in cmd-070). Committed. |
| cmd-068 | done | WPF only. Spectrum and waterfall draw the max of the points each pixel covers. Resolution fixed at Max (3200); selector hidden. Client 10.9.0 in C:\mscc-net9. Servers not rebuilt. Smoke pass (Stew 2026-10-09). Committed. |
| cmd-067b | done | Audio panel help text only. Client was 10.8.1, then cmd-068 moved it to 10.9.0. Header says only Operator Out is required to Start, and (none) means not set. Committed in 8564843. |
| cmd-067 | done | Part A committed in 8564843. WPF was 10.8.0, then cmd-067b moved the client to 10.8.1. trans 3.146, recv 3.144, ms-sdr 3.182 in C:\mscc-net9. Multus and Proficio hidden from audio lists. Speaker required. Mic and digital can be (none). PTT blocks voice or digital when that mic is unset. 2200 m and 630 m parse for RX IQ. Trans starts output-only without a mic. Recv rejects a bad speaker and does not use the radio as the speaker. Blank COM is COM0, no popup. Part B not started. Live smoke 1-7 not run (no radio). No push. |
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

cmd-076: WPF only, code ready, stopped before commit. Client 10.13.2. RX IQ button is APPLY ALL. START shows ACTIVE during a session. Cancelling APPLY or APPLY ALL sets a status line. Servers not rebuilt. Smoke 1-5 not run (no radio). No push.
cmd-075: WPF only, code ready, stopped before commit. Client 10.13.1. SMOOTHING 0xDA default 3, full frame rate, waterfall Falling once, per-band BASELINE and waterfall levels. Servers not rebuilt. Smoke 1-6 not run (no radio). No push.
cmd-074: Part A code ready, stopped before commit. Client 10.13.0. Trans 3.147, ms-sdr 3.183. ALC button gone; trans ignores ALC-off. Volume and mic gain saved a few seconds after the last slider step. Smoke 1-5 not run (no radio). No push.
cmd-073b: recv 3.147, client 10.11.3. Smooth S-meter needle and average-power meter. Smoke pass. Committed.
cmd-071: recv 3.146, client 10.11.2 version-only. Smoke pass. Committed. LO point paused (see note).
cmd-070: client 10.11.1. S-meter dBm toggle, nearest S unit, spectrum centre -83.6. Smoke pass. Committed.
cmd-069: recv 3.145 and client was 10.9.1, then cmd-070 moved it to 10.11.1. True dB. Smoke pass. Committed.
cmd-068: spectrum max-per-pixel, resolution fixed at 3200. Client was 10.9.0, then cmd-069 moved it to 10.9.1. Smoke pass. Committed.
cmd-067b: Audio help text. Client was 10.8.1. Not committed.
cmd-067: Part A code ready, not committed. Client is now 10.8.1. trans 3.146, recv 3.144, ms-sdr 3.182. All four are in C:\mscc-net9. Live bench not run (no radio on this laptop). Part B is ubuntu-stew. cmd-038 still pending.
cmd-064: done. Windows recv 3.143. The -12 kHz spur fix is in the DSP. The pixel notch is gone. cmd-038 still pending.
cmd-062: done. WPF 10.2.0, ms-sdr 3.181. Host park is opcode 0x29. The client no longer copies cal files.
cmd-059: WPF 10.1.2, trans 3.145, ms-sdr 3.180. Short on-tab Save settings line. Dead cal helpers removed.