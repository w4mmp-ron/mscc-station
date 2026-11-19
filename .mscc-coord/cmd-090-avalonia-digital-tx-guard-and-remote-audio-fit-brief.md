# cmd-090 — Avalonia (stew-HP amd64): Digital TX guard (port of cmd-089) + Remote Audio window fits 1366x768

Host: stew-HP (Ubuntu amd64). Files: `mscc-ui/Avalonia-Migration/src/MSCC.Avalonia/` only.
Do NOT touch `rpi/`, `Solidus/`, `installers/rpi/`, or the WPF tree.
"Finished" for this cmd = commit and push only. No installer upload and no GitHub release.
Build the amd64 deb locally so Stew can install and test, but do not commit any `.deb`.
Pull with `git pull --rebase --autostash` first. Follow repo-root CLAUDE.md / AGENTS.md.
Version: mscc-ui 0.6.75 -> 0.6.76 (csproj, CHANGELOG, and the deb control/version file the repo uses).

## Problem 1 — Digital remote: noise bump at the start and end of every transmission

Stew, 2026-11-19, stew-HP Digital remote with WSJT-X to the Pi, TUNE and FT8: a wide noise bump at the
start and at the end of each transmission, clean signal in between. Radio keying is not the cause.

Cause: `RemoteAudio/LinuxMicSender.cs` sends mic packets all the time and has no idea when the radio
is keyed. The Pi's transmit buffer is full of pre-key audio at key-down, and the tone starts with a hard
step. Windows fixed this in cmd-089 / 089b / 089c (WPF 11.19.2, see
`.mscc-coord/cmd-089-one-cable-digital-tx-guard-brief.md` and the WPF `RemoteAudio/RemoteMicSender.cs`).
Port the same behavior. Digital path only; Phones stays exactly as it is.

Do this:
1. Add a keyed state to `LinuxMicSender` (thread-safe, volatile bool or Interlocked), exposed through
   `RemoteAfEngine` (e.g. `SetTxGuard(bool keyed)` plus a `DigitalPath` flag).
2. `MainViewModel`: when the Remote AF path is Digital, call it on every key change (PttOn and TuneMode,
   including CAT PTT from WSJT-X via `cat.SetPtt`). Key-down = PttOn || TuneMode true.
3. In `CaptureLoop`, Digital only:
   - Not keyed: send packets with all-zero samples (keep sending, normal sequence numbers). The Pi
     buffer then holds only silence at key-down.
   - Key-down: keep sending zeros for 400 ms (mic dump), then go live with a 5 ms linear fade-in
     (240 samples at 48 kHz).
   - Key-up: 5 ms linear fade-out to zero, then zeros.
   - Receive playback muted while keyed in Digital (as WPF 089), back on at key-up with no player restart.
4. Log lines (match WPF wording): at key-down `Remote AF: TX guard — RX playback muted, mic dump 400 ms`,
   at key-up `Remote AF: TX guard off`.
5. Diagnostic (temporary, gate under the existing EVENT/debug logging so it can be stripped in the
   cleanup pass): at key-down and key-up log the mic peak level (dBFS) of the 100 ms before and the
   100 ms after the edge, one line each. This tells us right away if the trailing bump is in our audio.

## Problem 2 — Remote Audio window: Mic vol hidden on stew-HP's 1366x768 screen

`Views/RemoteAfWindow.axaml` is 440x520 with fixed rows and the log box in the `*` row. On the laptop's
smaller screen the bottom of the window (Mic and Mic vol) is cut off or sits under the debug log.

Do this:
1. Put rows 0–2 (path radios, RX panel, EQ / MIC TX panel) inside a ScrollViewer (vertical auto) so
   every control can be reached at any window height.
2. Give the log box a fixed small height (about 80 px, MinHeight 60) below the panels, not the `*` row.
3. Cap the window to the screen: on open, if Height is more than the working area, set it to the
   working area height minus 40 and position it so the top edge is on screen.
4. Same cap for `Views/DebugLogWindow.axaml` (720x420): open it so it does not cover the Remote Audio
   window when both are open (place it to the side or below, inside the working area).

## Test checklist (Stew, stew-HP to Pi, SA)
- [ ] Digital, WSJT-X TUNE: no bump at start, no bump at end
- [ ] Digital, FT8 transmit: no bump at start, no bump at end
- [ ] WSJT-X receive decodes normally between transmissions
- [ ] Log shows TX guard lines and the peak-level lines at key-down and key-up
- [ ] Phones remote unchanged (PTT, mic, receive)
- [ ] Remote Audio window: Mic and Mic vol visible and adjustable on stew-HP
- [ ] Debug Log window does not cover the Remote Audio window
