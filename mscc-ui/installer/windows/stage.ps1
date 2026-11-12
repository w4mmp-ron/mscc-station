# stage.ps1 - refresh installer\windows\payload from the build outputs.
# Run after building the client and the Windows servers, then build MSCC.aip
# in Actual Installer. Fixed files (init-files, factory, PSoC Firmware,
# utilities, .bat/.vbs) live in payload\ and are not touched here.

$ErrorActionPreference = 'Stop'
$here    = Split-Path -Parent $MyInvocation.MyCommand.Path
$payload = Join-Path $here 'payload'
$ui      = Resolve-Path (Join-Path $here '..\..')                  # mscc-ui
$wt      = Join-Path $ui 'windows-work-tree'
$rel     = Join-Path $ui 'Release\windows-wpf'
$client  = Join-Path $wt 'mscc-mscc\mscc-new\src\MSCC.Wpf\bin\Release\net9.0-windows'
$remote  = Join-Path $wt 'mscc-mscc\mscc-new\src\MSCC.Remote\bin\Release\net9.0-windows'
$deploy  = Join-Path $wt 'mscc-mscc\mscc-new\deploy'
$factory = Resolve-Path (Join-Path $ui '..\factory')

function Copy-Set($from, $names) {
    foreach ($n in $names) {
        $src = Join-Path $from $n
        if (-not (Test-Path $src)) { Write-Warning "missing: $src"; continue }
        Copy-Item $src $payload -Recurse -Force
        Write-Host "  $n"
    }
}

Write-Host "Servers ($rel)"
Copy-Set $rel @('ms-sdr-MKII.exe','mscc-recv.exe','Mscc-trans.exe',
                'MsccRemotePhones.exe','MsccRemotePhones.dll',
                'MsccRemotePhones.deps.json','MsccRemotePhones.runtimeconfig.json',
                'portaudio_x86.dll','pthreadVC2.dll')

Write-Host "Client ($client)"
if (Test-Path $client) {
    Get-ChildItem $client -Exclude *.pdb | ForEach-Object {
        Copy-Item $_.FullName $payload -Recurse -Force; Write-Host "  $($_.Name)" }
} else { Write-Warning "missing: $client (dotnet build -c Release first)" }

Write-Host "Remote ($remote)"
if (Test-Path $remote) {
    Copy-Set $remote @('MSCC-Remote.exe','MSCC-Remote.dll',
                       'MSCC-Remote.deps.json','MSCC-Remote.runtimeconfig.json')
} else { Write-Warning "missing: $remote (MSCC-Remote not refreshed)" }

Write-Host "Scripts ($deploy)"
Copy-Set $deploy @('Start-MsccServers.bat','runhidden.vbs')

Write-Host "Factory ($factory)"
Copy-Item (Join-Path $factory '*') (Join-Path $payload 'factory') -Recurse -Force

$wtRoot = Resolve-Path (Join-Path $ui '..')
$bp      = Join-Path $payload 'Black Pill Firmware'
New-Item -ItemType Directory -Force $bp | Out-Null
$flash   = Join-Path $wtRoot 'firmware-programmers\windows\stm32-flash'
$rel25   = Join-Path $wtRoot 'psoc-replacement-stm32\proficio-stm32f411-25MHz\release'
Write-Host "Black Pill tools ($flash)"
Copy-Item (Join-Path $flash '*') $bp -Force
Write-Host "Black Pill 25 MHz ($rel25)"
# Newest versioned images BlackPill-25MHz-<major>.<minor>-app.bin / -boot-app.bin (the -bN
# rebuilds are never shipped); older firmware .bin files in the payload folder are removed.
Get-ChildItem $bp -File -Filter '*.bin' | Remove-Item -Force
$bpRx = '^BlackPill-25MHz-(\d+)\.(\d+)-app\.bin$'
$bpApp = Get-ChildItem $rel25 -File -Filter 'BlackPill-25MHz-*-app.bin' |
         Where-Object { $_.Name -match $bpRx } |
         Sort-Object { [int]($_.Name -replace $bpRx,'$1') }, { [int]($_.Name -replace $bpRx,'$2') } |
         Select-Object -Last 1
if (-not $bpApp) { throw "no versioned Black Pill firmware (BlackPill-25MHz-<ver>-app.bin) in $rel25" }
$bpBoot = $bpApp.Name -replace '-app\.bin$','-boot-app.bin'
foreach ($n in $bpApp.Name, $bpBoot, 'bootload-PA8.bin') {
    Copy-Item (Join-Path $rel25 $n) $bp -Force; Write-Host "  $n" }

# PSoC firmware: newest <Radio>-<major>.<minor>.hex/.cyacd from each tree's Release\
$psocSrc = Join-Path $wtRoot 'radio-psoc-firmware'
$psocDst = Join-Path $payload 'PSoC Firmware'
$psoc = [ordered]@{   # payload folder = tree, file prefix
    'Proficio-legacy'   = 'Proficio-Ultimus-Legacy',   'Proficio-Legacy'
    'Geminus-MKII'      = 'Geminus-MKII',              'Geminus-MKII'
    'Proficio-MKII-PTT' = 'Proficio-Ultimus-MKII-PTT', 'Proficio-MKII-PTT'
    'Proficio-MKII-ATU' = 'Proficio-Ultimus-MKII-ATU', 'Proficio-MKII-ATU'
    'Geminus-legacy'    = 'Geminus-Legacy',            'Geminus-Legacy'
    'Ultimus-Legacy'    = 'Proficio-Ultimus-Legacy',   'Ultimus-Legacy'
    'Ultimus-MKII-ATU'  = 'Proficio-Ultimus-MKII-ATU', 'Ultimus-MKII-ATU'
    'Ultimus-MKII-PTT'  = 'Proficio-Ultimus-MKII-PTT', 'Ultimus-MKII-PTT'
}
Write-Host "PSoC firmware ($psocSrc)"
foreach ($dst in $psoc.Keys) {
    $tree, $prefix = $psoc[$dst]
    $rx = '^' + [regex]::Escape($prefix) + '-(\d+)\.(\d+)\.hex$'
    $hex = Get-ChildItem (Join-Path $psocSrc "$tree\Release") -Filter '*.hex' |
           Where-Object { $_.Name -match $rx } |
           Sort-Object { [int]($_.Name -replace $rx,'$1') }, { [int]($_.Name -replace $rx,'$2') } |
           Select-Object -Last 1
    if (-not $hex) { Write-Warning "no versioned firmware for $dst in $tree\Release"; continue }
    $cyacd = Join-Path $hex.DirectoryName ($hex.BaseName + '.cyacd')
    $d = Join-Path $psocDst $dst
    New-Item -ItemType Directory -Force $d | Out-Null
    Get-ChildItem $d -File -Force | Remove-Item -Force
    Copy-Item $hex.FullName, $cyacd $d -Force
    Write-Host "  $dst\$($hex.BaseName)"
}

$files = Get-ChildItem $payload -Recurse -File
Write-Host ("payload: {0} files, {1:N1} MB" -f $files.Count, (($files | Measure-Object Length -Sum).Sum / 1MB))
