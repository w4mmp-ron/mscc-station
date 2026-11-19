# cmd-091: Avalonia (stew-HP amd64), WPF parity block: Connect, CAT mode, identity prompt, defaults

Host: stew-HP (Ubuntu amd64). Files: `Avalonia/src/MSCC.Avalonia/` only (the maintained client, package
mscc-avalonia). Do NOT edit `mscc-ui/Avalonia-Migration` (frozen), `rpi/`, `Solidus/`, `installers/rpi/`,
or the WPF tree (WPF is the reference; read it, do not change it).
"Finished" for this cmd = commit and push only. No installer upload and no GitHub release.
Build the amd64 deb locally for Stew to test; do not commit any `.deb`.
Pull with `git pull --rebase --autostash` first. Follow repo-root CLAUDE.md / AGENTS.md.
Version: mscc-avalonia 1.7.07 -> 1.7.08 (csproj, DEBIAN/control, `installers/windows/CHANGELOG.md` entry as Ron does).

Source: read-only parity review 2026-11-19 (WPF 11.19.2 vs Avalonia 1.7.06). Line numbers are from that review
and may have shifted; find the code by name.

## 1. Connect must not overwrite the server's settings (size M)
Avalonia calls `PushActiveVfoToRadioAsync(force: true)` and `PushStickyOperateToRadioAsync()` on connect
(MainViewModel ~1871/1873; ~36 `await _radio.` calls at ~8046). WPF sends only pan resolution, smoothing,
audio route and NB at connect (WPF MainViewModel ~6425-6452). See also
`.mscc-coord/NOTE-FOR-STEW-AVALONIA-CONNECT-OVERWRITES-SERVER-2026-11-17.md`.
Do this:
- Make the Avalonia connect sequence send what WPF sends, nothing more. The radio/server state wins; the
  client reads it back and shows it.
- Before removing a value from the push, check whether the server reports it back. For any value the server
  never reports (step, CW pitch, QSK and FM power are the likely ones), do exactly what WPF does for it.
- List in the commit message which values were dropped and which were kept, and why.

## 2. CAT mode-set from WSJT-X applies the mode (size S)
Avalonia `cat.SetModeDigit` sets only `ModeText` (~3042-3045), so the label changes and the radio does not.
WPF sets `ActiveMode`, which sends the mode (WPF ~971-974, setter ~1947-1967).
Do this: route CAT MD through the same path a mode button uses (including the DIG-U overlay), on the UI thread.
The CAT reply reports the mode the radio is now in.

## 3. Transverter report listener (size S)
WPF handles `TransverterReported` (WPF ~6896). Avalonia has no handler. Add it and store the state the same way
WPF does. No new UI (WPF has none).

## 4. NB limits match WPF (size S)
Avalonia settings load (~7758-7759) and connect push (~8113-8114) clamp NB pulse to 10-510 and threshold to 1-1009.
Use 50-510 and 1-1000 everywhere, as WPF (~6440-6441) and the Avalonia controls do.

## 5. FW/Core identity prompt after Connect (size S)
WPF waits 2.5 s after Connect; if firmware/core identity has not arrived it asks Yes/No and on Yes sends
0xB2/0xB3/0xFE (WPF ~1691, ~1788-1846). Port it with `MsccDialog` Yes/No calling `RequestIdentityAsync()`.
No answer or No = keep running (warning, not a lockout).

## 6. LAST_HF/LF default mode = USB (size S)
Stew, 2026-11-19: USB, same as WPF. `BandLastUsedStore.cs` (~182-183) returns "" for the default mode; return
"USB" for the 14.074 MHz / 474.2 kHz defaults (WPF `SpectrumWaterfallSettings.cs` ~206/209).

## 7. Cleanup (size S)
- Remove the never-called `SendAlcOnAsync` (~6717).
- cmd-090 passed on the SA (Stew, 2026-11-19, 1.7.07): remove the temporary TX guard peak-level lines in
  `LinuxMicSender.cs` (`TxGuardEdgeDiag` and its helpers). Keep the two "TX guard" on/off log lines.

## Test checklist (Stew, stew-HP to Pi)
- [ ] Change a setting on the server side (e.g. AGC, NB, filter), then connect Avalonia: the server value stays and shows in the client
- [ ] Connect still shows the right VFO, mode, band and meters
- [ ] WSJT-X with Mode set to change the radio mode (or flrig): the radio changes mode, not just the label
- [ ] Back to WSJT-X Mode None: MSCC mode buttons work as before
- [ ] NB pulse/threshold sliders hold 50-510 / 1-1000 across a restart
- [ ] Connect with identity missing: Yes/No prompt appears; No keeps working
- [ ] Fresh band memory: 20 m lands on 14.074 USB, 630 m on 474.2 USB
- [ ] Remote Digital TUNE/FT8 still clean (cmd-090 unchanged), log has no peak lines
