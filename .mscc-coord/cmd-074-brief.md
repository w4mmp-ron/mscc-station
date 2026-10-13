# cmd-074: TX ALC always on (button removed) + volume / mic-gain sliders saved (Ron's Pi c0d26a1 + 85b694c)

Host for Part A: **windows-new-hp**. Repo: `C:\Users\n8vet\OneDrive\Documents\GitHub\mscc-station`, at `0dff8fe` (clean, up to date).
Part B is for **stew-HP (Ubuntu)**, to be run later as a separate Build session.
Background: `.mscc-coord/NOTE-FOR-STEW-ALC-BUTTON-2026-10-12.md`. Ron's Pi commits `c0d26a1` (trans: ALC always on) and `85b694c` (ms-sdr: deferred save of volume / mic gain).

Scope:
- **WPF:** remove the TX ALC button.
- **Windows SDRcore-trans:** ignore ALC-off.
- **Windows ms-sdr-MKII:** save the volume and mic-gain sliders.
- Versions, CHANGELOG, status.

**Do NOT edit `rpi/`, `Solidus/`, or `installers/rpi/`** (read only).
Don't touch: MSCC.Core (`SetAlcOnAsync` and `CMD_SET_ALC_MULTIPLIER` stay, because Avalonia still uses them), Avalonia, recv, the ALC algorithm (`doALC`), the ALC meter, the protocol, firmware.
Stew: "Lets not go too crazy with the code changes." Small changes, no refactors.
**STOP BEFORE COMMIT.** Show `git status`, `git diff --stat`, and the full diff of each source file.
Line numbers are from `0dff8fe`; check them before you edit.

---

## Part A1. WPF: remove the TX ALC button
**Today:** an ALC toggle on the TX tab (`MainWindow.xaml:1197-1213`) flips `AlcOn` (`MainViewModel.cs:280-281`, default true). `OnAlcOnChanged` (4820) then sends 0x23 1/0 via `SetAlcOnAsync`. `AlcOn` is not persisted in the WPF client ini and is used nowhere else in WPF.

1. **`MSCC.Wpf/MainWindow.xaml` lines 1197-1213:** delete the whole `<Button Grid.Row="1" Grid.Column="1" Content="ALC" … </Button>` element. That runs from `<Button Grid.Row="1" Grid.Column="1" Content="ALC"` to its closing `</Button>`, just before `</Grid>`. Leave the grid's row/column definitions alone; the cell just stays empty.
2. **`MSCC.Wpf/ViewModels/MainViewModel.cs`:**
   - Lines 280-281: delete `[ObservableProperty]` + `private bool _alcOn = true;`.
   - Line 1944: delete `ToggleAlcCommand = new RelayCommand(() => AlcOn = !AlcOn);`.
   - Line 3669: delete `public IRelayCommand ToggleAlcCommand { get; }`.
   - Line 4820: delete `partial void OnAlcOnChanged(bool value) { … }`.
3. Check: `git grep -n -w -E "AlcOn|ToggleAlcCommand|_alcOn" -- mscc-ui/windows-work-tree/mscc-mscc/mscc-new/src/MSCC.Wpf` returns **nothing**. Leave MSCC.Core `SetAlcOnAsync` / `IRadioService` as they are (Avalonia uses them).
   With the button gone, the WPF client never sends 0x23. trans starts with ALC on (its default), and Part A2 makes that permanent.

## Part A2. Windows SDRcore-trans: ALC always on (Pi `c0d26a1`)
`mscc-ui/windows-work-tree/SDRcore-trans/sources/udp_thread.c` lines 539-545: replace
```c
            case CMD_SET_ALC_MULTIPLIER:
                if (t_opcode_data == 0) {
                    G_Do_ALC = FALSE;
                } else {
                    G_ALC_multiplier = (float) 11.0 / 10.0f;
                    G_Do_ALC = TRUE;
                }
```
with
```c
            case CMD_SET_ALC_MULTIPLIER:
                //ALC is always on. The off value (0) was for testing and is ignored.
                G_ALC_multiplier = (float) 11.0 / 10.0f;
                G_Do_ALC = TRUE;
```
The log line that follows stays as it is.
This protects older clients and Avalonia, which can still send 0.

**Version:** `SDRcore-trans/sources/extern.h:37` `#define VERSION_MINOR 146` → `147` (trans **3.147**).

## Part A3. Windows ms-sdr-MKII: save volume / mic gain a few seconds after the last slider step (Pi `85b694c`)
**Today:** `source/user_controls.c` cases `CMD_SET_PHONES_VOLUME_LEVEL` (1026), `CMD_SET_PHONES_MIC_GAIN_LEVEL` (1038), `CMD_SET_DIGITAL_VOLUME_LEVEL` (1051) and `CMD_SET_DIGITAL_MIC_GAIN_LEVEL` (1063) set `update_flag = FALSE`. So the values are applied but **never written** to `user_controls.ini`, and after a host restart the old levels come back. (`update_flag = FALSE` stays, so a slider drag doesn't write the file on every step.)

1. **`source/extern.h`**, after line 514 (`extern int User_Controls_Update_Init_File();`):
   ```c
#define USER_CONTROLS_SAVE_DELAY 2 //Flusher_thread passes (3 s each) after the last volume / mic gain step
extern uint8_t G_User_Controls_Save_Pending;
   ```
   (On the Pi this is `3` because its Flusher loop runs every 1 s. The Windows Flusher loop runs every 3 s (`Sleep(3000)`, `main.c:439`), so `2` gives a write 3-6 s after the last step.)
2. **`source/user_controls.c`**, directly before `int User_Controls_Update_Init_File() {` (line 699):
   ```c
/* Volume and mic gain sliders send many steps, so those commands do not write the file.
 * They set this count and Flusher_thread writes the file once, that many passes after
 * the last step. */
uint8_t G_User_Controls_Save_Pending = 0;

   ```
3. In each of the four cases (1026, 1038, 1051, 1063), add **one line directly before** `update_flag = FALSE;`:
   ```c
                G_User_Controls_Save_Pending = USER_CONTROLS_SAVE_DELAY;
   ```
4. **`source/main.c` `Flusher_thread`**, inside `if (G_Allow_Log_Write == TRUE) {`, directly after `Sleep(3000);` (line 439):
   ```c

            /* Volume / mic gain sliders: write user_controls.ini once, after the last step. */
            if (G_User_Controls_Save_Pending > 0) {
                G_User_Controls_Save_Pending--;
                if (G_User_Controls_Save_Pending == 0) {
                    User_Controls_Update_Init_File();
                }
            }
   ```
   (It has to sit inside that branch, because the loop only sleeps there. `G_Allow_Log_Write` is only briefly FALSE while the log is reset, `main.c:387-393`.)

**Version:** ms-sdr `source/version.h` `VERSION_MINOR` is **auto-incremented by the PreBuildEvent** (`increment-version.ps1`): 3.182 → **3.183** for one build. Don't hand-edit it; report what the build gives.

## Part A4. Build, versions, CHANGELOG (Windows)
1. Stop MSCC and its servers.
2. Build **SDRcore-trans** (MFC|Win32, v143, same recipe as recv in cmd-071/073b) and copy `Mscc-trans.exe` to `C:\mscc-net9` (and the tracked Release copy under `mscc-ui/Release/windows-wpf/`, if trans has one there; follow where the existing `Mscc-trans.exe` came from).
3. Build **ms-sdr-MKII** (its usual solution/config). Copy `ms-sdr-MKII.exe` to `C:\mscc-net9` (and the Release copy, if tracked).
4. **WPF:** `dotnet build MSCC.sln -c Release` from `mscc-ui/windows-work-tree/mscc-mscc/mscc-new`. This bumps the client version and copies to `C:\mscc-net9`.
   **Client version:** the bump script is date-based (month.day.index). The first build on 2026-10-13 gives **10.13.0**, not 10.11.4. Use what the build prints; **don't hand-edit `ClientVersion.txt`**. The installer name, title bar and CHANGELOG all follow that number.
5. Check:
   - `C:\mscc-net9` has today's `Mscc-trans.exe`, `ms-sdr-MKII.exe` and `MSCC.Wpf.exe`, each identical to its Release copy if there is one.
   - `MSCC.Wpf.exe` ProductVersion = `ClientVersion.txt`.
   - recv (3.147) is unchanged.
   - The logs show trans 3.147 and ms-sdr 3.183 on first start.
6. `installers/windows/CHANGELOG.md`: a **new section above R10.11.3**, same heading style:
   ```
   ## R10.13.0 - 2026-10-13  (`mscc-net9-R10-13-0-install.exe`, title bar 10.13.0)

   ### Changed
   - TX ALC is always on. The ALC button is gone from the TX tab; the transmit server ignores an "ALC off" from older clients.
   - Volume and mic gain (phones and digital): the slider settings are now saved on the host a few seconds after you stop moving them, and come back after a restart.
   - Transmit server 3.147, ms-sdr 3.183 (Windows).
   ```
   (Use the real client / ms-sdr numbers.)
7. Update `.mscc-coord/status/windows-new-hp.md` (cmd-074 row: trans 3.147, ms-sdr 3.183, client 10.13.0, not committed).
8. Stew packages `mscc-net9-R10-13-0-install.exe` from `C:\mscc-net9` after the smoke test (not part of this command).

## Part A5. Smoke (NEW-HP; Stew)
1. **No ALC button** on the TX tab, and the layout around it looks fine.
2. **ALC acts:** SSB voice (or a loud digital tone in USB) driven hard. The ALC meter moves and the output is limited, exactly as with ALC on before. The trans log shows `CMD_SET_ALC_MULTIPLIER … G_Do_ACL: 1` if anything still sends 0x23.
3. **Volume saved:**
   - Set phones volume and phones mic gain to unusual values (e.g. 37 / 63); do the same for digital volume / mic gain if that device is used. Wait about 10 s.
   - `%LocalAppData%\MSCC-NET9\user_controls.ini` shows the new values.
   - Stop MSCC (host and client), restart: the sliders and the audio level come back as set.
   - A slider drag writes the file once (ms-sdr log), not on every step.
4. **Normal operation:** RX, SSB TX, TUNE, CW keying, band change. Power and the ALC meter are as before.
5. cmd-068..073b still good: spectrum, the S-meter needle and dBm toggle work; the title shows the new version.

---

## Part B (later, stew-HP Ubuntu): same server changes
Run as a separate Build session on stew-HP, after Part A passes. Same don't-touch list. Avalonia is **not** changed here: its ALC button keeps sending 0x23, which trans now ignores. Removing the Avalonia button is a later, optional Avalonia brief.

**B1. `linux/SDRcore-trans-linux/sources/udp_thread.c` lines 662-668:** the same replacement as A2 (identical code there).
Version: `linux/SDRcore-trans-linux/sources/extern.h:33` `VERSION_MINOR 142` → `143` (trans **3.143**).

**B2. `linux/ms-sdr-linux/source`.** The gap is confirmed: the same four cases set `update_flag = FALSE` with no save (`user_controls.c` 1030 / 1042 / 1055 / 1067). Ubuntu's `Flusher_thread` has the Pi structure (`Sleep(1000)` at `main.c:484`), so take Ron's `85b694c` hunks **verbatim**:
1. `extern.h`, after line 550 (`extern int User_Controls_Update_Init_File();`):
   `#define USER_CONTROLS_SAVE_DELAY 3 //seconds after the last volume / mic gain step` and `extern uint8_t G_User_Controls_Save_Pending;`
2. `user_controls.c`, before `int User_Controls_Update_Init_File() {` (line 704): the comment and `uint8_t G_User_Controls_Save_Pending = 0;` as in A3.2 (Pi wording: "that many seconds after the last step").
3. The same one-line `G_User_Controls_Save_Pending = USER_CONTROLS_SAVE_DELAY;` before `update_flag = FALSE;` in the four cases.
4. `main.c`, directly after `Sleep(1000);` (line 484), before `if (G_Allow_Log_Write == TRUE) {`: the Pi block (same as A3.4, at that indentation).
5. Check: `git diff --no-index --ignore-cr-at-eol` of the three files against `rpi/ms-sdr-linux/source/` shows no difference in these hunks.

ms-sdr's version is auto-bumped by `make` (`bump-version.sh`): 3.180 → 3.181.

**B3. Build and packaging:** as in `.mscc-coord/cmd-073-brief.md` (make trans + ms-sdr, the new `mscc_1.0.x_amd64.deb`, `installers/linux/CHANGELOG.md` section, `.mscc-coord/status/ubuntu-stew.md`).
Smoke: the same A5 steps 2-4 on stew-HP with Avalonia. The ALC button still shows there, but ALC stays on either way.

STOP. No commit, no push.
