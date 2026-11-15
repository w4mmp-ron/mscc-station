# Note for Ron: AGC FAST RELEASE slider range in Avalonia (2026-11-15)

From Stew (written with Grok Bot). Question only; no client code changed. Stew thinks the WPF range is the correct one.

## What we see

WPF, right-hand operate panel, `MainWindow.xaml` 2197-2198:

    <Slider Width="108" Minimum="0" Maximum="2000" SmallChange="10"
            Value="{Binding AgcFastRelease, Mode=TwoWay}" PreviewMouseWheel="Slider_PreviewMouseWheel"

Avalonia, SETTINGS / operate row, `Views/MainWindow.axaml` 1613-1615:

    <Slider Grid.Column="1" Classes="mscc-slider" Minimum="0" Maximum="100" Value="{Binding AgcFastRelease}"
            IsEnabled="{Binding IsConnected}"
            ToolTip.Tip="AGC Fast Release (ms). Button above cycles SLO/MED/FST."/>

So WPF goes 0-2000 ms in steps of 10. Avalonia's slider only goes 0-100, and has no SmallChange.

Avalonia is also inconsistent with itself: the slider max is 100, but the view-model and the settings file clamp to 0-1000 (`MainViewModel.cs` 2664 / 7643 / 7999, `ClientSettingsStore.cs` 820). WPF sends the slider value with no extra clamp (`MainViewModel.cs` 5475).

## What the servers do

Opcode `CMD_SET_AGC_FAST_LEVEL` 0xCB. The value is milliseconds. ms-sdr just forwards it to recv (`user_controls.c`, `CMD_SET_AGC_FAST_LEVEL`).

recv (`udp_thread.c` 1003-1044, same in the Windows, Linux and Pi trees) treats AGC mode (`CMD_GET_SET_AGC` 0x87) as:

- 0 SLO: release = 2000 ms
- 1 MED: release = 1000 ms
- 2 FST: release = the 0xCB value (`AGC_Release_level`)

0xCB itself has no clamp: `AGC_Release_level = (float) s_opcode_data`. That value is applied only while FAST is selected. So the server will take 2000 ms (same as SLO) or anything in between. An Avalonia max of 100 ms would make FAST always much faster than MED (1000) or SLO (2000).

## Question

Please confirm the intended FAST range (Stew expects 0-2000, like WPF and like SLO) and fix the Avalonia slider, or tell us why it should stay at 100. The 1000 clamp in the Avalonia settings/view-model may need to move too.

## Also FYI (mouse wheel)

We are adding mouse-wheel handling on the Avalonia sliders to match WPF: one notch = SmallChange (default 1). We will set AGC SmallChange to 10 only after the range is settled, or we will leave the AGC slider out of that change until you answer.
