# cmd-065 — Ubuntu (stew-HP): MSCC UI dock icon fix + mscc-ui 0.6.72 + mscc 1.0.50 rebuild

Repo: /home/stew/Documents/GitHub/mscc-station (main == origin/main, at/after a4a9c49 / 1ce94eb).
Do NOT touch rpi/ or Solidus/. STOP BEFORE COMMIT — show `git status` + `git diff --stat` for review.

## Findings (read-only investigation, 2026-10-07)
- Live window (XWayland, GNOME Wayland session): `WM_CLASS = "MSCC.Avalonia", "MSCC.Avalonia"`.
  Avalonia 12.1.1 defaults WM_CLASS to the entry assembly name (MSCC.Avalonia); no X11PlatformOptions in Program.cs.
- Neither .desktop has StartupWMClass, and the desktop id is `mscc-ui`, so GNOME can't match the
  window to the pinned icon and shows a generic gear icon (MainWindow Icon is /Assets/avalonia-logo.ico).
- The pinned `mscc-ui.desktop` comes from ~/.local/share/applications/mscc-ui.desktop, which overrides the
  packaged /usr/share/applications/mscc-ui.desktop. It runs Exec=/home/stew/mscc-ui/mscc-ui (an old
  copy from a tarball, pid running /home/stew/mscc-ui/MSCC.Avalonia), NOT the .deb's /usr/bin/mscc-ui → /opt/mscc-ui.
- CW PHONES: commit b37e680 already took the PHONES checkbox and SetCwPhones send out of shared source
  (MainWindow.axaml/MainViewModel). Only the settings-store field CW_PHONES is left, and it's harmless. An amd64 rebuild drops it.
- mscc 1.0.50 needs: recv 3.143 (ef67816: DC blocker 0.98 + full-block FFT) and the ms-sdr 800-bin fix
  (9e2ea92, ms-sdr VERSION_MINOR 175). Version lives in linux/mscc-deb/packaging/DEBIAN/control (1.0.49).

## 1. Dock icon fix
a) mscc-ui/Avalonia-Migration/packaging/mscc-ui/usr/share/applications/mscc-ui.desktop — add:
```
StartupWMClass=mscc-ui
```
b) mscc-ui/Avalonia-Migration/src/MSCC.Avalonia/Program.cs — make WM_CLASS deterministic:
```diff
         => AppBuilder.Configure<App>()
             .UsePlatformDetect()
+            .With(new X11PlatformOptions { WmClass = "mscc-ui" })
```
(Check the property name in Avalonia 12.1.1 (`X11PlatformOptions.WmClass`). If it isn't there, keep the default
and use `StartupWMClass=MSCC.Avalonia` in the .desktop instead.)
c) Window icon: copy packaging/mscc-ui/icons/mscc-ui-256.png → src/MSCC.Avalonia/Assets/mscc-ui.png
(already covered by `<AvaloniaResource Include="Assets\**" />`). In Views/MainWindow.axaml, change
`Icon="/Assets/avalonia-logo.ico"` → `Icon="/Assets/mscc-ui.png"` (and the same in any other top-level Window that uses avalonia-logo.ico).
d) Local override on stew-HP (not a repo change; do it after installing the new deb): remove or fix
`~/.local/share/applications/mscc-ui.desktop` so the dock uses the packaged entry. Back it up first:
`mv ~/.local/share/applications/mscc-ui.desktop ~/mscc-ui.desktop.bak && update-desktop-database ~/.local/share/applications`.
The favorite id stays `mscc-ui.desktop`, so the pin survives.

## 2. mscc-ui 0.6.72 (amd64)
- Bump `<Version>` in src/MSCC.Avalonia/MSCC.Avalonia.csproj (0.6.71 → 0.6.72) and `Version:` in
  packaging/mscc-ui/DEBIAN/control (0.6.71 → 0.6.72). (The template says Architecture arm64; the amd64 script rewrites it. Leave it.)
- Build: `rm -rf mscc-ui/Avalonia-Migration/publish/linux-x64-sc && ./linux-build/mscc-ui-x64.sh && ./linux-build/build-mscc-ui-deb-amd64.sh`
  (the deb script only republishes if no publish exists, so clear the stale publish first).
- Check: `dpkg-deb -c ...mscc-ui_0.6.72_amd64.deb | grep desktop` and `dpkg-deb --fsys-tarfile ... | tar -xO ./usr/share/applications/mscc-ui.desktop | grep StartupWMClass`.

## 3. mscc 1.0.50 (amd64 servers)
- `linux/mscc-deb/packaging/DEBIAN/control`: Version 1.0.49 → 1.0.50 (one line in the Description is fine: recv DC blocker + 800-bin fix).
- Build: `./linux-build/build-mscc-deb-amd64.sh`. It rebuilds into $HOME/mscc via mscc-linux.sh with the mscc-portaudio rpath.
  Make sure the build makes recv 3.143 (extern.h) and ms-sdr 3.175+ (the Linux make auto-bumps VERSION_MINOR; record the final numbers).
  If the version.h/extern.h auto-bumps leave diffs, list them in the review.

## 4. installers/linux
- Run `./linux-build/drop-installers.sh linux` (the build scripts call it). Make sure installers/linux has
  mscc_1.0.50_amd64.deb and mscc-ui_0.6.72_amd64.deb, and that the 1.0.49/0.6.71 debs were replaced, not left alongside.
- installers/linux/CHANGELOG.md: shipped table → mscc 1.0.50, mscc-ui 0.6.72. New section
  `## mscc 1.0.50 / mscc-ui 0.6.72 — 2026-10-07` with:
  Fixed: receiver DC blocker (spur ~12 kHz below the VFO) and full-block spectrum FFT; the 800 spectrum resolution now reaches the receiver;
  the MSCC UI dock icon now shows as running on the pinned icon (no extra gear icon) and the window shows the MSCC icon.
  Removed: CW tab PHONES checkbox (amd64).
  Mark the 1.0.49/0.6.71 section "(superseded)". In "Not yet in an installer", keep only "Spectrum strongest bin per display point".

## 5. Smoke (stew-HP)
1. `sudo apt install ./installers/linux/mscc-portaudio_*_amd64.deb ./installers/linux/mscc_1.0.50_amd64.deb ./installers/linux/mscc-ui_0.6.72_amd64.deb`
2. Do step 1d. Log out/in if the dock doesn't refresh.
3. Launch MSCC UI from the pinned dock icon → there's one icon with a running dot and no gear icon. `xprop` on the window (click it) shows WM_CLASS "mscc-ui".
   `pgrep -af MSCC.Avalonia` shows /opt/mscc-ui/MSCC.Avalonia. The title shows MSCC 0.6.72.
4. The CW tab has no PHONES checkbox.
5. MSCC Start, then connect: Core/recv shows 3.143. There's no spur ~12 kHz below the VFO. Switching SPECTRUM RESOLUTION to 800 changes the spectrum.
## STOP — no commit/push. Report the diff and smoke results for review.
