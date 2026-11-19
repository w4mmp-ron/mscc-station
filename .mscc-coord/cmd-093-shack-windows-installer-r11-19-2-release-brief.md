# cmd-093: Shack, Windows installer R11-19-2 + docs + release notes (no publish)

**STOP notes (read first)**
- Work on **Shack only**. Do not touch `rpi/`, `Solidus/`, `installers/rpi/`, `mscc-ui/Avalonia-Migration`, or any source code. No client rebuild (a build bumps ClientVersion past 11.19.2).
- **Do NOT run `stage.ps1`.** On Shack it deletes the payload Black Pill `.bin` files and then fails (no `psoc-replacement-stm32\...\release` folder), and it would copy a stale `MSCC.Wpf.deps.json`.
- **Do NOT run `gh release create` or `gh release delete`.** Stop after the commit and hand Stew the notes file and the exact commands. Stew publishes.
- Do not push (Stew pushes). Do not commit any `.exe` or `.deb`.
- Pull first: `git pull --rebase --autostash`. If anything came in that touches `mscc-ui/installer/windows/payload` or the WPF client, stop and tell Stew.

## Inputs already on Shack
- WPF client 11.19.2 (`ClientVersion.txt`), Release build from `91476b2b` in `mscc-ui\windows-work-tree\mscc-mscc\mscc-new\src\MSCC.Wpf\bin\Release\net9.0-windows\`.
- `installers/linux/mscc_1.0.102_amd64.deb` (1,238,904 bytes) and `installers/linux/mscc-avalonia_1.7.08_amd64.deb` (44,770,360 bytes), copied from stew-HP (cmd-092, `d2ebe929`). Confirm sizes.
- Pi debs: on release R11-19-1 (`mscc_1.0.102_arm64.deb`, `mscc-avalonia_1.7.04_arm64.deb`), re-attached unchanged.

## Steps
1. **Payload:** copy only `MSCC.Wpf.exe`, `MSCC.Wpf.dll`, `MSCC.Core.dll` from the Release folder above into `mscc-ui/installer/windows/payload` (same place Ron updated in `fb60810e`). Then `git status mscc-ui/installer/windows/payload`: only those 3 files may change. Confirm the dll file version reads 11.19.2.
2. **MSCC.aip** (the live one is `mscc-ui/installer/windows/MSCC.aip`, not the repo-root or `mscc-ui/MSCC.aip` copies): `AppVersion=11.19.2`, `SetupFileName=mscc-net9-R11-19-2-install`. Keep a single UTF-8 BOM.
3. **Build the installer from PowerShell:** delete the stale `installers/windows/mscc-net9-R11-19-2-install.exe` (10:41, built during cmd-089, unverified) first, then run `& "$env:APPDATA\Actual Installer\actinst.exe" /S mscc-ui\installer\windows\MSCC.aip` (this is the Free edition path on Shack; CLAUDE.md lists Program Files). If it sits at 0 % CPU, answer No to the hidden newsletter box. Output: `installers/windows/mscc-net9-R11-19-2-install.exe`. Report its size and time.
4. **Docs:**
   - `docs/manuals/MSCC-Software-Versions-Addendum.md` + PDF (`python docs/manuals/make-addendum-pdf.py`, keep 2 pages): header R11-19-2; client 11.19.2; drop "No new installer yet"; add an Ubuntu row to the Installers table (`mscc` 1.0.102 amd64, `mscc-avalonia` 1.7.08 amd64) and an Ubuntu column to the programs table (ms-sdr 3.215, recv 3.157, trans 3.163). Windows servers stay as they are (still behind the Pi under the Windows hold).
   - `installers/windows/INSTALL.md`: R11-19-2, title bar 11.19.2.
   - `installers/windows/CHANGELOG.md`: add an "R11-19-2 installer" entry (client 11.19.2 one-cable Digital TX guard; Windows servers unchanged).
   - `mscc-ui/windows-work-tree/CLAUDE.md`: folder note for R11-19-2.
   - `installers/linux/INSTALL.md`: add a short "WSJT-X audio" part. Local (radio on this PC): Input `VirtualA.monitor`, Output `VirtualB`. Remote Digital (MSCC Avalonia to a Pi): Output to the device set as the Digital mic in Remote Audio (Stew used `VirtualB_TX`). Wrong output = radio keys with no signal.
   - Add the Phones tip to the Windows Remote guide source if it is in `docs/manuals` as markdown: on a laptop, the built-in mic hears the speakers; mute the speakers or use a headset. If the guide is PDF-only, list it as open instead.
5. **Release notes file** (not committed): `$env:TEMP\R11-19-2-notes.md`, layout per CLAUDE.md "Finished" step 5:
   - Version table: Windows installer R11-19-2 (client 11.19.2, changed; servers unchanged), Pi `mscc` 1.0.102 arm64 (unchanged), Pi `mscc-avalonia` 1.7.04 arm64 (unchanged), Ubuntu `mscc` 1.0.102 amd64 (new), Ubuntu `mscc-avalonia` 1.7.08 amd64 (new).
   - What changed: Windows client one-cable Digital TX guard; Ubuntu servers in step with the Pi (SWR-fault TX stop, CAT S-meter, QRO cal in the Black Pill, version tag, `mscc status` Versions section); Avalonia 1.7.07/1.7.08 (Digital TX guard, Remote Audio fits small screens, connect keeps the server's settings, CAT mode change applies, FW/Core prompt, USB defaults).
   - "Downloads: which file do I need?" one line per file.
   - Links: Windows install guide, Raspberry Pi install guide, Ubuntu install guide (https://github.com/w4mmp-ron/mscc-station/blob/main/installers/linux/INSTALL.md), Software versions PDF.
6. **Commit only:** `git pull --rebase --autostash`, then one commit with specific paths (the 3 payload files, the live `MSCC.aip`, the docs above). Message: `R11-19-2: Windows installer 11.19.2 + Ubuntu rows in Addendum and docs`, ending with a "With Ron" list. Do not push.
7. **Hand to Stew** (do not run): the notes file path, the five file paths with sizes, and these commands:
   - `gh release download R11-19-1 -R w4mmp-ron/mscc-station -p "*arm64.deb" -D $env:TEMP\r11191`
   - `gh release create R11-19-2 -R w4mmp-ron/mscc-station --title R11-19-2 --target main --notes-file $env:TEMP\R11-19-2-notes.md installers\windows\mscc-net9-R11-19-2-install.exe installers\linux\mscc_1.0.102_amd64.deb installers\linux\mscc-avalonia_1.7.08_amd64.deb $env:TEMP\r11191\mscc_1.0.102_arm64.deb $env:TEMP\r11191\mscc-avalonia_1.7.04_arm64.deb`
   - `gh release view R11-19-2 -R w4mmp-ron/mscc-station` (check the five sizes)
   - `gh release delete R11-19-0 -R w4mmp-ron/mscc-station --yes` (keeps the tag; leaves R11-19-2 + fallback R11-19-1)

## Test checklist (Stew)
- [ ] Install `mscc-net9-R11-19-2-install.exe` on NEW-HP: title bar 11.19.2, Remote Digital one cable clean
- [ ] Addendum PDF is 2 pages and shows the Ubuntu row/column
- [ ] Release page shows 5 files with the right sizes; only R11-19-2 and R11-19-1 remain
