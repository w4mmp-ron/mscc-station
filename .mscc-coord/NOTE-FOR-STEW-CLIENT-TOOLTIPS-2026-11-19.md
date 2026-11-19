# Note for Stew: client tooltips and the "VAC" wording, WPF and Avalonia (2026-11-19)

Ron would like you to take this one. The clients are yours; Ron does not change the Windows or
Ubuntu side.

## What is wrong

**Long tooltips show as one wide strip.** No tooltip in either client has a line break.
Counted 2026-11-19 (text tooltips only, main windows):

| Client | File | Tooltips | Over 70 chars | With a line break |
|---|---|---|---|---|
| WPF | `mscc-ui/windows-work-tree/mscc-mscc/mscc-new/src/MSCC.Wpf/MainWindow.xaml` | 140 | 38 | 0 |
| Avalonia | `Avalonia/src/MSCC.Avalonia/Views/MainWindow.axaml` | 137 | 28 | 0 |

Longest ones: two-tone test (264 chars), Farnsworth on the paddles (207), park the calibration
files / Save settings (193), "This PC is the operator" (184), Connect also starts the servers (167),
Farnsworth spacing (162). The other windows (Remote Audio, keyer memory, ...) are worth the same look.

**Wrong text:**
- WPF Reset configuration: "Radio calibration reloads from factory values for your radio at the next
  server start." Since 2026-11-10 a reset keeps all calibration (the radio and the cal files stay).
- WPF RX IQ APPLY ALL: the tooltip should say what happens with "PC files only" unchecked (Ron's
  earlier note; check the current wording).
- Avalonia: several texts differ from WPF, for example PADDLE still mentions 0x9D / cw.ini.

## Suggested fix

- Split long tooltips at sentence breaks, lines of about 50-70 characters (`&#x0a;` in WPF XAML;
  Avalonia: `&#x0a;` in `ToolTip.Tip` works too, or a `TextBlock` with `TextWrapping` and `MaxWidth`).
- Fix the wrong texts above.
- Make WPF and Avalonia say the same thing (WPF as the reference), as for any client change.

## Also: "VAC" in the Remote Audio window (WPF and Avalonia)

The Remote Digital labels say VAC, but the audio can be any pair of devices (Ron uses two audio
adapters wired back to back, not VAC). The text is hard-coded. Checked 2026-11-19:

| Now | Suggested | WPF `src/MSCC.Wpf/RemoteAfWindow.xaml.cs` | Avalonia `Views/RemoteAfWindow.axaml.cs` |
|---|---|---|---|
| DIGITAL RX (VAC) | DIGITAL RX | line 117 | line 73 |
| DIGITAL MIC TX (VAC) | DIGITAL MIC TX | line 118 | line 74 |
| Mute VAC play | Mute digital play | line 137 | line 93 |

Avalonia `Views/RemoteAfWindow.axaml` line 25, Digital tooltip: "VAC for WSJT on this PC. CAT arms in
the main client." -> "Audio devices for WSJT on this PC. CAT arms in the main client." (WPF: check
for the same tooltip.)

No protocol or behaviour change; text only.
