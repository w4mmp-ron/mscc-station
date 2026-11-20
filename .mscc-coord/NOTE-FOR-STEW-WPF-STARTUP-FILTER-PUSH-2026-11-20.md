# For Stew: WPF filter push at startup (from Ron, 2026-11-20)

Ron asked whether the client or the server should be the master for the filter settings. Our view
is the server (ms-sdr), the same rule as the Avalonia 1.7.08 connect fix: the server's settings
stay, and the client reads them and shows them.

There is one place where WPF still works the other way. Once per session, after the start
frequency is known, it sends its own LO CUT and HI CUT to the server
(`PushCutIndexesToRadioAsync` in WPF `MainViewModel.cs`, about line 5570, `_cutsPushedThisSession`).
Avalonia doesn't do this.

Why it was added: before your LSB fix (10.31.1), the server could have saved LO CUT index 0
in `user_controls.ini`. The push replaced that saved value with the client's own setting.

What we found in the Pi servers:

- ms-sdr loads FILTER_LOW_INDEX and FILTER_HIGH_INDEX from `user_controls.ini` and sends them to
  recv at startup. It saves whatever a client sends (`user_controls.c`).
- Index 0 is a real setting (LO CUT 500 Hz), so a range check in the server could not catch the old
  value. An index out of range is ignored by recv, which keeps the filter it already has.
- Any filter button press or mode change sends the right index again and the server saves it.
  So an old saved value from before 10.31 has long since been replaced.

So the push isn't needed any more. Nothing changes on the servers.

Options (your choice, WPF is yours):

1. Remove the startup push, so WPF works like Avalonia and the server is the master.
2. Leave it in. It does no harm, but at connect the client's setting wins over the server's.

If you remove it, test this: set a filter, close the client, start it again. The same filter should
show, and the log should show no LO CUT / HI CUT sent by the client at startup.
