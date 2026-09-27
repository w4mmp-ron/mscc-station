# Build brief - cmd-045a (paste-ready for Grok Build)

**Host:** windows-new-hp (NEW-HP-LAPTOP)  
**Checkout:** `C:\Users\n8vet\OneDrive\Documents\GitHub\mscc-station`  
**Role:** WPF **client** + Windows **ms-sdr-MKII** (one small `calibrate.c` addition, see 3)  
**Call the user Stew.**  
**Commit on this host when done. Do not git push** (Stew pushes). Do not pull.  
**Out of scope:** Avalonia (both trees), `linux/`, `rpi/`, `Solidus/`, SDRcore-recv/trans, mscc-init, firmware, CAT.

Follow-up to cmd-045 (`2227a67`, client 9.26.4). Line numbers checked at `2227a67`. Anchor on the quoted text.

Roots:
- WPF = `mscc-ui/windows-work-tree/mscc-mscc/mscc-new/src/MSCC.Wpf/`
- Core = `mscc-ui/windows-work-tree/mscc-mscc/mscc-new/src/MSCC.Core/`
- Server = `mscc-ui/windows-work-tree/ms-sdr-MKII/source/`

---

## 1. What Stew saw, and why

Stew: "there is no STOP for FREQ CAL." He's right. The FREQ CAL tab has LOOSE, AUTO, MANUAL, CHECK and RESET (MainWindow.xaml:1885-1891) and nothing else. cmd-045 item 3b meant the **main** Start/Stop button (`StartStop_Click`, MainWindow.xaml.cs:766-782). Build did that part: `ResetFreqCalSessionOnStop()` (:1860-1878) runs when the servers are stopped. The tab itself has no way to stop a run.

How a run works today:
- AUTO (`FreqCalAutoButton_Click` :1639, `StartFreqCalAutoAsync` :1710) sends LOOSE, CHECK=0, CAL_MODE, then `CMD_START_CALIBRATE` 0xA7 and returns. CHECK (:1738, :1777) sends LOOSE, then `CMD_SET_FREQ_CAL_CHECK` 0x8C = 1 and returns.
- The sweep runs **in the servers**. ms-sdr `calibrate.c` steps the frequency (`Calibrate_Si5351_Continue` :415, `Check_Calibration_Continue` :469). SDRcore-recv measures each step (`update_calibrate_data`, which can wait up to 30 s per step) and replies `CMD_SET_CAL_DATA_PROCESSED`. When the sweep is done, ms-sdr sends recv `CMD_SET_CALIBRATION_FINISHED`, recv replies `CMD_SET_CALIBRATION_DATA`, and ms-sdr finishes.
- The client only listens: progress 0x6A (`OnCalProgressReported` :1897), status 0x62 (`OnCalStatusReported` :1914) and delta 0x6B (:1966). There's no client loop, Task or CancellationToken to cancel.
- **No correction is saved during the sweep.** AUTO only writes the new PPM at the very end (`Calibrate_Si5351_Set_PPM` :323: `Freq_Set_Transceiver_Calibration` + `Update_PPM_ini`). FINE's `CMD_SET_CAL_MODE`=1 only re-sends the stored G_int/G_dec (:762). CHECK never writes anything. So a stopped run leaves the calibration as it was.
- **There's no abort command in ms-sdr** (`Process_Frequency_Calibration` :629-810 has none). If the client just stopped listening, the servers would keep sweeping, and an AUTO would still apply its PPM at the end. So a clean STOP needs a small server command (3).

## 2. Version

- Client auto-bumps on build: 9.26.4 -> **9.26.5**. Don't hand-edit `ClientVersion.txt`.
- ms-sdr-MKII `VERSION_MINOR` auto-bumps: 3.174 -> expect **3.175**. Report both.

## 3. ms-sdr-MKII: add `CMD_SET_CAL_ABORT`

1. Opcode: use **0x1F** if it's really free. It's marked `// Free` in the mscc-init `usbavrcmd.h`. Check it isn't used in ms-sdr `usbavrcmd.h`, `stew/usbavrcmd.h`, or Core `Protocol/Opcodes.cs`, and isn't passed through to the radio. If it's taken, pick another free byte and say which one.
2. Add `#define CMD_SET_CAL_ABORT 0x1F` to ms-sdr `usbavrcmd.h`, and add it to the "Frequency Calibration" case list in `main-controller.c` (:2229-2241) so it reaches `Process_Frequency_Calibration`.
3. In `calibrate.c`, add `case CMD_SET_CAL_ABORT:`. If an AUTO or CHECK is running:
   - set a file-scope `cal_abort_pending = TRUE`;
   - reset the run state in **both** places: the statics in `Process_Frequency_Calibration` and the separate statics in `Process_Check_Calibration` (:525). Move them to file scope or add a reset path. Also set `G_check_calibration = 0`;
   - put back what `Calibrate_Si5351_Failed` (:446) puts back, **without** touching the PPM: `G_mode`/`G_tune_freq` = previous, `ModeChanged(previous_mode)`, `freq_queue_add`, `CMD_SET_MAIN_MODE` to recv and trans, `G_Calibration_temperature = previous_temperature`, `G_Proficio_Allow_Temp_Check = TRUE`;
   - send recv `CMD_SET_CALIBRATION_FINISHED` so it clears its step counter (`Send_calibration_data` sets its `G_calibration_element = 0`). recv will answer with `CMD_SET_CALIBRATION_DATA`;
   - send nothing to the GUI (the client has already shown STOPPED). Log ` CMD_SET_CAL_ABORT: run stopped, PPM unchanged`.
   If nothing is running: just log it.
4. While `cal_abort_pending` is set, **drop** any late `CMD_SET_CAL_DATA_PROCESSED` or `CMD_SET_CALIBRATION_DATA` (AUTO and CHECK paths, :700-747 and `Process_Check_Calibration`). That means no `Set_PPM`, no `Check_PPM`, no `Failed`, and no GUI messages. Clear the flag when that `CMD_SET_CALIBRATION_DATA` arrives, and also at the next `CMD_START_CALIBRATE` / `CMD_SET_FREQ_CAL_CHECK`=1. Log each drop.
5. Edge case: if the abort arrives after `Calibrate_Si5351_Set_PPM` has already run, the AUTO has finished and the correction stays. That's fine. Just log it.

## 4. Core

`Opcodes.cs`: `CMD_SET_CAL_ABORT = 0x1F` (or whatever you picked). Add `Task AbortCalibrationAsync(CancellationToken ct = default)` to `IRadioService` (:303-309 area) and `UdpRadioService` (next to `StartCalibrateAsync` :976). It sends 1 and logs ` Send CMD_SET_CAL_ABORT`. If Avalonia implements `IRadioService` somewhere, give it a no-op/stub so it still compiles (don't wire any UI there).

## 5. WPF: STOP button on the FREQ CAL tab

**Design: a separate STOP button, not a Start button that turns into STOP.** This tab has no single Start button. Runs start from **two** buttons (AUTO and CHECK), and AUTO opens a COARSE/FINE dialog first. A separate STOP is clearer and doesn't affect MANUAL (which already exits by clicking MANUAL again).

1. XAML (MainWindow.xaml:1885-1891): add after RESET, in the same row:
   `<Button x:Name="FreqCalStopButton" Content="STOP" Width="70" Height="28" Margin="2" FontSize="12" FontWeight="Bold" IsEnabled="False" Click="FreqCalStopButton_Click" ToolTip="Stop the AUTO or CHECK run. The calibration is not changed."/>`
   Update the row comment to list STOP. Make it clearly visible when enabled (e.g. red/amber text like the other alerts) and grey when disabled. Check the row still fits.
2. **Enabled only while an AUTO or CHECK run is active.** Add one helper (e.g. `SetFreqCalRunActive(bool)`) that sets `_freqCalInProgress` and `FreqCalStopButton.IsEnabled` together. Use it at every place that sets `_freqCalInProgress` now: AUTO start :1696, CHECK start :1769, the two catch blocks :1724/:1789, `OnCalStatusReported` :1924, `ResetFreqCalSessionOnStop` :1862. MANUAL never enables STOP.
3. `FreqCalStopButton_Click`: if nothing is running, return. Otherwise:
   - log ` Freq Cal: STOP pressed during AUTO` (or `CHECK`);
   - `_ = ViewModel.RadioService.AbortCalibrationAsync()` (catch and log errors);
   - then do item 2's Stop behaviour. Reuse or extend `ResetFreqCalSessionOnStop()`: clear `_freqCalInProgress`/`_freqCalIsAuto`/`_freqCalRestorePending`, `SetFreqCalControlsEnabled(true)` (LOOSE, AUTO, MANUAL, CHECK, RESET), `ClearFreqCalRunningHint()`, **reset the progress bar to 0** and `_freqCalProgressSteps`/`_lastCalDelta`/`_freqCalHaveDelta` (e.g. `ResetFreqCalVisuals`), then status `STOPPED` in the idle colour;
   - don't clear `_freqCalResetPendingAuto` (a stopped AUTO didn't calibrate anything).
   - Once `_freqCalInProgress` is false, **QRP CAL / AMP CAL / TX IQ can be opened again** (the block is `blockForOtherCal` :687). Nothing else is needed there, but check it.
4. Mode, filter and pitch: the tab is still open after STOP, so the radio **stays in the tab's CW / 600 pitch / 200 filter** (the server abort puts back its pre-run mode, which is that CW). Leaving the tab later restores the user's mode, as it does now (`LeaveFreqCalTab`, MainViewModel.cs:3813). Don't call `LeaveFreqCalTab` on STOP.
5. Late messages: after STOP, any 0x62/0x6A/0x6B that still arrives hits the "not running" branches (:1904, :1960). Make sure the delta handler (:1966) doesn't add text to "STOPPED".
6. Main Stop and Close during a run: in `StartStop_Click` (:771) and `MainWindow_Closing` (:1112), if `_freqCalInProgress`, send `AbortCalibrationAsync()` first (best effort), then do what they already do. Change the Closing log (:1113) to say the run is aborted.

## 6. Build / deploy / commit

1. `git status` must be clean at the start (the orders commit is in). Local main is ahead of origin. **Don't pull, don't push.**
2. Stop MSCC. Build `ms-sdr-MKII.sln` (MFC/Win32, v143) -> `mscc-ui/Release/windows-wpf/ms-sdr-MKII.exe`, then copy it to `C:\mscc-net9\`.
3. Build `MSCC.Wpf` Release with no new warnings. The `CopyToMsccNet9` target deploys it to `C:\mscc-net9\`.
4. Commit the source, `ClientVersion.txt`, `version.h` and the Release ms-sdr exe. Hint: `cmd-045a: FREQ CAL STOP button + ms-sdr CMD_SET_CAL_ABORT`. **Do not push.**

## 7. Smoke for Stew (3 steps)

1. **STOP during AUTO:** open FREQ CAL. STOP is grey. Press AUTO, pick FINE. STOP lights up. Press STOP after a few progress steps. The status says STOPPED, the progress bar goes to 0, the amber line goes away, and LOOSE/AUTO/MANUAL/CHECK/RESET work again. You're still in CW 600/200. The frequency doesn't move and no "corrected" message appears. Then run CHECK to the end: the error is the same as before the stopped AUTO.
2. **STOP during CHECK:** press CHECK, then STOP. You get STOPPED, and STOP goes grey again. Now QRP CAL (or TX IQ) opens without "FREQUENCY CALIBRATION IN PROGRESS". Go back to FREQ CAL and run CHECK again: it runs normally to CHECK COMPLETED.
3. **Leave after STOP:** in USB, open FREQ CAL, start AUTO, press STOP, then click MAIN. USB comes back with your normal filter and pitch. MANUAL never lights STOP.

## Status / commit

- ACK in `.mscc-coord/status/windows-new-hp.md` (`accepted` -> `running` -> `done`, or `blocked` with the reason). No bare `|` in table cells.
- Report: client version, ms-sdr version, opcode used, commit hash, and whether any late server message got through after STOP.
- **Do not push.** Stew pushes. Ubuntu/Pi ms-sdr and Avalonia wait for a later order.

Full orders: `.mscc-coord/COMMANDS.yaml` id `cmd-045a`.
