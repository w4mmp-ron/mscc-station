# cmd-076: WPF RX IQ tab fixes (APPLY ALL label, START shows ACTIVE, APPLY cancel message)

Host: **windows-new-hp**. Repo: `C:\Users\n8vet\OneDrive\Documents\GitHub\mscc-station`. **WPF client only.**
Source: Ron's `.mscc-coord/NOTE-FOR-STEW-RX-IQ-TAB-2026-10-12.md`, points 1-3.

`git pull --ff-only` first; the tree must be clean apart from this brief.
**Do NOT edit `rpi/`, `Solidus/`, `installers/rpi/`, or any server.** Minimal changes.
**STOP BEFORE COMMIT.** Show `git status`, `git diff --stat` and the full diffs.
Line numbers are from `314b286`; check them before you edit.

**Not in this brief:** Ron's point 4 and Stew's JUMP TO IMAGE. Keeping the LO fixed and moving only the receive window needs a recv change (the NCO is fixed at +12 kHz, and RIT retunes the LO); see `NOTE-FOR-RON-IQ-PHASE-AUTOCAL-2026-10-13.md`. UP 24 kHz stays as it is.

## A. RESET ALL → APPLY ALL (RX IQ tab only)
- `MSCC.Wpf/MainWindow.xaml`, about line 1662: the RX IQ tab button bound to `ResetAllRxIqCommand` changes `Content="RESET ALL"` → `Content="APPLY ALL"`.
- New tooltip: `Copy the current I/Q value to all bands (saved).`
- Width 72 is enough; check the text isn't clipped.
- The TX IQ tab RESET ALL (about line 1859, a real factory reset) is **not** touched.
- In `MainViewModel.cs` `ResetAllRxIq`, change any status text that says "reset" to "apply all". Keep the command name; don't rename it.

## B. START shows ACTIVE
- `MainWindow.xaml`, about line 1631: the RX IQ START button has `Content="START"` set directly on the element, which overrides the style's DataTrigger `Content=ACTIVE` (about line 1644).
- Remove `Content="START"` from the `<Button …>` element.
- Add `<Setter Property="Content" Value="START"/>` inside its `<Style>`, next to the `Height` setter.
- No other change.

## C. APPLY confirmation: say when it was cancelled
- `MainViewModel.cs` `ApplyRxIq` (about line 4059): the dialog stays as it is, default **No** (a safe default for a save).
- In the cancelled branch, before `return;`, add:
  ```csharp
            RxIqStatus = "APPLY cancelled - value NOT saved. Press APPLY and choose Yes to save.";
  ```
- In `ResetAllRxIq`, give the cancelled branch the same kind of message: `"APPLY ALL cancelled - nothing changed."`

## D. Build, version, CHANGELOG
1. Stop MSCC. `dotnet build MSCC.sln -c Release` from `mscc-ui/windows-work-tree/mscc-mscc/mscc-new`. This auto-bumps the version by date (10.13.2 if built today) and copies to `C:\mscc-net9`. No server rebuild.
2. `installers/windows/CHANGELOG.md`: a new section above R10.13.1 with the real number:
   ```
   ## R10.13.2 - 2026-10-13  (`mscc-net9-R10-13-2-install.exe`, title bar 10.13.2)

   ### Changed
   - RX IQ tab: RESET ALL is now labelled APPLY ALL (it copies the current value to all bands). START now reads ACTIVE while a session runs. Cancelling APPLY now says the value was not saved.
   ```
3. `.mscc-coord/status/windows-new-hp.md`: a cmd-076 row (client version, WPF only, not committed).

## E. Smoke (Stew)
1. RX IQ tab: the button reads **APPLY ALL**, and the TX IQ tab still reads RESET ALL.
2. Press START: the button reads **ACTIVE** (with the highlight colour). Press it again: it reads **START** and the session ends.
3. APPLY → No (or Enter): the status line says "APPLY cancelled - value NOT saved…" and `recv-iq.ini` is unchanged. APPLY → Yes: saved as before ("APPLY succeeded").
4. APPLY ALL → No: the status says nothing changed. Yes: all bands get the value, as before.
5. UP 24 kHz, LO FINE, I/Q OFFSET and ZERO behave as before. The title shows the new version.

STOP. No commit, no push.
