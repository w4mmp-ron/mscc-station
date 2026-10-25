# stm32-dfu-downloader — Proficio application DFU flasher (Windows, Black Pill)

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

1. Hold **KEY (PA0)** + tap **NRST**, release KEY → DFU (`0483:df11`)
2. **Browse** to `app-firmware-async.bin` (STM32 repo: `psoc-replacement-stm32\proficio-stm32f411-25MHz\release\`)
3. **Burn application**

```text
dfu-util -d 0483:df11 -a 0 -s 0x08008000:leave -D <app.bin>
```
