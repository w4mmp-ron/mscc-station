# cmd-046 - Windows notes (addendum to Ron's brief)

Ron's brief `.mscc-coord/briefs/cmd-046.md` is the source of truth. This file only pins the
Windows part for **windows-new-hp**. Stew decided to follow Ron's rule (2026-09-27).

## Scope on this host

- **SDRcore-trans (Windows)** `mscc-ui/windows-work-tree/SDRcore-trans/sources/driver.c` - brief section 1.
- **WPF** `MainViewModel.cs` + `MainWindow.xaml` - brief sections 2 and 3.
- **Not here:** `linux/SDRcore-trans-linux/sources/driver.c` rides in **cmd-043** (ubuntu-stew).
  `rpi/` is unchanged (Ron). Avalonia out of scope.

## Line pointers (checked at d49f939)

| Where | Lines | What |
|-------|-------|------|
| driver.c `Driver_Get_QRO_Power` | :99; `'L'` :109-113, `'U'` :114-118 | ternary -> LSB_POWER / USB_POWER |
| driver.c `Driver_Get_QRP_Power` | :137; `'L'` :147-151, `'U'` :152-158 | same; delete WSJT comment :153-154 |
| driver.c `case 'T'` | :104 / :142 | keep TUNE_POWER |
| MainViewModel.cs `ResolveOperatePowerBank` | :1800; delete :1807-1810 | keep TUN line :1803-1804 |
| MainWindow.xaml A8 tooltip | :2094 and :2096 | SSB wording (below) |

Anchor on the quoted text; lines shift as you edit.

## Also revert (cmd-045 section 7)

- **7.2 slider:** DIG-U (and USB/LSB with Digital or Remote Digital audio) goes back to the
  **SSB** bank. The extra `NotifyMainOperatePower()` calls added on audio-path changes are
  harmless; leave them or remove them, your choice.
- **7.1 / A8 tooltip:** the "DIG-U uses Tune power" wording goes back to SSB wording:
  `Drive power for what you're doing: TUN→Tune, CW→CW, AM→AM carrier, FM→FM, LSB/USB/DIG-U→SSB (any audio). Same values as the RX/TX tab.`

## Versions

- Client **9.26.6 -> 9.26.7** (auto-bump, `ClientVersion.txt`).
- SDRcore-trans **3.141 -> 3.142** (`sources/extern.h` `VERSION_MINOR`).

## Build / deploy

- SDRcore-trans Release; copy `Mscc-trans.exe` (`mscc-ui/Release/windows-wpf/`) to `C:\mscc-net9\`.
- `MSCC.Wpf` Release; `CopyToMsccNet9` copies to `C:\mscc-net9\`. Stop MSCC first.
- ACK in `.mscc-coord/status/windows-new-hp.md` (no bare pipe characters in table cells).
- Commit locally. **Do not push. Do not pull.**

## Smoke (plain, 3 steps)

1. Pick DIG-U. The main power slider says **SSB POWER**.
2. Change SSB power. The DIG-U output on the SA changes with it.
3. Pick TUN. It still uses **Tune power**, and Tune power did not change in steps 1-2.
