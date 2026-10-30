# For Stew: replace the two CHU entries in the GEN button

From Ron, 2026-10-30.

CHU (Canada) is off the air, so the two CHU entries in the GEN rotation no longer give a
carrier to calibrate on. WWV already covers the U.S. (5, 10, 15, 20 MHz). For Europe, please
replace the CHU entries with the other two RWM (Moscow) frequencies. RWM 9.996 MHz is
already in the list.

| Now | Replace with | Why |
|---|---|---|
| CHU1 3.330 MHz | RWM 4.996 MHz | Night-time in Europe |
| CHU2 7.850 MHz | RWM 14.996 MHz | Daytime in Europe |

RWM sends a plain carrier with a Morse ID, offset 4 kHz from the WWV / BPM frequencies, so
it is never covered by them. With 4.996, 9.996 and 14.996 Europe has a carrier day and night.

## Where

WPF client, `mscc-new/src/MSCC.Wpf/MainWindow.xaml.cs`, `_genOptionsProficio` (about
line 1504):

```
("CHU1", 3_330_000L),   ->   ("RWM1", 4_996_000L),
("CHU2", 7_850_000L),   ->   ("RWM3", 14_996_000L),
("RWM", 9_996_000L),    ->   ("RWM2", 9_996_000L),   (label only, if you want them numbered)
```

Also the "WWV/CHU/RWM/USER" text in the GEN button tooltip (`MainWindow.xaml` about line
648) and the status text in `MainWindow.xaml.cs` (about line 318). The Avalonia client has
the same list in `MainViewModel.cs`.

Not tested here: client changes are yours.
