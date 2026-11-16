# From Ron: Avalonia client busy while idle; fix = Avalonia 12.1.6 (2026-11-15)

Please make this change in the Avalonia client, since you're working in it now.

## The problem

On the Pi (proficio, Pi 5), MSCC Avalonia uses 50-60 % of one core all the time, even when it
is not connected to the servers. It has always been like that.

## The cause

It is a bug in the Avalonia library, not in our code. Avalonia 12.1.1 (what
`MSCC.Avalonia.csproj` uses) has a loop in its X11 event loop that can go round and round
without ever waiting: about 42,000 times a second it asks the X display "anything new?" and gets
"no".

How we know:

- Stack samples of the running client (`dotnet-stack`): the busy thread is always inside
  Avalonia's own `X11PlatformThreading.RunLoop` / `Dispatcher.OnOSTimer` / `UpdateOSTimer` /
  `Wakeup`. None of our code is on the stack.
- A heap dump shows only 5 running timers (2 meter needles at 30 ms, the clock and 2 Avalonia
  ones at 1 s), about 70 wakeups a second in total. Our timers are not the cause.
- Avalonia rewrote exactly this loop in PR #22453 ("[X11] Implement
  IDispatcherImplWithExplicitBackgroundProcessing and some restructure of
  X11PlatformThreading", 2026-10-19), first released in 12.1.5:
  <https://github.com/AvaloniaUI/Avalonia/pull/22453>

## The test

Same client (1.7.04 source), only the Avalonia version changed, built for linux-arm64 and run
on proficio:

| | Avalonia 12.1.1 | Avalonia 12.1.6 |
|---|---|---|
| CPU, not connected | 50-61 % | 0.2-0.3 % |
| X display checks per second | about 42,000 | about 550 |
| CPU, connected (spectrum + waterfall) | not measured | about 33 % (real drawing work) |

It started, connected and ran normally on 12.1.6.

## The change

In `Avalonia/src/MSCC.Avalonia/MSCC.Avalonia.csproj`, change the four Avalonia packages from
`12.1.1` to `12.1.6`:

    <PackageReference Include="Avalonia" Version="12.1.6" />
    <PackageReference Include="Avalonia.Desktop" Version="12.1.6" />
    <PackageReference Include="Avalonia.Themes.Fluent" Version="12.1.6" />
    <PackageReference Include="Avalonia.Fonts.Inter" Version="12.1.6" />

Nothing else needs to change. It helps the Ubuntu / Debian PC copy too (same X11 code). Please
check it on your machines, and bump the client version as usual.
