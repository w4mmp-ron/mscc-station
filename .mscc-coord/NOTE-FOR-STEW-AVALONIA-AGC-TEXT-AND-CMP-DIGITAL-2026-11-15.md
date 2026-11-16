# From Ron: two small Avalonia bugs (AGC button text, CMP in Digital) (2026-11-15)

Found on proficio with Avalonia 1.7.04. Please fix them in the Avalonia client, since you're
working in it now. Line numbers are from `Avalonia/` at 525e586.

## 1. The AGC button shows SLO after every start

AGC itself is restored correctly; only the button text is wrong.

What the logs show after a client restart with AGC saved as FST:

- `~/.config/MSCC-Avalonia/mscc-avalonia.ini` has `AGC_LEVEL=2`.
- The client sends 0x87 = 2 on connect, and the server answers 0x87 = 2.
- The radio is in FST, but the button says SLO.

The cause is in `ViewModels/MainViewModel.cs`, in the constructor:

- Line 199: `LoadClientSettings()` sets `AgcLevel = 2`. `OnAgcLevelChanged` sets
  `AgcButtonText = "FST"`.
- Line 212: `AgcButtonText = "SLO";` sets the text back to SLO, unconditionally.
- When the server then reports 2, `AgcLevel` doesn't change, so `OnAgcLevelChanged` doesn't
  run and the text stays SLO.

So the first press goes from the real FST to SLO, and it takes three presses to get back to FST.

Fix: remove line 212, or set the text from `AgcLevel` there.

## 2. CMP stays clickable in Digital audio

- WPF (`MainWindow.xaml` around 2136): the CMP button has a DataTrigger on `IsDigitalAudio`
  that sets `IsEnabled=False`, so it is greyed out in Digital.
- Avalonia (`Views/MainWindow.axaml` 1603): the CMP button only uses
  `ToggleCompressionCommand` with `CanExecute = CanOperate`. `OnIsDigitalAudioChanged` turns
  compression off, but the button stays clickable, so CMP can be turned back on in Digital.

Fix: disable the button in Digital, the same as WPF. For example, make the command's
CanExecute `CanOperate() && !IsDigitalAudio` and call
`ToggleCompressionCommand.NotifyCanExecuteChanged()` in `OnIsDigitalAudioChanged`.
