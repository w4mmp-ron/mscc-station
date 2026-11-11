# Windows installer

Actual Installer (10.8) project for MSCC on Windows. Replaces building from `C:\mscc-net9`.

- `MSCC.aip`: the project. SourceDir = `payload\`, SetupFolder = `worktrees\installers\windows`.
  Copied from `worktrees\MSCC.aip` (R10-31-2); the paths are absolute, so fix them on another PC.
- `payload\`: exactly what gets installed to `<SystemDrive>\mscc-net9`.
- `stage.ps1`: copies the build outputs (servers, client, remote, deploy scripts, factory) into `payload\`.

## Release steps
1. Build the client (`dotnet build -c Release`) and the Windows servers.
2. `pwsh installer\windows\stage.ps1`
3. Open `MSCC.aip` in Actual Installer, set AppVersion / SetupFileName, build
   (or `"C:\Program Files (x86)\Actual Installer\actinst.exe" /S MSCC.aip`).
4. Publish (2026-11-10, Ron): installers are **not committed** (`.gitignore`). Put the new
   `.exe` and the current Pi `.deb` files on a GitHub Release, e.g.
   `gh release create R11-10-14 installers/windows/*.exe installers/rpi/*.deb --title "..." --notes-file notes.md`.
   Users download from <https://github.com/w4mmp-ron/mscc-station/releases>.

## payload sources
| Files | Source |
|---|---|
| ms-sdr-MKII, mscc-recv, Mscc-trans, MsccRemotePhones, portaudio_x86, pthreadVC2 | `Release\windows-wpf\` (stage.ps1) |
| MSCC.Wpf, MSCC.Core, CommunityToolkit, NAudio, System.IO.Ports, runtimes\ | client Release build (stage.ps1) |
| MSCC-Remote.* | MSCC.Remote Release build (stage.ps1) |
| Start-MsccServers.bat, runhidden.vbs | `mscc-new\deploy\` (stage.ps1) |
| factory\ | `worktrees\factory` (stage.ps1) |
| init-files\, Initialize.bat, Install-MsccServers-AtBoot.bat, mscc-init.exe, mscc-remote.ico | fixed, kept here (from C:\mscc-net9, 2026-11-10) |
| PSoC Firmware\ (one folder per radio) | newest `<Radio>-<major>.<minor>.hex/.cyacd` from each `radio-psoc-firmware\<tree>\Release\` (stage.ps1; old files in the folder are removed) |
| Black Pill Firmware\ (stm32-app-flash, dfu-util, ST-Link scripts + 25 MHz bins) | `firmware-programmers\windows\stm32-flash` + `psoc-replacement-stm32\proficio-stm32f411-25MHz\release` (stage.ps1) |

(25 MHz board only; the 8 MHz build is not shipped.)
| utilities\ (PSoC bootloader, vc_redist) | fixed, kept here |

Seeded from `C:\mscc-net9` on 2026-11-10. Old firmware folders left out:
`Geminus MKII`, `Proficio`, `Proficio-MKII`, `Profico MKII` (duplicates of the per-radio folders).