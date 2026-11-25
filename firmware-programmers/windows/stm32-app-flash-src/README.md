# stm32-app-flash-src — Proficio application DFU flasher (Windows, Black Pill)

WinForms UI that runs **`dfu-util.exe` from this app’s folder**.

**Application updates only** (`0x08008000`).  
**Does not flash the bootloader** — use ST-Link or Cube for that (`../stm32-flash/stm32-bootloader-flash.ps1`).

Moved here from `psoc-replacement-stm32/proficio-stm32f411-25MHz/Win-downloader/` (2026-10-25).

## Layout

| Path | Role |
|------|------|
| **`../stm32-flash/stm32-app-flash.exe`** | Ready-to-run flasher (+ `.dll`, `.deps.json`, `.runtimeconfig.json`) |
| `../stm32-flash/dfu-util.exe` | Bundled next to the exe |
| `../stm32-flash/libusb-1.0.dll` | Required beside dfu-util |
| `tools/` | Source copy of dfu-util |
| `ProficioDfuDownloader/` | WinForms source |

## Run

`..\stm32-flash\stm32-app-flash.exe`

Requires **.NET 9** desktop runtime.

## Rebuild

```powershell
dotnet publish .\ProficioDfuDownloader\ProficioDfuDownloader.csproj -c Release
```

Output goes to `..\stm32-flash\` (`PublishDir` in the `.csproj`) as `stm32-app-flash.exe`
(`AssemblyName`; renamed from `ProficioDfuDownloader.exe` 2026-10-25). The source project keeps
its old name `ProficioDfuDownloader`.

## Use

1. **Check Radio**: running version (radio `16C0:05DC`), flash mode (`0483:df11`) or not found.
2. **Browse** to `BlackPill-25MHz-<version>-app.bin` (STM32 repo: `psoc-replacement-stm32\proficio-stm32f411-25MHz\release\`)
3. **Burn application**. Same steps as the Pi `stm32-app-flash` script (2026-11-24):
   - Radio running: sends USB command `0xFE` (`RadioUsb.cs`, through the `libusb-1.0.dll` beside
     dfu-util; WinUSB binds by itself since pill 3.17), waits up to 10 s for `0483:df11`.
   - Already in flash mode (BOOT jumper on): flashes anyway, then says to remove the jumper.
   - Burns, judges success by "File downloaded successfully" (the get_status error after
     `:leave` is normal), waits up to 10 s for the radio and shows its version.

```text
dfu-util -d 0483:df11 -a 0 -s 0x08008000:leave -D <app.bin>
```

Fallback if the pill does not go to flash mode: BOOT jumper (PA8) on + NRST (LED B), Burn,
jumper off + NRST.

Note: Claude's PowerShell tool can't open USB devices (libusb ACCESS error); test from Bash /
a normal shell.
