# Note for Stew: Launch Servers with a remote Host (2026-11-25)

Stew, an old WPF problem Ron hit today, and a small matching change for Avalonia if you want it.

## WPF (fixed in WPF 11.25.1, R11-25-1)

**What was wrong:** with Launch Servers ticked and Host set to another computer (Ron's Pi,
`proficio.local`), Connect started the Windows servers on the PC anyway, then connected to the Pi.
The local servers could not find the radio and showed warning pop-ups. The start code checked only
the checkbox (`LAUNCH_SERVERS`), never the Host.

**What 11.25.1 does** (`MSCC.Wpf/ViewModels/MainViewModel.cs`, `MainWindow.xaml(.cs)`):
- New `IsThisPcHost(host)`: empty, `localhost`, loopback, this computer's name (or name.local) or
  one of its own addresses. Same rule as Avalonia's `IsLocalHost`, except empty counts as local.
- `StartRadioService`: starts servers only when Launch Servers is ticked and the Host is this PC;
  otherwise it logs "Launch Servers ignored (connect only)" and only connects.
- `EnforceLaunchServersForHost()`, called when Host changes and once at startup: a Host that is not
  this PC unticks Launch Servers (saved as `LAUNCH_SERVERS=0`) and logs it.
- The checkbox is greyed out while Host is not this PC (`LaunchServersOptionEnabled`), like
  Avalonia. Back to 127.0.0.1: clickable again, still unticked.

**Tested (Ron):** Host `proficio.local`: no servers started on the PC, Disconnect left the Pi
servers running; Host 127.0.0.1 then tick, then Host back to `proficio.local`: the box unticked
itself and greyed out.

## Avalonia (option, yours)

Avalonia already starts servers only for a local Host (`LaunchServers && IsLocalHost(Host)`) and
greys the box out (`LaunchOptionEnabled`). But it leaves the box **ticked** while greyed out, which
looks as if the servers will start. Option: in `OnHostChanged`, untick `LaunchServers` when
`!IsLocalHost(value)` (and once after the settings load), so the box shows what Connect will do.
