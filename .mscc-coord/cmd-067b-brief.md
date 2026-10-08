# cmd-067b - Audio panel help text (WPF only, one line)

Follow-up to cmd-067 (same uncommitted tree on NEW-HP). Do NOT commit or push. Do NOT touch rpi/, Solidus/, installers/rpi/, servers, or Linux.

## Change
File: `mscc-ui/windows-work-tree/mscc-mscc/mscc-new/src/MSCC.Wpf/Controls/AudioSettingsPanel.xaml`, line 11 (the top help TextBlock).

Replace the last sentence:
  `Operator Out + Mic are required before Start; digital is optional unless you use digi/VAC.`
with:
  `Only Operator Out (speaker) is required to Start. Mic and digital are optional; without them, voice or digital transmit stays off until you set them.`

Also change `Blank = not set` to `(none) = not set` in the same line so it matches the new dropdown choice. Leave the rest of the text as is.

## Version / build
- Index the WPF client version (ClientVersion.txt) per the standing rule so the exe name and title bar match, then build and copy to C:\mscc-net9 as usual.
- No server rebuild needed.

## Smoke
- Settings > Audio header shows the new sentence; title bar shows the new client version.
