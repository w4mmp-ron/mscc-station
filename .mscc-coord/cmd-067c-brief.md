# cmd-067c - Linux: trans Digital with no operator mic + revert Avalonia PTT/comm-port changes (stew-HP)

Follow-up to cmd-067 Part B, in the same uncommitted tree on stew-HP (base 71f7f43). Do NOT commit or push.
Do NOT touch rpi/, Solidus/, installers/rpi/, or Windows.

## 1. trans: Digital audio with a digital mic but no operator mic
File `linux/SDRcore-trans-linux/sources/udp_thread.c`, `CMD_SET_AUDIO_DEVICE` / `DIGITAL_AUDIO` case (lines 504-569).
Today lines 520-529 go output-only whenever the operator mic is invalid, even when the digital mic is valid. The operator index
is only needed for the stop call, and `manage_stream(0, ...)` ignores the device.

Replace lines 520-532, from `if (op_idx < 0 || op_idx >= MAX_INPUT_DEVICES) {` through
`stream_status = manage_stream(0, G_input_devices[op_idx].device_index, G_input_devices[op_idx].num_channels);`, with:

```c
                G_audio_mode = DIGITAL_AUDIO;
                if (op_idx < 0 || op_idx >= MAX_INPUT_DEVICES) {
                    /* Stop only: manage_stream(0, ...) ignores the device. */
                    stream_status = manage_stream(0, 0, 2);
                    if (dig_idx < 0 || dig_idx >= MAX_INPUT_DEVICES) {
                        print_time();
                        fprintf(G_fp_logfile,
                            "[%d] UDP Thread. CMD_SET_AUDIO_DEVICE DIGITAL: no digital or operator mic — output-only I/Q\n",
                            line_number++);
                        stream_status = manage_stream(1, -1, 2);
                        break;
                    }
                    print_time();
                    fprintf(G_fp_logfile,
                        "[%d] UDP Thread. CMD_SET_AUDIO_DEVICE DIGITAL: no operator mic — opening digital mic idx %d\n",
                        line_number++, dig_idx);
                } else {
                    stream_status = manage_stream(0, G_input_devices[op_idx].device_index,
                        G_input_devices[op_idx].num_channels);
                }
```
- Leave the rest of the case as is: the digital-open block from 533, including TUNE/CW output-only. `dig_idx` was already set to `op_idx` at 518 when the digital mic is unset, so "both invalid" goes output-only.
- One line: in the "DIGITAL stream failed (%d) (no operator fallback)" branch (556-561 before the edit), add `stream_status = manage_stream(1, -1, 2);` after the log, and change its text to "... — output-only I/Q".
- OPERATOR (571-597) and REMOTE (599-627): no change.
- main.c: no change. ms-sdr `User_Controls_Apply_To_Cores()` sends the saved P/D path after trans starts output-only.

## 2. Revert the Part B Avalonia PTT block and the comm-port.ini drop
MSCC Init and the mscc postinst always write comm-port.ini (/dev/tnt0), digital-microphone.ini (VirtualB.monitor) and
digital-speaker.ini (VirtualA), and Init forces a mic pick, so these could never fire.
- `mscc-ui/Avalonia-Migration/src/MSCC.Avalonia/Services/LocalServerLauncher.cs`: its only Part B change is the 2 removed lines. `git diff --stat` shows 2 deletions; I checked this. Run:
  `git checkout 71f7f43 -- mscc-ui/Avalonia-Migration/src/MSCC.Avalonia/Services/LocalServerLauncher.cs`
  `git diff` on the file must then be empty.
- `mscc-ui/Avalonia-Migration/src/MSCC.Avalonia/ViewModels/MainViewModel.cs`: hand edit. Do NOT `git checkout` this file.
  1. Line 188: replace the whole `AppendLog(...)` line with
     `        AppendLog("MSCC Avalonia 0.6.73 — trans starts with no mic; RX IQ 2200/630.");`
  2. `OnPttOnChanged`, lines 1038-1048: delete the whole `if (value && TxAudioNotSetReason() is string why) { ... return; }` block (11 lines). Leave `if (!CanOperate() || TxSetByServer) return;` then the blank line and `_ = SendPttAsync(value);` as they were.
  3. Lines 1086-1098: delete the `/// <summary>Voice/digital TX needs a local mic ...` comment and the whole `TxAudioNotSetReason()` method, plus the blank line after it, so `private async Task SendPttAsync(bool on)` follows `}` and one blank line.
  4. KEEP `_clientVersionText = "0.6.73"` (494) and `meters is 2200 or 630 or 160 ...` (4259).
  Check: `git diff 71f7f43 -- <MainViewModel.cs>` shows only 3 hunks: the 188 log line (0.6.72→0.6.73 text), the 494 version, and the 2200/630 line. `grep -n TxAudioNotSetReason` finds nothing.

## 3. CHANGELOG (`installers/linux/CHANGELOG.md`)
Replace the "### Fixed" bullets under `## mscc 1.0.51 / mscc-ui 0.6.73 — 2026-10-08` with:
```
### Fixed
- Transmit server starts when the operator mic is missing or not found (TX voice off; TUNE/CW still work, I/Q output-only).
- Digital audio uses the digital mic even when the operator mic is missing.
- RX IQ START works on 2200 m and 630 m (UI).

If your mic changes, re-run MSCC Init, then Stop/Start MSCC.
```
Remove the PTT pop-up and comm-port.ini bullets. Leave the shipped-versions table (1.0.51 / 0.6.73) and the older sections as is.

## 4. Versions / build
- No bumps: mscc **1.0.51**, mscc-ui **0.6.73**, trans **3.142** (none are committed or released yet). ms-sdr `version.h` auto-bumps on make (177 → probably 178). Record the final number.
- Servers: `./linux-build/build-mscc-deb-amd64.sh`, which rebuilds `linux/mscc-deb/mscc_1.0.51_amd64.deb` and drops it into installers/linux.
- UI: `rm -rf mscc-ui/Avalonia-Migration/publish/linux-x64-sc && ./linux-build/mscc-ui-x64.sh && ./linux-build/build-mscc-ui-deb-amd64.sh`. That rebuilds `mscc-ui_0.6.73_amd64.deb` in `mscc-ui/Release/avalonia/x86_64/` and installers/linux.
- Check:
  - installers/linux has only `mscc_1.0.51_amd64.deb` and `mscc-ui_0.6.73_amd64.deb`, both with today's times.
  - `grep -c 'no operator mic — opening digital mic' $HOME/mscc/sdrcore-trans` returns 1.
  - In the new UI deb, `MSCC.Avalonia.dll` does not contain "MSCC Init / Settings to transmit voice".
- Update the cmd-067 row in `.mscc-coord/status/ubuntu-stew.md` with 067c and the final ms-sdr number.

## 5. Smoke (stew-HP, install both rebuilt debs)
Back up first: `cp ~/.local/mscc/operator-microphone.ini ~/operator-microphone.ini.bak`
1. Missing operator mic: write a name that matches no device (`echo ZZZ_NO_SUCH_MIC > ~/.local/mscc/operator-microphone.ini`), then Stop/Start MSCC.
   - The trans log shows "No operator mic set: TX voice off" and "OUTPUT-ONLY I/Q". RX works, with no keep-alive loss.
   - USB PTT keys but sends no audio (no pop-up).
   - TUNE gives RF, and CW keys.
2. Same setup, with digital mic `VirtualB.monitor` (as Init writes it), Audio D, DIG-U.
   - The log shows "no operator mic — opening digital mic".
   - WSJT-X TX or Tune puts digital audio on the air (ALC/power moves).
3. Restore: `cp ~/operator-microphone.ini.bak ~/.local/mscc/operator-microphone.ini`, then Stop/Start. SSB TX audio works, and P/D switch as before.
4. RX IQ START opens on 2200 m and 630 m (Geminus). The title/version shows 0.6.73.
5. Optional: move `~/.local/mscc/comm-port.ini` aside. The Avalonia launch warning lists comm-port.ini again. Put it back.

## Report
Lines changed per file, the final ms-sdr version.h number, deb paths/sizes/times, and smoke results (or "not run, no radio").

STOP. No commit, no push.
