# Build brief - cmd-047a: Avalonia 0.6.64 UI nits (10m button, stray "Band:" label) + FREQ CAL fixes left over from cmd-047 (paste-ready for Grok Build)

**Host:** ubuntu-stew only (stew-HP-Notebook, `/home/stew/Documents/GitHub/mscc-station`).
**Call the user Stew.** Stew is not a Linux person: plain copy-paste commands, one per line.
**Commit locally when done. Do NOT git push and do NOT git pull.**
**Edit only** `mscc-ui/Avalonia-Migration/` (source + `packaging/mscc-ui/DEBIAN/control`), the kit outputs (`mscc-ui/Release/avalonia/x86_64/`, `installers/linux/`) and `.mscc-coord/status/ubuntu-stew.md`.
**Testing is LOCAL ONLY** (this laptop, Host 127.0.0.1, radio on USB). No remote host, no Pi kit.
**Do NOT touch:** `rpi/` (Ron's folder), `linux/` servers, WPF (`MSCC.Wpf`), **MSCC.Core**, mscc-init, firmware, the RPi Grok Build app.
**Leave alone:** the uncommitted build leftovers under `linux/tty0tty-master/module/` (`.*.cmd`, `tty0tty.mod.c`). Do not stage, commit, clean or revert them. Never run `sudo` yourself; give Stew the line.

**Version: 0.6.64** (not 0.6.62: 0.6.62 and 0.6.63 were already used by the cmd-047 follow-up commits `80297ff` / `8999f9a`, and their debs exist; apt needs a higher number).

Roots (stew-HP):
- AVA = `mscc-ui/Avalonia-Migration/src/MSCC.Avalonia/`
- WPF reference (read only) = `mscc-ui/windows-work-tree/mscc-mscc/mscc-new/src/MSCC.Wpf/`

AVA line numbers were checked at `8999f9a` (Avalonia 0.6.63). They shift as you edit: anchor on the quoted text.

---

## 0. Start

```bash
cd /home/stew/Documents/GitHub/mscc-station
git status -sb
git log --oneline -5
dpkg -l mscc mscc-ui | tail -2
```

- **HEAD gate:** top commit = `coord: cmd-047a Avalonia UI nits + FREQ CAL fixes`, parent `8999f9a` (cmd-047: Avalonia 0.6.63 FREQ CAL keeps QRP/AMP/TX IQ clickable), then `80297ff`, `05b6725`, `f89a1bd`. None of these are pushed ("diverged" from origin is expected; do not pull). If HEAD is anything else, stop and tell Stew.
- **Close-X / Auto fix:** already committed (`05b6725` "stop Launch-owned servers on window close (X)" + `80297ff` 0.6.62 bump). Nothing to commit first. Keep that behaviour (`KickOwnedServerStop()` at the top of `OnClosing` and `PrepareForCloseAsync`, `ArgumentList` in `StartDetachedStop`). Do not undo it.
- **Allowed dirt:** only the tty0tty build leftovers under `linux/tty0tty-master/module/`. Anything else modified: stop and tell Stew.
- Expected installed: mscc 1.0.47, mscc-ui 0.6.63.
- ACK `cmd-047a` as `accepted`, then `running`, in `.mscc-coord/status/ubuntu-stew.md`. **No bare pipe characters inside table cells** (write `abs(N)` or `\|`).

---

## 1. UI nit: 10m band button is narrow (match WPF: same width as the other band buttons)

**Cause (found):** band bar grid, `AVA/Views/MainWindow.axaml:266`:

```xml
<Grid ColumnDefinitions="Auto,Auto,*,*,*,*,*,*,*,*,*,Auto,Auto" ColumnSpacing="2">
```

That is 13 columns (0-12) with only **9** star columns (2-10). The 10 HF buttons 160...10 sit in columns 2-11, so **10m (Grid.Column="11") lands in an `Auto` column** and shrinks to its text. It is not a Width/MinWidth/style issue (the 10 button has the same attributes as 160-12).

This came from cmd-047 section 7 (radio-model button removal): the old grid was `Auto,Auto,Auto,*,*,*,*,*,*,*,*,*,*,Auto,Auto` (3 Auto + **10** star + 2 Auto = 15). Removing the button should have dropped one `Auto`; it dropped one `Auto` **and** one `*`.

**Fix:** 2 LF Auto + 10 HF star + 1 GEN Auto (the band label goes, see section 2):

```xml
<Grid ColumnDefinitions="Auto,Auto,*,*,*,*,*,*,*,*,*,*,Auto" ColumnSpacing="2">
```

WPF (`MainWindow.xaml` ~:635-646) uses fixed `Width="40"` for 630 and 160...10 and `Width="48"` for 2200 and the GEN/USER button. Avalonia's stretch layout is fine as long as all ten HF buttons get equal width; don't switch to fixed widths.

## 2. UI nit: green "Band: 20m" text on top of the USER button (match WPF: no label)

**What it is (found):** `AVA/Views/MainWindow.axaml:340-341`:

```xml
<TextBlock Grid.Column="13" Text="{Binding BandText, StringFormat=Band: {0}}"
           VerticalAlignment="Center" Margin="6,0,2,0" Foreground="{DynamicResource UiAccentBrush}" FontSize="11"/>
```

It is a real (not debug) Avalonia-only band label bound to `BandText`, in the accent colour (green). Before cmd-047 it had its own `Auto` column 14. After cmd-047 the grid has no column 13, so Avalonia clamps it into the **last** column (12), the GEN button (`USER`), and it draws over the button: "B...20m" = `Band: 20m`.

**WPF has no such label**: its band bar is 2200, 630, 160...10, then the GEN button (`USER`), nothing after it.

**Fix:** delete that `TextBlock` (lines 340-341) and update the grid comment at :264 to `<!-- Band bar: LF | HF | GEN (FW major gates HF/LF) -->`. **Keep** the `BandText` property: the ViewModel uses it for band logic and last-used saves (`SaveLastUsed`, band recall). Don't remove it or its setters.

Check the bar at 1024 px wide: all ten HF buttons equal width, USER button readable, nothing overlapping.

---

## 3. FREQ CAL fixes (review of `f89a1bd`; none of these were touched by `05b6725` / `80297ff` / `8999f9a`)

### 3.1 Bug 1: Disconnect then Connect with the FREQ CAL tab open doesn't go back to CW

**Cause:** `Disconnect` → `PrepareDisconnectAsync` (VM :1478-1498) calls `LeaveFreqCalTab(sendToRadio: IsConnected)` when `_freqCalEntryHeld`. `LeaveFreqCalTab` (VM :5362) starts with `_freqCalTabActive = false;`, so after Connect the `ModeReported` handler (VM :7059 `if (_freqCalTabActive && !_freqCalHoldingCw && IsConnected) _ = EnterFreqCalTab();`) never re-applies CW, though the tab is still selected. (cmd-047 brief 4.1 said keep `_freqCalTabActive` when the tab stays selected.) Also note the 300 ms `WaitAsync` bound: the flag must be right even if the wait times out.

**Fix:**
- `LeaveFreqCalTab(bool sendToRadio = true, bool tabStillOpen = false)`: set `_freqCalTabActive = tabStillOpen;` (first line, replaces `= false`).
- `PrepareDisconnectAsync`: call `LeaveFreqCalTab(sendToRadio: IsConnected, tabStillOpen: !closeWindow)`. The tab-change handler and `Dispose` keep the default (`false`).
- Disconnect with the tab open but **not** holding (mode was unknown, `_freqCalEntryHeld == false`) must also keep `_freqCalTabActive` (today it does, since Leave isn't called; keep it that way).
- On the first `ModeReported` after Connect with the tab still open, `EnterFreqCalTab()` runs (existing :7059). Add the log line ` Freq Cal: Start with tab open, re-applied CW/600/200` there (cmd-047 brief 4.1 asked for it; it is missing).

### 3.2 Bug 2: FREQ CAL with VFO B active checks and changes VFO A's mode and label

**Cause:** `ForceCwForFreqCalAsync` (VM ~:5125-5193, called by `EnterFreqCalTab` and at every AUTO/CHECK start) reads `string currentMode = (ModeText ?? "").Trim();` (VFO A) and writes `ModeText = "CW"; NotifyModeFlags();` (VFO A label). With VFO B active: VFO A's label turns CW, and at AUTO/CHECK start (tab already CW on B) it compares VFO A's mode, re-sends CW and saves A's mode into `_modeBeforeFreqCal`.

**Fix:** use the active VFO everywhere in `ForceCwForFreqCalAsync`:
- `string currentMode = (ActiveModeString ?? "").Trim();` (`ActiveModeString` VM :1091).
- After `SetModeAsync("CW")`: `if (UseVfoA) { ModeText = "CW"; NotifyModeFlags(); } else { VfoBModeText = "CW"; }` then `RefreshSpectrumFilterOverlay()` and `SyncRfPowerFromMode()` (CW bank).
- `EnterFreqCalTab` already snapshots the active VFO (`_freqCalModeSaved`, `_freqCalSavedVfoA`) and `LeaveFreqCalTab` restores the right label; keep that.
- If the user switches VFO while the tab holds CW, don't add new behaviour; just make sure nothing writes the other VFO's label.

### 3.3 Bug 3: deferred restore is not cancelled when you come back to the tab mid-run

**Cause:** leaving FREQ CAL during AUTO/CHECK calls `DeferFreqCalRestore()` (`_freqCalRestorePending = true`). Coming back to the tab before the run ends calls `EnterFreqCalTab()`, which returns early (`_freqCalHoldingCw` is still true) and never clears the flag. When 0x62 arrives, `OnFreqCalStatusReported` (VM :5262-5266) runs `LeaveFreqCalTab(sendToRadio: true)` while the tab is open: mode goes back to USB etc. on the FREQ CAL tab and `_freqCalTabActive` goes false.

**Fix:** first lines of `EnterFreqCalTab()`: `if (_freqCalRestorePending) { _freqCalRestorePending = false; AppendLog("Freq Cal: deferred restore cancelled (tab re-entered)"); }` then `_freqCalTabActive = true;` as today.

### 3.4 Minor items

a. **Stale `_modeBeforeFreqCal` makes every Disconnect show STOPPED.** `ForceCwForFreqCalAsync` sets it (VM :5135); the only reader that clears it, `RestoreModeAfterFreqCalAsync` (:5195-5222), has **no caller** since cmd-047 (the tab owns the restore). `ForceStopFreqCal` (:5288-5291) returns early only when `_modeBeforeFreqCal == null`, so once set, every Disconnect runs the forced stop and shows `STOPPED`. **Fix:** delete the `_modeBeforeFreqCal` field (:86), its assignment in `ForceCwForFreqCalAsync`, the dead `RestoreModeAfterFreqCalAsync`, and the `&& _modeBeforeFreqCal == null` term in the `ForceStopFreqCal` guard. A plain Disconnect with no cal activity must not touch the FREQ CAL status.

b. **Servers marked "ours" when no start command was found.** `EnsureLocalServersAsync` (VM ~:1397-1460): the `if (before < 3) { _serversOurs = true; ... }` block runs even on the `mscc not installed` branch. **Fix:** remember `bool startRan = ctl != null || mscc != null;` and mark ours only `if (startRan && before < 3)`. Without a start command log ` Launch: no start command - not ours` and leave `_serversOurs = false`.

c. **Idle text says "servers start automatically" even for a remote Host.** The constructor sets it unconditionally (VM :172) before `LoadClientSettings()` (:191); Disconnect (:1371-1373) already checks. **Fix:** one helper `string IdleStatusText() => LaunchServers && IsLocalHost(Host) ? "Disconnected — press Connect (servers start automatically)." : "Disconnected — MSCC Start, then Connect.";` Use it after `LoadClientSettings()` in the constructor, in Disconnect, and in `OnHostChanged` (:6823) / `OnLaunchServersChanged` (:6832) when `!IsConnected && !IsBusy` (only replace the text if it is currently one of the two idle texts, so error texts are not wiped).

d. **"Launch: server alive (first keep-alive)" on every Connect.** `_keepAlivesReceived` is reset on every Connect (:1265) and the log at :6989 fires whenever `n == 1`. **Fix:** add `_launchStartedThisConnect` (set true in `EnsureLocalServersAsync` when the start ran and servers are marked ours, false at the start of each Connect) and log only `if (n == 1 && _launchStartedThisConnect)`. The cold-start grace (:7423) is unchanged.

e. **"VFO B saved" log missing; no guard against saving CW as VFO B's mode.** In the settings builder (VM ~:6600-6609, `LastVfoBMode = ...`):
   - If the FREQ CAL tab holds CW on B (`_freqCalHoldingCw && !_freqCalSavedVfoA`) write `_freqCalModeSaved` as B's mode; else if `!UseVfoA && IsCalibrationOrIqSessionActive()` write the last saved B values (`_lastSavedVfoBHz` / `_lastSavedVfoBMode`). Same idea for A: if the tab holds CW on A (`_freqCalHoldingCw && _freqCalSavedVfoA`) write `LastMode = _freqCalModeSaved` (like the existing `CwFilterIndex` line).
   - After building, if B's freq or mode differs from `_lastSavedVfoBHz` / `_lastSavedVfoBMode`: log ` VFO B saved: f=<hz> mode=<mode>` and update both fields (only on change, not every save). Update them on the UI thread (the builder runs there; the write is off-thread).

f. **Tooltips** (`AVA/Views/MainWindow.axaml`):
   - NR (:1559) `Noise reduction on/off (level = NR slider)` → `Noise reduction on/off` (WPF :2280; cmd-047 B19).
   - MANUAL − (:1226) `PPM −1 (rate-limited)` → `Decrease PPM by 1 (rate-limited to protect radio)`; MANUAL + (:1234) `PPM +1 (rate-limited)` → `Increase PPM by 1 (rate-limited to protect radio)` (WPF :1902 / :1907).

g. Leave as is: 8999f9a made QRP CAL / AMP CAL / TX IQ tabs clickable during a FREQ CAL run and shows the `FREQUENCY CALIBRATION IN PROGRESS.` popup from the tab handler (Stew's decision). Don't revert it.

---

## 4. Status history: restore what cmd-047 deleted from `.mscc-coord/status/ubuntu-stew.md`

`f89a1bd` rewrote the file and dropped history. Restore it from git (read with `git show`, don't check out the old file over the new one):

```bash
git show 231d521:.mscc-coord/status/ubuntu-stew.md
git show 519c682:.mscc-coord/status/ubuntu-stew.md
```

- Put the **cmd-032** row back at the bottom of the ACK table: `| cmd-032 | done | mscc_1.0.44_amd64.deb with remote_mic stream reset |`.
- cmd-043 row: restore the full note from `231d521` (smoke 1 pass, 2-8 waiting on Stew, commit 519c682, no pull, no push) and add `CAT works after 1.0.47 install (see Notes).`. cmd-042 row: restore `(no 1.0.45 amd64)`.
- Add a `### cmd-043 (history)` block under Notes with the Notes lines from `519c682` (deb path, recv/trans 3.141 + ms-sdr 3.171, x86-64 only, sudo note, first smoke without radio) and from `231d521` (smoke 1 with radio, 40 ms digital latency, CAT update 2026-09-27).
- Keep the cmd-047 row and cmd-047 Notes as they are now (0.6.63 local smoke pass), under a `### cmd-047` heading. Add cmd-047a on top of the table and a `### cmd-047a` notes block.
- No bare `|` in cells.

---

## 5. Version 0.6.63 → 0.6.64

`MSCC.Avalonia.csproj` `<Version>`, `MainViewModel.cs` `_clientVersionText` (:469) and the startup log line (:183, e.g. `MSCC Avalonia 0.6.64 — 10m button + band label fix, FREQ CAL VFO B / reconnect / deferred-restore fixes.`), `packaging/mscc-ui/DEBIAN/control` `Version:`.

## 6. Build and install (amd64)

```bash
cd /home/stew/Documents/GitHub/mscc-station
~/.dotnet/dotnet build mscc-ui/Avalonia-Migration/src/MSCC.Avalonia/MSCC.Avalonia.csproj -c Release
./linux-build/mscc-ui-x64.sh
./linux-build/build-mscc-ui-deb-amd64.sh
ls installers/linux/ mscc-ui/Release/avalonia/x86_64/ | grep mscc-ui_0.6.6
```

- No new errors or warnings.
- **Always run `mscc-ui-x64.sh` before the deb script** (the deb script reuses an old publish otherwise and would package 0.6.63 again).
- Expected: `installers/linux/mscc-ui_0.6.64_amd64.deb` and `mscc-ui/Release/avalonia/x86_64/mscc-ui_0.6.64_amd64.deb`.
- Stew installs:

```bash
sudo apt install ./installers/linux/mscc-ui_0.6.64_amd64.deb
```

## 7. Smoke for Stew (LOCAL ONLY, short; one at a time; write down what he says)

Before: `dpkg -l mscc-ui | tail -1` shows 0.6.64. Title shows **0.6.64**.

1. **Band bar:** the **10** button is as wide as 12, 15, 17 and 20. Make the window narrower and wider: 10 stays the same width as the others.
2. **No green text on USER:** the USER button is clean (no "Band: 20m" over it). Change band: still nothing over USER. Click USER a few times: WWV / CHU / RWM / USER rotate and read clearly.
3. **Reconnect with the tab open (bug 1):** in USB, open FREQ CAL (CW / 200 / 600). Press **Disconnect**, then **Connect** with FREQ CAL still open: it goes to **CW / 200 / 600 by itself**. Go to MAIN: **USB** with your old filter and pitch.
4. **VFO B (bug 2):** VFO A on 7.200 LSB, VFO B on 10.000 USB (WWV). Click VFO **B**. Open FREQ CAL: **VFO B** shows CW, **VFO A still shows LSB**. Run **CHECK**: VFO A still LSB. Go to MAIN: VFO B back to USB, VFO A LSB.
5. **Deferred restore (bug 3):** on FREQ CAL press **AUTO**, go to MAIN (allowed), then come back to FREQ CAL before it finishes. When it finishes you are **still in CW 200/600** on the FREQ CAL tab. Go to MAIN: your old mode comes back.
6. **No false STOPPED (minor a):** open FREQ CAL, run a CHECK, go to MAIN. Disconnect and Connect: the FREQ CAL status does **not** say STOPPED (it shows the last result or OK).
7. **Idle text (minor c):** Disconnected, type another IP in Host: the status says `Disconnected — MSCC Start, then Connect.` Put 127.0.0.1 back: `... press Connect (servers start automatically).`
8. **Server alive once (minor d):** with the servers already running (`mscc start`), Connect and Disconnect twice. The MSCC log has no `Launch: server alive` lines for those connects.
9. **VFO B saved (minor e):** change VFO B's frequency, wait 2 s: the log has one `VFO B saved: f=... mode=...`. With VFO B active, open FREQ CAL, close MSCC UI with the X, reopen: VFO B shows its old mode (not CW).
10. **Tooltips:** NR says `Noise reduction on/off`. MANUAL − / + say `Decrease PPM by 1 ...` / `Increase PPM by 1 ...`.
11. **Close-X still stops our servers:** with Launch ticked and servers stopped (`mscc stop`), Connect, then close with the X, wait 10 s, `mscc status`: stopped.

Radio not available? Do 1, 2, 7, 8 (servers only), 10, 11 and mark the rest "not run" in the ACK.

## 8. Commit (local only)

```bash
git add mscc-ui/Avalonia-Migration/src mscc-ui/Avalonia-Migration/packaging/mscc-ui/DEBIAN/control mscc-ui/Release/avalonia/x86_64 installers/linux .mscc-coord/status/ubuntu-stew.md
git status        # staged: nothing under rpi/, linux/, MSCC.Wpf or MSCC.Core; no publish/ or obj/. tty0tty leftovers stay modified and UNSTAGED.
git commit -m "cmd-047a: Avalonia 0.6.64 (10m button width, stray Band label, FREQ CAL reconnect/VFO B/deferred-restore fixes, status history restored)"
```

- **Do not push. Do not pull.**
- ACK `done` (or `blocked` + reason) in `.mscc-coord/status/ubuntu-stew.md`: commit hash, client 0.6.64, deb name, smoke 1-11 results (**say "not run" for anything Stew didn't do; don't write "done" or "pass" without his answer**), and that the cmd-032 / cmd-043 history is back. No bare `|` in cells.

## Known risks

- `LeaveFreqCalTab(tabStillOpen: true)` on Disconnect plus the existing `ModeReported` re-entry: make sure it runs once (the `!_freqCalHoldingCw` check does that) and not on Close.
- VFO switch while the tab holds CW is still not handled specially (out of scope); just never write the other VFO's label.
- Deleting `_modeBeforeFreqCal` / `RestoreModeAfterFreqCalAsync`: grep first; nothing else should reference them.
- The grid fix changes the band bar width math; check nothing else in that grid uses a column number above 12.

## Out of scope

`rpi/`, `linux/` (and the tty0tty leftovers), WPF, Core, mscc-init, firmware, arm64 / Pi kit, remote testing, per-mode Hi memory.

Full orders: `.mscc-coord/COMMANDS.yaml` id `cmd-047a`.
