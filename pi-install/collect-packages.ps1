# Refresh pi-install/packages/ from the latest known build outputs in this repo.
# Run from anywhere:
#   powershell -NoProfile -ExecutionPolicy Bypass -File .\pi-install\collect-packages.ps1

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
if (-not (Test-Path (Join-Path $root "mscc-deb"))) {
    $root = $PSScriptRoot
    if (-not (Test-Path (Join-Path $root "..\mscc-deb"))) {
        throw "Cannot find mscc-station root (expected mscc-deb next to pi-install)."
    }
    $root = Resolve-Path (Join-Path $PSScriptRoot "..")
}

$dest = Join-Path $PSScriptRoot "packages"
New-Item -ItemType Directory -Path $dest -Force | Out-Null

function Latest-Deb([string]$dir, [string]$pattern) {
    $files = Get-ChildItem -Path $dir -Filter $pattern -ErrorAction SilentlyContinue |
        Sort-Object LastWriteTime -Descending
    if (-not $files) { return $null }
    return $files[0]
}

$jobs = @(
    @{ Dir = "mscc-portaudio"; Pattern = "mscc-portaudio_*_arm64.deb" },
    @{ Dir = "mscc-deb"; Pattern = "mscc_*_arm64.deb" },
    @{ Dir = "mscc-init-gui"; Pattern = "mscc-init-gui_*_all.deb" },
    @{ Dir = "mscc-ui\Avalonia-Migration"; Pattern = "mscc-ui_*_arm64.deb" }
)

Write-Host "Collecting into $dest"
Write-Host "Repo root: $root"
Write-Host ""

foreach ($j in $jobs) {
    $dir = Join-Path $root $j.Dir
    $latest = Latest-Deb $dir $j.Pattern
    if ($null -eq $latest) {
        Write-Host "MISSING: $($j.Dir)\$($j.Pattern)"
        continue
    }
    $to = Join-Path $dest $latest.Name
    Copy-Item $latest.FullName $to -Force
    Write-Host ("OK  {0,-40} {1,12:N0} bytes  ({2})" -f $latest.Name, $latest.Length, $latest.LastWriteTime.ToString("yyyy-MM-dd"))
}

# Keep helper script next to INSTALL.md
$sh = Join-Path $root "mscc-deb\install-mscc.sh"
if (Test-Path $sh) {
    Copy-Item $sh (Join-Path $PSScriptRoot "install-mscc.sh") -Force
    Write-Host "OK  install-mscc.sh"
}

Write-Host ""
Write-Host "Done. Update INSTALL.md version table if filenames changed."
Write-Host "Then copy pi-install/packages/ to the Pi (USB or scp)."
