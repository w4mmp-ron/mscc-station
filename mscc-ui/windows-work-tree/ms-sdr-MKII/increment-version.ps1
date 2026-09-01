# Increment VERSION_MINOR in source\version.h before each build.
$ErrorActionPreference = "Stop"
$versionFile = Join-Path $PSScriptRoot "source\version.h"
if (-not (Test-Path -LiteralPath $versionFile)) {
    Write-Error "version.h not found: $versionFile"
    exit 1
}
$content = Get-Content -LiteralPath $versionFile -Raw
if ($content -notmatch '#define\s+VERSION_MINOR\s+(\d+)') {
    Write-Error "VERSION_MINOR not found in $versionFile"
    exit 1
}
$old = [int]$Matches[1]
$new = $old + 1
if ($new -gt 255) {
    # Wire protocol packs minor into one byte
    $new = 0
}
$updated = $content -replace '#define\s+VERSION_MINOR\s+\d+', "#define VERSION_MINOR $new"
# Preserve final newline style
[System.IO.File]::WriteAllText($versionFile, $updated)
Write-Host "VERSION_MINOR $old -> $new"
