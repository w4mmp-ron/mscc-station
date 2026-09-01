# Publish MSCC.Avalonia for Raspberry Pi OS 64-bit (linux-arm64).
# Run from "Avalonia Migration" or any directory (script locates itself).

$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $Root

Write-Host "Building framework-dependent linux-arm64 → publish\linux-arm64" -ForegroundColor Cyan
dotnet publish src\MSCC.Avalonia\MSCC.Avalonia.csproj `
  -c Release -r linux-arm64 --self-contained false `
  -o "$Root\publish\linux-arm64"

Write-Host "Building self-contained linux-arm64 → publish\linux-arm64-sc" -ForegroundColor Cyan
dotnet publish src\MSCC.Avalonia\MSCC.Avalonia.csproj `
  -c Release -r linux-arm64 --self-contained true `
  -o "$Root\publish\linux-arm64-sc"

Write-Host ""
Write-Host "Done." -ForegroundColor Green
Write-Host "  Framework-dependent: $Root\publish\linux-arm64   (needs .NET 9 on Pi)"
Write-Host "  Self-contained:      $Root\publish\linux-arm64-sc (larger; copy whole folder)"
Write-Host "See TESTING.md for Pi steps."
