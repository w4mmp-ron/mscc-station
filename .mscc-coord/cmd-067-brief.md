# cmd-067: hide Multus IQ from audio lists; no start fault without mic/digital/COM; block voice/digital PTT with no mic; RX IQ on 2200 m / 630 m

Hosts: **windows-new-hp** (Part A: WPF + Windows servers), then **ubuntu-stew** (Part B: Linux servers + Avalonia).
Repo: `C:\Users\n8vet\OneDrive\Documents\GitHub\mscc-station` (Windows) and `/home/stew/Documents/GitHub/mscc-station` (stew-HP).
**Do NOT touch `rpi/`, `Solidus/`, or `installers/rpi/`.** Stew: "Lets not go too crazy with the code changes." Keep each change small, with no refactors.
**STOP BEFORE COMMIT.** Show `git status` and `git diff --stat`, plus the full diff for each file.
Line numbers are from `dedffb5` (2026-10-08). Check them before you edit.

## Background (read-only findings)
- The WPF client saves only the text before `(` (`AudioDeviceConfig.ToMatchKey`), plus one trailing space. Nothing selected gives an **empty** ini file.
  The Windows servers read that ini with `fgets` and then `strstr(lpInfo->name, key)` across **every** MME device. Last match wins. An empty key matches every device,
  so the "mic" ends up as the last MME device, often an output with 0 input channels. Also, `NO_INPUT_DEVICE`/`NO_OUTPUT_DEVICE` = 100, but the tables are
  `[MAX_*_DEVICES=50]`, and Windows `udp_thread.c` indexes them without a bounds check on CMD_SET_AUDIO_DEVICE. Windows recv `manage_stream` also dereferences
  `Pa_GetDeviceInfo(dev)->...` with no NULL check. Any of these can cause the 0xc0000005. ms-sdr then logs trans/recv FAILED and stops, and the client shows the keep-alive warning.
- **The Linux servers already have most of the hardening, so use them as the model.** `linux/SDRcore-*-linux` skip a blank key (`want[0]`), skip Multus/Proficio,
  require channels ≥ 1, and bounds-check every index in udp_thread. They also have an output-only I/Q stream (`manage_stream(1, -1, 2)`), and recv falls back to the default output.
  Linux ms-sdr `comm-port.c` already treats a missing or invalid ini as a PTY default and is non-fatal. The one Linux gap: trans exits when **no mic is configured** (see B1).
- The Windows client start gate (`ConfigBootstrap.EvaluateLocalSetup`) puts **COM port** and **operator mic** in `Missing`, so Start is refused.
  Only the speaker should be required.
- Linux mscc-init (`linux/mscc-init-linux/sources/main.c` ~582-602) already hides Multus/Proficio from its pick lists. **Avalonia has no
  operator device dropdowns** (device setup is mscc-init on Linux), so the Multus filter is a WPF-only change.
- **2200 m and 630 m are already in the band tables almost everywhere.** Server tables have 12 records, with 630 m = record 10 and 2200 m = record 11 appended at the end
  (iq.ini, power_cal.ini, `G_iq_stack[12]`, `proficio_table[12]`, ms-sdr `iq_calibration_freqs[]`/`IQ_Band_to_Record`, `last_used.c` B630/B2200).
  The client already has them in band buttons, `GetBandNameForFrequency`, last-used keys (2200M/630M), QRP/AMP CAL, TX IQ, per-band S/W bank, and LAST_LF (474.2 kHz).
  The **only** missing piece is the RX IQ guard `TryParseAmateurBandMeters` in both clients, so no array resize or index shift is needed.

## Part A: Windows (windows-new-hp)

### A1. Hide the Multus IQ device from the WPF audio lists
File: `mscc-ui/windows-work-tree/mscc-mscc/mscc-new/src/MSCC.Wpf/AudioDeviceConfig.cs` lines 46-92.
- Add `private static bool IsRadioIqDevice(string name) => name.Contains("Multus", StringComparison.OrdinalIgnoreCase) || name.Contains("Proficio", StringComparison.OrdinalIgnoreCase);`
- In `GetOutputDevices()` and `GetInputDevices()`, `continue` when `IsRadioIqDevice(name)`. This one change covers Out, Mic, Digital Out, and Digital Mic,
  plus the start gate (all of them use these two lists).
- Why "Multus": it is the same identifier the servers use to find the I/Q device (`strstr(lpInfo->name, "Multus")`), and it survives the 31-char MME truncation
  ("Line (3- Multus IQ Sound)"). "Proficio" covers the Linux/Pulse naming. Don't filter on "Line", which would hide real line-ins.

### A2. Client: only the speaker is required; blank = not set
1. `MSCC.Wpf/ConfigBootstrap.cs`
   - 316-334 (COM): an empty port, or `COM0`, goes to `status.Warnings.Add("No COM port: CAT disabled")` instead of `Missing`. Keep the existing "not found" warning.
   - 353-367 (operator mic): all three `status.Missing.Add(...)` become `status.Warnings.Add("Operator mic not set: TX voice off (CW/TUNE/remote still work)")`.
   - 387 message text: "Local radio needs an operator speaker before starting servers." Speaker checks (336-351) stay hard.
2. `MSCC.Wpf/CommPortConfig.cs`: line 28 default `PortName = ""` (was "COM1"). In `Save()` (95-115), write `COM0` when `port` is empty.
   (ms-sdr already treats COM0 as no CAT.)
3. `MSCC.Wpf/Controls/AudioSettingsPanel.xaml.cs`
   - `FillCombo` (93-102): add an optional `bool allowNone`. When it's true, insert `new AudioDeviceConfig.DeviceChoice { DisplayName = "(none)", MatchKey = "" }` first.
     Saved index = `FindBestIndex(devices, key)` + 1. An empty key selects "(none)". Use `allowNone: true` for OpMic, DigSpeaker, and DigMic (67-70). The speaker stays without it.
     `KeyFromCombo` already returns `d.MatchKey` (""), and `WriteIni` already writes an empty file for "", so nothing else changes.
   - `CountUnsetRequired` (104-110): count only `OpSpeakerCombo`. Status text (76): "Select operator Out (Mic and digital optional), then Apply".
   - Optional, skip if it gets bigger: a "(none)" item in the COM list (`CommPortSettingsPanel.xaml.cs` 95-140) that saves `COM0`.

### A2b. WPF: block voice/digital PTT when the needed mic isn't set (Stew: "no hoops", don't go out silent)
File: `MSCC.Wpf/ViewModels/MainViewModel.cs`. There is **one** client TX entry point: `PttOn` → `OnPttOnChanged` (1133-1153) → `SetTransmitAsync(value)` (0xBA).
The PTT button (`TogglePtt`, 3652-3653, `MainWindow.xaml` 2010) and remote CAT `cat.SetPtt` (903) both set `PttOn`. WPF has no keyboard PTT/MOX binding.
TUNE is a separate path (`TuneMode`, untouched).
1. Add a helper next to `OnPttOnChanged`:
   ```csharp
   /// <summary>Voice/digital TX needs a local mic for the current audio path. Null = OK to key.</summary>
   private string? TxAudioNotSetReason()
   {
       // Host on another machine (no local servers) or Remote audio (mic is on the remote PC): not ours to judge.
       if (!_launchServersOnStart || RemoteAudio) return null;
       var mode = RadioState.ActiveVfo.Mode;
       if (TuneMode || mode is RadioMode.CW or RadioMode.TUNE) return null;
       var ins = AudioDeviceConfig.GetInputDevices();
       bool digital = IsDigitalAudio || mode == RadioMode.DigU;   // DIG-U forces Audio D (ApplyDigUAudioPolicy)
       string file = digital ? AudioDeviceConfig.DigitalMicFile : AudioDeviceConfig.OperatorMicFile;
       string key = AudioDeviceConfig.ReadIni(file).Trim();
       if (key.Length > 0 && AudioDeviceConfig.SavedKeyMatchesDevice(key, ins)) return null;
       return digital
           ? "Set up digital audio (Digital Mic) in the Settings tab to transmit in digital modes."
           : "Set up audio (Mic) in the Settings tab to transmit voice.";
   }
   ```
   "Mic set" = the same test the start gate uses: the ini is non-empty and matches a listed (non-Multus) input.
2. In `OnPttOnChanged`, right after the SWR block (after line 1144), use the same revert pattern as SWR:
   ```csharp
   if (value && !TxSetByServer && !_suppressTransmitCommands && TxAudioNotSetReason() is string why)
   {
       MonitorTextBoxText(" PTT blocked: " + why);
       Application.Current?.Dispatcher.BeginInvoke(() =>
       {
           if (PttOn) PttOn = false;
           MessageBox.Show(why, "MSCC", MessageBoxButton.OK, MessageBoxImage.Information);
       });
       return;   // no 0xBA, IsTransmitting unchanged
   }
   ```
   (Resetting `PttOn = false` sends one harmless 0xBA off, same as the SWR block does today.)

### A3. Windows SDRcore-trans: start without a mic (I/Q TX stays up)
Dir: `mscc-ui/windows-work-tree/SDRcore-trans/sources/`
1. `input-devices.c`
   - After the `fgets` calls in `Get_Operator_Sound_Device` (73-79) and `Get_Digital_Sound_Device` (30-37), strip trailing `\r\n`. If the rest is only spaces, set `[0] = '\0'`.
   - `build_input_devices` (126-131) and `build_digital_input_devices` (97-104): only match when `key[0] != '\0' && lpInfo->maxInputChannels >= 1 &&
     !strstr(lpInfo->name, "Multus") && !strstr(lpInfo->name, "Proficio")`. Leave the table fill as is.
2. `main.c`
   - Add a small helper next to `manage_stream`:
     ```c
     /* Valid record → PortAudio device, else -1 (output-only I/Q). */
     int mic_dev(const struct input_devices *t, int idx, int *ch) {
         if (idx < 0 || idx >= MAX_INPUT_DEVICES || t[idx].num_channels < 1) { *ch = 0; return -1; }
         *ch = t[idx].num_channels; return t[idx].device_index; }
     ```
     Export it via `extern.h` (non-static) so `udp_thread.c` can use it.
   - `manage_stream` (203-290): when `device < 0 || channels < 1`, open **output-only**. Use `Pa_IsFormatSupported(NULL, &outputParameters, 96000)` and
     `Pa_OpenStream(&stream, NULL, &outputParameters, ...)`, and set `inputchannels = 0`. Otherwise keep the current path. Log "OUTPUT-ONLY I/Q (no mic)".
     Guard the `format_error`/`lpError` `->errorText` prints against NULL.
   - Callback (94-126): replace the `inputBuffer == NULL` silent-return branch with a zero mic buffer so TUNE/CW/two-tone still make I/Q
     (same as the Linux trans callback):
     ```c
     } else if (inputBuffer == NULL) {
         static SAMPLE zero_in[4096 * 2];   /* zero-initialised */
         inbuffer = zero_in; outbuffer = (sp_float*) outputBuffer; mic_channels = 1;
     } else {
     ```
     Keep the `framesPerBuffer <= 4096u` guard like the remote path. If it's larger, keep the old silent fill.
   - 478-487: `Pa_GetDefaultInputDevice() == paNoDevice` is log-only, not `goto error`.
   - 509-524: when there's no mic, log "No operator mic set: TX voice off" and call `manage_stream(1, -1, 0)`. Don't `MessageBox` and don't `goto error`.
     When there is a mic, use `mic_dev(...)`. If the mic open fails, retry output-only before `goto error` (same as Linux main.c ~1186-1196).
     The fatal paths stay only for "Multus I/Q device not found" and an output-only failure.
3. `udp_thread.c` 476-510 (CMD_SET_AUDIO_DEVICE) and 970-978 (CMD_GET_SET_MIC_DEVICE): replace every direct `G_input_devices[idx]` /
   `G_digital_input_devices[idx]` with `mic_dev(table, idx, &ch)`. A digital mic that isn't set falls back to the operator mic, and if that isn't set either, -1 (output-only).
   Bounds-check `device_input_record_index` against `MAX_INPUT_DEVICES`.
4. `extern.h`: `VERSION_MINOR` 145 → 146.

### A4. Windows SDRcore-recv: no crash on a blank or odd speaker
Dir: `mscc-ui/windows-work-tree/SDRcore-recv/sources/`
1. `output-devices.c`: same trim as A3.1 in `Get_Sound_Device` (76-84) and `Get_Digital_Sound_Device` (32-40). Same match guard in `build_output_devices` (135-141)
   and `build_digital_output_devices` (105-111): key non-empty, `maxOutputChannels >= 1`, not Multus/Proficio. This also stops RX audio from being routed into the radio's I/Q output.
2. `main.c`
   - `manage_stream` (251-334): if `Pa_GetDeviceInfo(device) == NULL`, log and return `paInvalidDevice` (line 273 dereferences without a check).
     Open the speaker with `channelCount = 2`, because the callback always writes 2 samples per frame and a 1-ch open would overrun the buffer. If that fails, log it and return the error.
   - 565-576: if `G_output_device_index == NO_OUTPUT_DEVICE`, fall back to `Pa_GetDefaultOutputDevice()` when it's in the table and not Multus
     (copy Linux recv main.c 1101-1123). Call `Audio_Device_Error` only when there's no output device at all.
3. `udp_thread.c` 727-744, 862-890 (and the commented 1261-1264): bounds-check `G_output_device_index` / `G_digital_output_device_index` before indexing.
   A digital speaker that isn't set reopens the operator speaker (copy Linux recv udp_thread.c 764-830).
4. `extern.h`: `VERSION_MINOR` 143 → 144.

### A5. Windows ms-sdr: blank or missing COM = no CAT, no popup
File: `mscc-ui/windows-work-tree/ms-sdr-MKII/source/comm-port.c` (the Linux file `linux/ms-sdr-linux/source/comm-port.c` ~780-1000 is the model).
- 780-787: if `strstr(comm_port_record, "COMM_PORT_NAME")` or the `","` search returns NULL, or the name is empty, use `strcpy(G_comm_port, "COM0")`.
  (Today a NULL+15 dereference is possible.) Null-check the other `strstr` field pointers before `atoi` (796-799 use them unchecked). Clamp the indexes into the
  `baud_rates[]`/`parity_values[]`... tables.
- 830: `if (G_comm_port[0] != '\0' && G_comm_port[3] != '0')`.
- 664-665 (`open_comm_port`): drop the `MB_TASKMODAL` MessageBox "SERIAL PORT OPEN FAILED". Keep the log line and add
  `Gui_Add_Message("CAT PORT <name> NOT AVAILABLE - CAT DISABLED\n")` (a quiet note in the client).
- 871-876 (missing comm-port.ini): no MessageBox. Log it and continue without CAT. Optionally write the COM0 default line like the Linux file's `#else` branch.
- `VERSION_MINOR` auto-bumps on build (181 → 182). Don't hand-edit it.

### A6. WPF RX IQ on 2200 m / 630 m
File: `MSCC.Wpf/ViewModels/MainViewModel.cs` 3684-3694 `TryParseAmateurBandMeters`:
`return meters is 2200 or 630 or 160 or 80 or 60 or 40 or 30 or 20 or 17 or 15 or 12 or 10 ? meters : null;` and update the summary comment.
Nothing else is needed. `StartRxIq` (3895-3962) already handles LF: `GetAmpCalFrequencyHz` gives 2200 → 135 750 and 630 → 475 000, and LoadLastUsedForBand knows "2200m"/"630m".
`SetIqBandAsync(2200|630)` → ms-sdr `CMD_SET_IQ_BAND` → recv `Get_IQ_Record` maps IQ_B2200/IQ_B630, and iq.ini records 10/11 already exist.

### A7. Build and deploy (Windows)
1. Build SDRcore-trans (`SDRcore-trans/sdrcore-trans.sln`, Release), SDRcore-recv (`sdrcore-recv.sln`, Release), and ms-sdr-MKII (Release) the usual way.
   Copy `Mscc-trans.exe`, `mscc-recv.exe`, and `ms-sdr-MKII.exe` to `C:\mscc-net9\` and to `Release/windows-wpf` if that's where they normally land.
2. **Build the WPF client last** (`dotnet build MSCC.sln -c Release` from `mscc-new`). This runs the `ClientVersion.txt` auto-bump (M.D.I) and copies to `C:\mscc-net9`.
   **Standing rule:** always re-index or rebuild the WPF client before Stew packages from `C:\mscc-net9`, even if WPF didn't change, so the exe name and title bar show the new version.
   Record it: client M.D.I, trans 3.146, recv 3.144, ms-sdr 3.182. Firmware is Stew's, so don't touch it.

### A8. Smoke (BENCH, Windows)
1. Settings → Audio: "Line (… Multus IQ Sound)" and any other Multus/Proficio entry are missing from Out, Mic, Digital Out, and Digital Mic.
2. Mic = (none), digital = (none), speaker set, Apply. Start isn't refused and the setup line shows the mic note. The servers start, and RX audio, spectrum, and S-meter work.
   No keep-alive warning, no trans FAILED in the ms-sdr log, and the trans log shows "OUTPUT-ONLY I/Q (no mic)".
3. Same setup: TUNE and CW key and give RF (I/Q path alive). Switching Audio P/D/R doesn't crash anything (check the trans and recv logs).
3a. Mic = (none), mode LSB/USB/AM/FM: PTT button shows "Set up audio (Mic) in the Settings tab to transmit voice." PTT pops back off, no RF, and the log shows "PTT blocked".
3b. Digital Mic = (none), mode DIG-U (or Audio D): PTT shows the digital-audio pop-up and doesn't key. Set Digital Mic, and DIG-U PTT keys.
3c. With a mic set, voice PTT keys normally. Remote audio (R-Phones/R-Digital) PTT is not blocked by the local ini.
4. COM: delete or blank comm-port.ini, or pick a missing COMx. Start works with no modal popup, and the log says CAT disabled. A real COM port still opens CAT.
5. Pick a mic again: SSB TX audio works as before (no regression).
6. Geminus on 2200 m and on 630 m: the RX IQ tab START opens a session (no INVALID BAND), APPLY commits, and the recv log shows the band 2200/630 record. HF bands are unchanged.
7. Existing `%LocalAppData%\MSCC-NET9` with old iq.ini/power_cal.ini/last-used files still loads (no format change).

## Part B: Linux (ubuntu-stew), separate Build run after Part A
### B1. SDRcore-trans-linux: start when no mic is configured
File: `linux/SDRcore-trans-linux/sources/main.c`
- 1103-1109: when `G_input_device_index == NO_INPUT_DEVICE`, log "No operator mic set: TX voice off" and skip the mic block. Don't MessageBox and don't `goto error`.
- 1118-1129 digital fallback: only copy the operator record when the operator index is valid. **Today it writes `G_digital_input_devices[100]` out of bounds**
  if the op mic is missing.
- 1147-1196: when there's no mic, skip the `inputParameters` checks and open `manage_stream(1, -1, 2)` (output-only I/Q already exists). Keep the I/Q device checks.
- Optional, small: `udp_thread.c` 520-527 / 571-577 / 597-603. When `op_idx` is invalid, use output-only (`manage_stream(0,0,2); manage_stream(1,-1,2)`)
  and still set `G_audio_mode`, so REMOTE (MSA1 mic) and DIGITAL with a valid digi mic still work. Today it "aborts the switch". That's safe, but it blocks those modes.
- `extern.h` `VERSION_MINOR` 141 → 142.
- Already behaves (no change): Linux recv (blank key, Multus skip, default-output fallback, bounds checks) and Linux ms-sdr COM (invalid/missing ini → PTY default,
  non-fatal, COM0 = PTY).
### B2. Avalonia RX IQ on 2200 m / 630 m
`mscc-ui/Avalonia-Migration/src/MSCC.Avalonia/ViewModels/MainViewModel.cs` 4225-4237: add `2200 or 630` to the `meters is ...` list. Nothing else is needed
(`GetCalFrequencyHz` 3498 already has 2200/630, `CalBandNumbers`/`TxIqBandNumbers` include them, and the LF band buttons are gated to Geminus).
Optional: `Services/LocalServerLauncher.cs` 60-61. Drop `comm-port.ini` from `MissingSetupItems()` (ms-sdr writes the PTY default), so a missing COM file doesn't trigger
the "not set up" alert.
### B2b. Avalonia: same PTT block (local servers only)
File: `mscc-ui/Avalonia-Migration/src/MSCC.Avalonia/ViewModels/MainViewModel.cs`. The entry point is `PttOn` → `OnPttOnChanged` (1027-1040) → `SendPttAsync` (1076+).
The PTT button (`TogglePtt` 1022) and remote CAT `cat.SetPtt` (2409) set `PttOn`.
Avalonia **can** tell whether the mic is set only when the servers run on this machine (`LaunchServers && IsLocalHost(Host)`), by reading `~/.local/mscc/*.ini` via
`RemoteAudio/LinuxDigitalIni.cs` (`OperatorMic`, `DigitalMic`). It can't see the PortAudio device list, so the check is "ini non-empty" only.
For a remote host (Pi), the client can't know, so skip the check.
1. Helper:
   ```csharp
   private string? TxAudioNotSetReason()
   {
       if (!(LaunchServers && IsLocalHost(Host)) || RemoteAudio) return null;
       string m = (ModeText ?? "").Trim().ToUpperInvariant();
       if (TuneMode || m is "CW" or "TUNE") return null;
       bool digital = IsDigitalAudio || ModeIsDigU;
       string key = digital ? LinuxDigitalIni.DigitalMic : LinuxDigitalIni.OperatorMic;
       if (!string.IsNullOrWhiteSpace(key)) return null;
       return digital
           ? "Set up digital audio (Digital Mic) in MSCC Init / Settings to transmit in digital modes."
           : "Set up audio (Mic) in MSCC Init / Settings to transmit voice.";
   }
   ```
2. In `OnPttOnChanged`, after `if (!CanOperate() || TxSetByServer) return;` (1037):
   ```csharp
   if (value && TxAudioNotSetReason() is string why)
   {
       AppendLog("PTT blocked: " + why); StatusText = why;
       Dispatcher.UIThread.Post(async () => { PttOn = false; await MsccDialog.AlertAsync(why); });
       return;
   }
   ```
   Wording: on Linux the mic is picked in MSCC Init. Keep "Settings" if Avalonia has an Audio settings page Stew prefers to point at.

### B3. Versions and build (stew-HP)
- `linux/mscc-deb/packaging/DEBIAN/control` 1.0.50 → **1.0.51**. `MSCC.Avalonia.csproj` `<Version>` and `packaging/mscc-ui/DEBIAN/control` 0.6.72 → **0.6.73**.
- `./linux-build/build-mscc-deb-amd64.sh` (ms-sdr auto-bumps `version.h` from 176; record the final number). Then `rm -rf mscc-ui/Avalonia-Migration/publish/linux-x64-sc &&
  ./linux-build/mscc-ui-x64.sh && ./linux-build/build-mscc-ui-deb-amd64.sh`. Check that installers/linux has only mscc_1.0.51 and mscc-ui_0.6.73 (old ones replaced).
- `installers/linux/CHANGELOG.md`: new section `## mscc 1.0.51 / mscc-ui 0.6.73 — <date>`. Fixed: trans starts with no mic set (I/Q TX only); RX IQ on 2200 m / 630 m.
### B4. Smoke (stew-HP)
1. Empty `~/.local/mscc/operator-microphone.ini`, then Start. trans stays up (log: output-only I/Q), and RX works. TUNE gives RF. No MSCC stop / keep-alive loss.
1a. Mic ini empty, mode USB: PTT shows the pop-up and doesn't key. DIG-U with digital-microphone.ini empty: digital pop-up. TUNE/CW still key.
2. Mic restored: SSB TX audio works. 3. RX IQ START on 2200 m and 630 m (Geminus) opens. 4. Old ini files load.

## Caveats for the PTT block (client-side only)
- Keying that doesn't go through the client's `PttOn` is **not** blocked and still goes out silent with no mic. That covers the radio's rear PTT/footswitch, ms-sdr CAT
  (WSJT-X/fldigi on the local COM/PTY pair), and anything the server reports via 0xBC (`TxSetByServer`). Blocking those needs a server-side check (trans/ms-sdr refuse voice TX with no mic).
  That's out of scope here. Ask Stew if he wants it later.
- Remote CAT (`RemoteCat`, WSJT-X on a remote PC) keys through `PttOn`, but only with Remote audio on, which the check skips by design.
- Avalonia keyer-memory play in a non-CW mode (MainViewModel.cs ~3081-3092) sets `PttOn` with `_suppressTransmitCommands` and calls `SendPttAsync(true)` directly, so it bypasses the check.
  Leave it as is (keyer memories are CW-oriented). Note it in the review.
- WPF with servers on another host (`LaunchServersOnStart` off): no check, because the mic config lives on that host.

## What NOT to touch
`rpi/`, `Solidus/`, `installers/rpi/`, firmware, the ini formats/record order (no band inserts, no array resizes), the Multus I/Q device selection in the servers,
and full-name device storage. **Optional idea, not in this cmd:** store full device names instead of the "(" prefix (fixes "Line " ambiguity). Ask Stew first.

## Report
Diffs, client M.D.I, trans/recv/ms-sdr versions (Windows), mscc/mscc-ui deb versions and the final ms-sdr minor (Linux), smoke results 1-7 / 1-4. STOP, no commit/push.
