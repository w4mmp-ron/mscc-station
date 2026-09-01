# MSCC client version: month.day.iteration (e.g. 7.16.0)
# Same calendar day → increment iteration; month or day change → reset to 0.
$ErrorActionPreference = 'Stop'
$path = Join-Path $PSScriptRoot 'ClientVersion.txt'
$now = Get-Date
$month = $now.Month
$day = $now.Day
$iteration = 0

if (Test-Path -LiteralPath $path) {
    $raw = (Get-Content -LiteralPath $path -Raw).Trim()
    $parts = $raw -split '\.'
    if ($parts.Length -ge 3) {
        $prevM = 0; $prevD = 0; $prevI = 0
        [void][int]::TryParse($parts[0], [ref]$prevM)
        [void][int]::TryParse($parts[1], [ref]$prevD)
        [void][int]::TryParse($parts[2], [ref]$prevI)
        if ($prevM -eq $month -and $prevD -eq $day) {
            $iteration = $prevI + 1
        }
        # else new day/month → iteration stays 0
    }
}

$ver = '{0}.{1}.{2}' -f $month, $day, $iteration
# Avoid BOM so MSBuild/readback stay clean (Windows PowerShell 5.1 compatible)
[System.IO.File]::WriteAllText($path, $ver)
# Single-line stdout for MSBuild to capture
Write-Output $ver
