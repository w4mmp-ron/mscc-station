# Burn Black Pill flash DFU bootloader via ST-Link (STM32CubeProgrammer CLI).
# Default: bootload-PA8.bin @ 0x08000000. -Image combined: boot-app-firmware-async.bin
# (bootloader + app in one image) @ 0x08000000.
#
# Usage:
#   .\flash-bootloader.ps1
#   .\flash-bootloader.ps1 -Image bootload-PA8
#   .\flash-bootloader.ps1 -Image combined
#   .\flash-bootloader.ps1 -BinPath .\bootload-PA8.bin
#
# Without -BinPath the image is looked for next to this script, then in the STM32 repo's
# release folder (..\..\..\psoc-replacement-stm32\proficio-stm32f411-25MHz\release).
#
# Requires: STM32CubeProgrammer + genuine ST-Link. See BURN-BOOTLOADER.md in
# psoc-replacement-stm32\proficio-stm32f411-25MHz\docs

[CmdletBinding()]
param(
    [ValidateSet('bootload-PA8', 'combined')]
    [string] $Image = 'bootload-PA8',

    [string] $BinPath = '',

    [string] $CubeCli = '',

    [int] $FreqKHz = 4000
)

$ErrorActionPreference = 'Stop'
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$BlAddr = '0x08000000'

function Find-CubeCli {
    param([string] $Override)
    if ($Override -and (Test-Path -LiteralPath $Override)) {
        return (Resolve-Path -LiteralPath $Override).Path
    }
    $candidates = @(
        "${env:ProgramFiles}\STMicroelectronics\STM32Cube\STM32CubeProgrammer\bin\STM32_Programmer_CLI.exe",
        "${env:ProgramFiles(x86)}\STMicroelectronics\STM32Cube\STM32CubeProgrammer\bin\STM32_Programmer_CLI.exe"
    )
    foreach ($c in $candidates) {
        if (Test-Path -LiteralPath $c) { return $c }
    }
    throw @"
STM32_Programmer_CLI.exe not found.
Install STM32CubeProgrammer from:
  https://www.st.com/en/development-tools/stm32cubeprog.html
Or pass -CubeCli path\to\STM32_Programmer_CLI.exe
"@
}

if (-not $BinPath) {
    $name = switch ($Image) {
        'bootload-PA8' { 'bootload-PA8.bin' }
        'combined'     { 'boot-app-firmware-async.bin' }
    }
    $ReleaseDir = Join-Path $ScriptDir '..\..\..\psoc-replacement-stm32\proficio-stm32f411-25MHz\release'
    $BinPath = Join-Path $ScriptDir $name
    if (-not (Test-Path -LiteralPath $BinPath)) { $BinPath = Join-Path $ReleaseDir $name }
}

if (-not (Test-Path -LiteralPath $BinPath)) {
    throw "Bootloader image not found: $BinPath"
}

$BinPath = (Resolve-Path -LiteralPath $BinPath).Path
$cli = Find-CubeCli -Override $CubeCli

Write-Host "Cube CLI : $cli"
Write-Host "Image    : $BinPath"
Write-Host "Address  : $BlAddr"
Write-Host "Size     : $((Get-Item -LiteralPath $BinPath).Length) bytes"
Write-Host ""

Write-Host "=== Probe ST-Link / STM32F411 ==="
& $cli -c port=SWD mode=UR freq=$FreqKHz
if ($LASTEXITCODE -ne 0) {
    throw "Probe failed (exit $LASTEXITCODE). Check ST-Link wiring and that a genuine probe is connected."
}

Write-Host ""
Write-Host "=== Flash bootloader ==="
& $cli -c port=SWD mode=UR freq=$FreqKHz -w $BinPath $BlAddr -v -rst
if ($LASTEXITCODE -ne 0) {
    throw "Flash failed (exit $LASTEXITCODE)."
}

Write-Host ""
Write-Host "OK: bootloader programmed at $BlAddr and verified."
Write-Host "DFU: KEY+NRST (and PA8->GND+NRST if this was bootload-PA8). Remove PA8 jumper to run the app."
