# Copy the radio settings that are safe to share from the Pi to the Windows servers
# (Ron, 2026-10-28). Calibration and radio settings only; host files (ports, sound
# devices, IPs, i2c, remote audio, user_controls) are left alone.
#
#   copy-pi-ini.vbs          double-click: no console window; shows the plan, asks
#                            Yes / No, then shows the result (runs this with -Gui)
#   copy-pi-ini              copy from V:\ini-files (the Pi ~/.local/mscc share)
#   copy-pi-ini -Source X    copy from another folder
#   copy-pi-ini -WhatIf      show what would be copied, change nothing
#
# Stop the Windows servers first. The current Windows files are backed up to
# %LOCALAPPDATA%\MSCC-NET9\backup-<date>-<time>\ before anything is copied.
#
# iq.ini: Windows trans 3.150 and later reads IQ_OFFSET and IQ_PHASE, as the Pi does.

param(
    [string]$Source = 'V:\ini-files',
    [string]$Dest = (Join-Path $env:LOCALAPPDATA 'MSCC-NET9'),
    [switch]$WhatIf,
    [switch]$Gui
)

$title = 'Copy Pi settings to Windows'
if ($Gui) { Add-Type -AssemblyName System.Windows.Forms }

function Show([string]$text, [string]$icon = 'Information', [string]$buttons = 'OK') {
    if ($Gui) {
        return [System.Windows.Forms.MessageBox]::Show($text, $title, $buttons, $icon)
    }
    Write-Host $text
}

function Fail([string]$text) {
    if ($Gui) { Show $text 'Error' | Out-Null } else { Write-Error $text }
    exit 1
}

$files = @(
    'recv-iq.ini',        # receive I/Q calibration
    'iq.ini',             # transmit I/Q calibration
    'power_cal.ini',      # power calibration per band
    'amplifier_cal.ini',  # amplifier calibration
    'amplifier.ini',      # power limit per band
    'power.ini',          # power per mode
    'cw.ini',             # keyer settings (CW_Keyer_Installed=1)
    'freq_cal.ini',       # frequency calibration (same today)
    'amp-cal-status.ini'  # amplifier calibration status (same today)
)

$servers = 'ms-sdr-MKII', 'mscc-recv', 'Mscc-trans'

if (-not (Test-Path $Source)) { Fail "Source not found: $Source (Pi share mapped as V:?)" }
if (-not (Test-Path $Dest))   { Fail "Windows settings folder not found: $Dest" }

$running = Get-Process -Name $servers -ErrorAction SilentlyContinue
if ($running -and -not $WhatIf) {
    Fail ("Stop the Windows servers first: " + (($running.Name | Sort-Object -Unique) -join ', '))
}

$missing = @($files | Where-Object { -not (Test-Path (Join-Path $Source $_)) })
$todo = @($files | Where-Object { $missing -notcontains $_ })

# Plan: which files differ from the Windows copy
$copy = @(); $same = @()
foreach ($f in $todo) {
    $s = Join-Path $Source $f
    $d = Join-Path $Dest $f
    if ((Test-Path $d) -and
        ((Get-Content $s -Raw) -replace "`r", '') -eq ((Get-Content $d -Raw) -replace "`r", '')) {
        $same += $f
    } else {
        $copy += $f
    }
}

$plan = "From: $Source`nTo:   $Dest`n`n"
if ($copy)    { $plan += "Copy:`n  " + ($copy -join "`n  ") + "`n" }
if ($same)    { $plan += "Already the same:`n  " + ($same -join "`n  ") + "`n" }
if ($missing) { $plan += "Not on the Pi, skipped:`n  " + ($missing -join "`n  ") + "`n" }

if (-not $copy) { Show ($plan + "`nNothing to copy.") | Out-Null; exit 0 }
if ($WhatIf)    { Show ($plan + "`n(-WhatIf: nothing changed)") | Out-Null; exit 0 }
if ($Gui) {
    $answer = Show ($plan + "`nThe Windows files are backed up first. Copy now?") 'Question' 'YesNo'
    if ($answer -ne 'Yes') { exit 0 }
}

$backup = Join-Path $Dest ('backup-' + (Get-Date -Format 'yyyy-MM-dd-HHmmss'))
New-Item -ItemType Directory -Path $backup | Out-Null
foreach ($f in $copy) {
    $d = Join-Path $Dest $f
    if (Test-Path $d) { Copy-Item $d $backup }
}
foreach ($f in $copy) {
    Copy-Item (Join-Path $Source $f) (Join-Path $Dest $f) -Force
}

Show ("Copied:`n  " + ($copy -join "`n  ") + "`n`nBackup: $backup`n`n" +
      "Start the servers; check power with TUN on one band.") | Out-Null
