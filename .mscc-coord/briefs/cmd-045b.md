# Build brief - cmd-045b (paste-ready for Grok Build)

**Host:** windows-new-hp (NEW-HP-LAPTOP)  
**Checkout:** `C:\Users\n8vet\OneDrive\Documents\GitHub\mscc-station`  
**Role:** WPF **client** + Windows **ms-sdr-MKII** (`calibrate.c` only on the server side)  
**Call the user Stew.**  
**Commit on this host when done. Do not git push** (Stew pushes). Do not pull.  
**Out of scope:** Avalonia (both trees), `linux/`, `rpi/`, `Solidus/`, SDRcore-recv/trans, mscc-init, firmware, CAT.

Follow-up to cmd-045a (`5588f71`, client 9.26.5, ms-sdr 3.175). Line numbers checked at `5588f71`. Anchor on the quoted text.

Roots:
- WPF = `mscc-ui/windows-work-tree/mscc-mscc/mscc-new/src/MSCC.Wpf/`
- Core = `mscc-ui/windows-work-tree/mscc-mscc/mscc-new/src/MSCC.Core/`
- Server = `mscc-ui/windows-work-tree/ms-sdr-MKII/source/`

Four items: STOP colour (1), restart race after STOP (2), no second finish on abort (3), one misplaced comment (4).

---

## 1. STOP button: same look as the other FREQ CAL buttons

What's there now (MainWindow.xaml:1886-1905):
- LOOSE, AUTO, MANUAL, CHECK and RESET (:1886-1890) have **no `Style` and no `Foreground`**. They use the plain default WPF Button look: black text when enabled, the standard grey text/face when disabled.
- STOP (:1891-1905) is different in two ways: `Foreground="#FFC000"` (:1894) **and** a local `<Button.Style>` (:1895-1904) `BasedOn="{StaticResource MsccGoldButtonStyle}"` with its own `IsEnabled=False` trigger (Opacity 0.45, Foreground #888888). The local `Foreground` attribute beats every style setter, so STOP always shows amber text, and the gold style gives it a different face from its neighbours.

Fix:
1. Delete `Foreground="#FFC000"` (:1894).
2. Delete the whole `<Button.Style> ... </Button.Style>` block (:1895-1904), so STOP has no local style, exactly like the other five. Close the tag as `.../>` or keep `</Button>`, whichever is cleaner.
3. Keep `x:Name`, `Content`, `Width="70" Height="28" Margin="2" FontSize="12" FontWeight="Bold"`, `IsEnabled="False"`, `Click` and `ToolTip`.
4. Result: STOP looks the same as LOOSE/AUTO/MANUAL/CHECK/RESET in every colour scheme. It follows the scheme exactly as far as they do. Enabled = same text colour as the others (black on the default face). Disabled = the same grey as a disabled neighbour (e.g. AUTO during a run).
5. Don't restyle the other five buttons in this order. Note for the report: those five don't use `MsccGoldButtonStyle` or the `UiButton*` scheme brushes (`ApplyUiChromeTheme`, MainWindow.xaml.cs:340-387), so today a Setup colour-scheme change doesn't recolour any FREQ CAL button. If Stew wants all six themed, that's a follow-up order.

## 2. Restart race after STOP ("abort drain")

**Problem.** After STOP, SDRcore-recv can still be inside the aborted step (`update_calibrate_data`, up to 30 s). Its late `CMD_SET_CAL_DATA_PROCESSED` / `CMD_SET_CALIBRATION_DATA` are dropped only while `cal_abort_pending` is set. Today the flag is cleared at the **next start** (`CMD_SET_FREQ_CAL_CHECK`=1 :680, `CMD_START_CALIBRATE` :701). So if Stew presses AUTO or CHECK right after STOP, the old replies land in the new run. They can move its step counter, and a late `CMD_SET_CALIBRATION_DATA` can end the new AUTO early through `Calibrate_Si5351_Set_PPM` (:751) with a bad value.

**Mechanism (one rule on each side).**
- **Server:** after an abort, ms-sdr owes exactly one `CMD_SET_CALIBRATION_DATA` from recv (the reply to the one `CMD_SET_CALIBRATION_FINISHED` that is in flight). The drain stays open until that reply arrives, or 35 s have passed. While it's open, late replies are dropped and **new AUTO/CHECK starts are refused**. When it closes, ms-sdr tells the client.
- **Client:** after STOP, AUTO and CHECK stay grey with `STOPPED — wait…` until the server says the drain is done, or a 36 s client timer fires (35 s server timeout + 1 s margin, so the client never re-enables before the server would accept). Then status `STOPPED`, AUTO/CHECK enabled.
- **Server -> client message:** reuse opcode **0x1F** (`CMD_SET_CAL_ABORT`) in the server -> GUI direction, 16-bit value like 0x62: `1` = drain done (ready), `2` = start refused (drain still open). Client -> server 0x1F stays "abort". Check that `Gui_send_param` passes 0x1F through to the client and that nothing on the client already treats an incoming 0x1F as something else. If 0x1F can't be used this way, pick a free byte, say which one, and add it on both sides.

### 2a. ms-sdr-MKII `calibrate.c`

1. File scope, next to `cal_abort_pending` (:33): `static ULONGLONG cal_abort_since = 0;` (Windows `GetTickCount64()`) and `#define CAL_ABORT_DRAIN_MS 35000`.
2. Helper `static void Cal_Abort_Drain_Done(const char *why)`: `cal_abort_pending = FALSE; cal_abort_since = 0; Gui_send_param(CMD_SET_CAL_ABORT, 1);` and log ` CMD_SET_CAL_ABORT: drain done (<why>)`.
3. Helper `static int Cal_Abort_Drain_Blocks_Start(void)`: if `!cal_abort_pending`, return 0. If `GetTickCount64() - cal_abort_since >= CAL_ABORT_DRAIN_MS`, call `Cal_Abort_Drain_Done("timeout")` and return 0. Otherwise log ` CMD_SET_CAL_ABORT: start refused, drain pending (<ms> ms)`, `Gui_send_param(CMD_SET_CAL_ABORT, 2)`, return 1.
4. `CMD_SET_FREQ_CAL_CHECK` (:677-691): at the top, **before** `G_check_calibration = opcode_data_8_bit;` (:678), if the value is 1 and `Cal_Abort_Drain_Blocks_Start()`, `break` (don't touch `G_check_calibration`, don't start). Remove `cal_abort_pending = FALSE;` (:680).
5. `CMD_START_CALIBRATE` (:700-713): first line `if (Cal_Abort_Drain_Blocks_Start()) break;`. Remove `cal_abort_pending = FALSE;` (:701).
6. `CMD_SET_CAL_ABORT` (:715-737): see item 3 for the new "owed reply" test. When a reply is owed: `cal_abort_pending = TRUE; cal_abort_since = GetTickCount64();` plus the existing restore. When nothing is owed (idle, or run already finished): **don't** set pending; call `Cal_Abort_Drain_Done("nothing owed")` right away so the client re-enables at once. Keep the existing restore/log for "run stopped" vs "nothing running".
7. `CMD_SET_CALIBRATION_DATA` (:739-744): while pending, drop it (as now) and call `Cal_Abort_Drain_Done("recv finish reply")` instead of just clearing the flag.
8. `CMD_SET_CAL_DATA_PROCESSED` (:766-770): while pending, drop it (as now). If the 35 s have passed, also call `Cal_Abort_Drain_Done("timeout")` after dropping.
9. The client's main Stop may kill ms-sdr during a drain. That's fine: the flag is process memory and starts at FALSE.
10. Log every drop, refuse and done. No other GUI messages.

### 2b. Core

1. `UdpRadioService` receive switch (`case Opcodes.CMD_SET_CALIBRATION_FINISHED:` :1727): add `case Opcodes.CMD_SET_CAL_ABORT:`. Read the Int16 like 0x62, raise `event Action<int>? CalAbortStateReported`, log ` Processed cal abort state: {val} (1=drain done, 2=start refused)`.
2. Add `event Action<int> CalAbortStateReported;` to `IRadioService` (next to `CalStatusReported` :315). Any other `IRadioService` implementation (Avalonia/stub) only gets an unused event so it compiles.

### 2c. WPF (`MainWindow.xaml.cs`)

1. Fields: `bool _freqCalAbortDrainPending`, `DispatcherTimer? _freqCalAbortDrainTimer` (36 s, one-shot), `const string FreqCalStoppedWaitText = "STOPPED — wait…"`.
2. Subscribe/unsubscribe `CalAbortStateReported` next to `CalStatusReported` (:146 and :1107). Marshal to the Dispatcher like `OnCalStatusReported` (:1946).
3. `FreqCalStopButton_Click` (:1887-1894): after `ResetFreqCalSessionOnStop()`, call `BeginFreqCalAbortDrain()`: set pending, status `STOPPED — wait…` (idle colour), grey AUTO and CHECK, (re)start the 36 s timer, log ` Freq Cal: STOP - waiting for server drain`.
4. `EndFreqCalAbortDrain(string why)`: if not pending, return. Clear pending, stop the timer, re-enable AUTO/CHECK (only if nothing else is blocking: not in MANUAL, no run active), and if the status still shows the wait text set it to `STOPPED`. Log ` Freq Cal: drain done (<why>)`. Call it from: event value 1 (`server`), the timer tick (`client timeout`), and main Stop in `StartStop_Click` (:771-783, servers going away).
5. Event value 2 (server refused a start): the server never started a run, so undo the client's start. If `_freqCalInProgress`, do the same UI reset as STOP (`ResetFreqCalSessionOnStop()`, no abort sent), then `BeginFreqCalAbortDrain()` again. Log ` Freq Cal: start refused by server (drain pending)`.
6. `SetFreqCalControlsEnabled` (:1855-1864): AUTO and CHECK get `enabled && !_freqCalAbortDrainPending`. Every existing call (late 0x62 in `OnCalStatusReported` :1959, catch blocks :1732/:1797, `ResetFreqCalSessionOnStop` :1907) then can't re-enable them early. LOOSE, MANUAL and RESET stay as they are.
7. Belt and braces: at the top of `FreqCalAutoButton_Click` (:1645) and `FreqCalCheckButton_Click` (:1744), `if (_freqCalAbortDrainPending) return;` with a log line.
8. `ResetFreqCalVisuals` (:1812-1827): treat any text starting with `STOPPED` as idle colour (the wait text too).
9. Late 0x6A/0x6B during the drain already hit the "not running" branches. Make sure they don't overwrite the wait text.
10. MANUAL, leaving the tab and QRP CAL/AMP CAL/TX IQ are unaffected (`_freqCalInProgress` is already false after STOP).

## 3. No second finish on abort (Risk B)

**Problem.** At the last step, `Calibrate_Si5351_Continue` (:454-456) / `Check_Calibration_Continue` (:507) have **already** sent recv `CMD_SET_CALIBRATION_FINISHED`, and the state is `CALIBRATION_COMPLETE` (3), not `CALIBRATION_RUNNING` (2). The abort test (:717-721) also matches on `*_initialized == CALIBRATION_INITIALIZED` or `G_check_calibration`, so it sends a **second** FINISHED (:732). recv answers twice. With today's code the first answer clears the flag (:741), and the second one goes through as a real result. Because `Reset_Calibration_Run_State` set `G_check_calibration = 0`, it even takes the **AUTO** branch: a spurious FAIL, or `Set_PPM`.

Fix in `case CMD_SET_CAL_ABORT` (:715-737):
- `stepping = (auto_calibration_state == CALIBRATION_RUNNING || check_calibration_state == CALIBRATION_RUNNING)`
- `finishing = (auto_calibration_state == CALIBRATION_COMPLETE || check_calibration_state == CALIBRATION_COMPLETE)` (FINISHED already sent, its reply not in yet)
- `owed = stepping || finishing`
- Send recv `CMD_SET_CALIBRATION_FINISHED` **only if `stepping`**.
- Open the drain (item 2a.6) only if `owed`. Either way exactly one `CMD_SET_CALIBRATION_DATA` is expected, and it closes the drain.
- The restore (mode/freq/temperature, `Reset_Calibration_Run_State`) runs if `owed` **or** the old test matched, as now. Log which case: ` CMD_SET_CAL_ABORT: stepping, finish sent` / ` finishing, finish already sent` / ` nothing owed`.
- Read the states **before** `Reset_Calibration_Run_State()` clears them.

## 4. Cosmetic

MainWindow.xaml.cs:1866: the summary `/// <summary>Stop ends a FREQ CAL run in the UI. Mode restore is LeaveFreqCalTab.</summary>` sits on `SetFreqCalRunActive` (:1867). Move it to `ResetFreqCalSessionOnStop()` (:1896). Give `SetFreqCalRunActive` its own one-liner, e.g. `/// <summary>Sets _freqCalInProgress and STOP enabled together.</summary>`.

## 5. Version

- Client auto-bumps on build: 9.26.5 -> **9.26.6**. Don't hand-edit `ClientVersion.txt`.
- ms-sdr-MKII `VERSION_MINOR` auto-bumps: 3.175 -> expect **3.176**. Report both.

## 6. Build / deploy / commit

1. `git status` must be clean at the start, apart from the usual `ms-sdr-MKII/Release/*.iobj/*.ipdb` noise (don't commit those). The orders commit is in. Local main is ahead of origin. **Don't pull, don't push.**
2. Stop MSCC. Build `ms-sdr-MKII.sln` (MFC/Win32, v143) -> `mscc-ui/Release/windows-wpf/ms-sdr-MKII.exe`, then copy it to `C:\mscc-net9\`.
3. Build `MSCC.Wpf` Release with no new warnings. The `CopyToMsccNet9` target deploys it to `C:\mscc-net9\`.
4. Commit the source, `ClientVersion.txt`, `version.h` and the Release ms-sdr exe. Hint: `cmd-045b: FREQ CAL STOP colour + abort drain + no double finish`. **Do not push.**

## 7. Smoke for Stew (3 steps)

1. **STOP colour:** open FREQ CAL. STOP is grey like a disabled button. Press CHECK: STOP lights up with the **same** text colour and face as LOOSE/MANUAL/RESET (not amber). Press STOP. Now change the colour scheme in Setup and come back: STOP still looks like the other five, both enabled (during a run) and disabled (idle).
2. **STOP then restart:** press AUTO (FINE), then STOP after a step or two. The status says `STOPPED — wait…` and AUTO and CHECK stay grey. LOOSE/MANUAL/RESET work. Within about 35 s (usually a few seconds) the status goes to `STOPPED` and AUTO/CHECK light up again. Now run CHECK to the end: it counts from 0, finishes normally, and the error is the same as before the stopped AUTO.
3. **STOP near the end of CHECK:** press CHECK and wait until the progress bar is almost full, then press STOP. You get `STOPPED — wait…` then `STOPPED`. There's **no** "CHECK FAILED" or "AUTO FAILED", no "corrected" message, and the frequency doesn't jump. The ms-sdr log shows `finishing, finish already sent` or `stepping, finish sent`, one dropped `CMD_SET_CALIBRATION_DATA` and `drain done`.

## Status / commit

- ACK in `.mscc-coord/status/windows-new-hp.md` (`accepted` -> `running` -> `done`, or `blocked` with the reason). No bare `|` in table cells.
- Report: client version, ms-sdr version, the server -> client opcode/values used for drain done/refused, commit hash, and how long the drain took in the smoke (from the ms-sdr log).
- **Do not push.** Stew pushes. Ubuntu/Pi ms-sdr and Avalonia wait for a later order.

Full orders: `.mscc-coord/COMMANDS.yaml` id `cmd-045b`.
