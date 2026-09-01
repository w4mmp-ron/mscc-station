# Build mscc-ui_*.deb for Raspberry Pi OS 64-bit (linux-arm64).
# Self-contained Avalonia client + desktop menu entry (X-MSCC, same menu as Start/Stop/Init).
# Requires: .NET 9 SDK, Windows tar (bsdtar). No WSL/dpkg-deb required.
#
# Usage (from "Avalonia Migration" or any cwd):
#   powershell -NoProfile -ExecutionPolicy Bypass -File build-mscc-ui-deb.ps1

$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $Root

$PkgTemplate = Join-Path $Root "packaging\mscc-ui"
$ControlFile = Join-Path $PkgTemplate "DEBIAN\control"
if (-not (Test-Path $ControlFile)) { throw "Missing $ControlFile" }

$Version = (Select-String -Path $ControlFile -Pattern '^Version:\s*(.+)$').Matches[0].Groups[1].Value.Trim()
if ([string]::IsNullOrWhiteSpace($Version)) { throw "No Version in control" }

$OutDeb = Join-Path $Root "mscc-ui_${Version}_arm64.deb"
$PublishDir = Join-Path $Root "publish\linux-arm64-sc"
$Stage = Join-Path $env:TEMP ("mscc-ui-deb-" + [Guid]::NewGuid().ToString("N"))
$Pkg = Join-Path $Stage "pkg"

Write-Host "=== mscc-ui deb builder ===" -ForegroundColor Cyan
Write-Host "  version: $Version"
Write-Host "  out:     $OutDeb"
Write-Host "  stage:   $Stage"

# 1) Self-contained publish (no .NET install on Pi)
Write-Host "Publishing self-contained linux-arm64..." -ForegroundColor Cyan
dotnet publish (Join-Path $Root "src\MSCC.Avalonia\MSCC.Avalonia.csproj") `
  -c Release -r linux-arm64 --self-contained true `
  -p:PublishSingleFile=false `
  -o $PublishDir
if ($LASTEXITCODE -ne 0) { throw "dotnet publish failed" }

$AppBin = Join-Path $PublishDir "MSCC.Avalonia"
if (-not (Test-Path $AppBin)) {
  # Windows publish may name without extension still as MSCC.Avalonia for linux
  $alt = Get-ChildItem $PublishDir -Filter "MSCC.Avalonia*" | Select-Object -First 1
  if ($null -eq $alt) { throw "Publish output missing MSCC.Avalonia in $PublishDir" }
}

# 2) Stage package tree
if (Test-Path $Stage) { Remove-Item -Recurse -Force $Stage }
New-Item -ItemType Directory -Force -Path $Pkg | Out-Null
Copy-Item -Recurse -Force $PkgTemplate\* $Pkg\

# App payload
$Opt = Join-Path $Pkg "opt\mscc-ui"
New-Item -ItemType Directory -Force -Path $Opt | Out-Null
Copy-Item -Recurse -Force (Join-Path $PublishDir "*") $Opt\

# Icons (hicolor + pixmaps fallback)
$IconSrc256 = Join-Path $PkgTemplate "icons\mscc-ui-256.png"
$IconSrc48 = Join-Path $PkgTemplate "icons\mscc-ui-48.png"
$Icon256Dir = Join-Path $Pkg "usr\share\icons\hicolor\256x256\apps"
$Icon48Dir = Join-Path $Pkg "usr\share\icons\hicolor\48x48\apps"
$Pixmaps = Join-Path $Pkg "usr\share\pixmaps"
New-Item -ItemType Directory -Force -Path $Icon256Dir, $Icon48Dir, $Pixmaps | Out-Null
if (Test-Path $IconSrc256) {
  Copy-Item $IconSrc256 (Join-Path $Icon256Dir "mscc-ui.png") -Force
  Copy-Item $IconSrc256 (Join-Path $Pixmaps "mscc-ui.png") -Force
}
if (Test-Path $IconSrc48) {
  Copy-Item $IconSrc48 (Join-Path $Icon48Dir "mscc-ui.png") -Force
}

# Docs
$Doc = Join-Path $Pkg "usr\share\doc\mscc-ui"
New-Item -ItemType Directory -Force -Path $Doc | Out-Null
@"
mscc-ui $Version
================
MSCC Avalonia connect-only GUI for Multus / Proficio on Raspberry Pi OS 64-bit.

Install:
  sudo apt install -y ./mscc-ui_${Version}_arm64.deb

Run:
  MSCC Start   (servers)
  MSCC UI      (this client)  — or: mscc-ui

Settings (sticky):
  ~/.config/MSCC/mscc-avalonia.ini
  ~/.config/MSCC/mscc-favorites.ini

Does not start or stop ms-sdr; use MSCC Start / MSCC Stop.
"@ | Set-Content -Encoding utf8 (Join-Path $Doc "README.txt")

# Permissions notes for deb: executables
# tar will store mode; we set Unix modes when creating tar entries via --mode if supported
# bsdtar on Windows may not set +x; use a postinst chmod (already there)

# 3) Installed-Size (kB)
$sizeKb = [int]((Get-ChildItem -Recurse $Pkg -File | Measure-Object -Property Length -Sum).Sum / 1KB)
$ctrl = Get-Content (Join-Path $Pkg "DEBIAN\control") -Raw
if ($ctrl -match 'Installed-Size:') {
  $ctrl = $ctrl -replace 'Installed-Size:\s*\d+', "Installed-Size: $sizeKb"
} else {
  $ctrl = $ctrl.TrimEnd() + "`nInstalled-Size: $sizeKb`n"
}
# Unix line endings for control
$ctrl = $ctrl -replace "`r`n", "`n"
if (-not $ctrl.EndsWith("`n")) { $ctrl += "`n" }
[System.IO.File]::WriteAllText((Join-Path $Pkg "DEBIAN\control"), $ctrl, [System.Text.UTF8Encoding]::new($false))

# Ensure scripts use LF and no BOM
foreach ($script in @("postinst", "postrm")) {
  $p = Join-Path $Pkg "DEBIAN\$script"
  if (Test-Path $p) {
    $t = [System.IO.File]::ReadAllText($p) -replace "`r`n", "`n" -replace "`r", "`n"
    [System.IO.File]::WriteAllText($p, $t, [System.Text.UTF8Encoding]::new($false))
  }
}
$launcher = Join-Path $Pkg "usr\bin\mscc-ui"
$lt = [System.IO.File]::ReadAllText($launcher) -replace "`r`n", "`n" -replace "`r", "`n"
[System.IO.File]::WriteAllText($launcher, $lt, [System.Text.UTF8Encoding]::new($false))

# 4) Build control.tar.gz and data.tar.gz
$ControlTar = Join-Path $Stage "control.tar.gz"
$DataTar = Join-Path $Stage "data.tar.gz"
$DebianBinary = Join-Path $Stage "debian-binary"
[System.IO.File]::WriteAllText($DebianBinary, "2.0`n", [System.Text.UTF8Encoding]::new($false))

# control + data tarballs with Unix modes (required so dpkg can run postinst)
$packPy = Join-Path $Stage "pack_tar.py"
@"
import tarfile, os, sys
pkg = sys.argv[1]
control_out = sys.argv[2]
data_out = sys.argv[3]
debian = os.path.join(pkg, 'DEBIAN')

with tarfile.open(control_out, 'w:gz', format=tarfile.GNU_FORMAT) as tar:
    for name, mode in [('control', 0o644), ('postinst', 0o755), ('postrm', 0o755)]:
        path = os.path.join(debian, name)
        info = tar.gettarinfo(path, arcname=name)
        info.uid = 0; info.gid = 0; info.uname = 'root'; info.gname = 'root'
        info.mode = mode
        with open(path, 'rb') as f:
            tar.addfile(info, f)

def add_tree(tar, root, rel=''):
    base = os.path.join(root, rel) if rel else root
    for entry in sorted(os.listdir(base)):
        full = os.path.join(base, entry)
        arc = os.path.join(rel, entry).replace('\\', '/') if rel else entry
        arc = arc.replace('\\', '/')
        if os.path.isdir(full):
            info = tar.gettarinfo(full, arcname=arc)
            info.uid = 0; info.gid = 0; info.uname = 'root'; info.gname = 'root'
            info.mode = 0o755
            tar.addfile(info)
            add_tree(tar, root, arc)
        else:
            info = tar.gettarinfo(full, arcname=arc)
            info.uid = 0; info.gid = 0; info.uname = 'root'; info.gname = 'root'
            if (arc.endswith('usr/bin/mscc-ui') or arc.endswith('opt/mscc-ui/MSCC.Avalonia')
                    or arc.endswith('.so') or os.path.basename(arc) == 'createdump'):
                info.mode = 0o755
            else:
                info.mode = 0o644
            with open(full, 'rb') as f:
                tar.addfile(info, f)

with tarfile.open(data_out, 'w:gz', format=tarfile.GNU_FORMAT) as tar:
    for top in ('usr', 'opt'):
        p = os.path.join(pkg, top)
        if os.path.isdir(p):
            add_tree(tar, pkg, top)
print('tarballs ok')
"@ | Set-Content -Encoding utf8 $packPy
python $packPy $Pkg $ControlTar $DataTar
if (-not (Test-Path $ControlTar)) { throw "Failed to create control.tar.gz" }
if (-not (Test-Path $DataTar)) { throw "Failed to create data.tar.gz" }

# 5) Assemble .deb (GNU ar format: debian-binary + control.tar.gz + data.tar.gz)
function New-DebFile {
  param([string]$DebPath, [string]$DebianBinaryPath, [string]$ControlTarPath, [string]$DataTarPath)
  $fs = [System.IO.File]::Create($DebPath)
  try {
    $bw = New-Object System.IO.BinaryWriter $fs
    # global magic: !&lt;arch&gt;\n
    $bw.Write([byte[]](0x21, 0x3C, 0x61, 0x72, 0x63, 0x68, 0x3E, 0x0A))

    function Emit-Member([string]$name, [byte[]]$bytes) {
      $nameField = $name.PadRight(16)
      if ($nameField.Length -gt 16) { $nameField = $nameField.Substring(0, 16) }
      $mtime = ([DateTimeOffset]::UtcNow.ToUnixTimeSeconds()).ToString().PadRight(12)
      if ($mtime.Length -gt 12) { $mtime = $mtime.Substring(0, 12) }
      $uid = "0".PadRight(6)
      $gid = "0".PadRight(6)
      $mode = "100644".PadRight(8)
      $size = $bytes.Length.ToString().PadRight(10)
      if ($size.Length -gt 10) { throw "member too large: $name" }
      $hdr = $nameField + $mtime + $uid + $gid + $mode + $size
      $bw.Write([System.Text.Encoding]::ASCII.GetBytes($hdr))
      $bw.Write([byte]0x60) # `
      $bw.Write([byte]0x0A) # \n
      $bw.Write($bytes)
      if (($bytes.Length % 2) -eq 1) { $bw.Write([byte]0x0A) }
    }

    Emit-Member "debian-binary" ([System.IO.File]::ReadAllBytes($DebianBinaryPath))
    Emit-Member "control.tar.gz" ([System.IO.File]::ReadAllBytes($ControlTarPath))
    Emit-Member "data.tar.gz" ([System.IO.File]::ReadAllBytes($DataTarPath))
    $bw.Flush()
  }
  finally {
    $fs.Dispose()
  }
}

if (Test-Path $OutDeb) { Remove-Item -Force $OutDeb }
New-DebFile -DebPath $OutDeb -DebianBinaryPath $DebianBinary -ControlTarPath $ControlTar -DataTarPath $DataTar

# Cleanup stage
Remove-Item -Recurse -Force $Stage -ErrorAction SilentlyContinue

$len = (Get-Item $OutDeb).Length
Write-Host ""
Write-Host "OK: $OutDeb" -ForegroundColor Green
Write-Host ("  size: {0:N1} MB" -f ($len / 1MB))
Write-Host ""
Write-Host "On the Pi (copy the .deb first):" -ForegroundColor Cyan
Write-Host "  sudo apt install -y ./mscc-ui_${Version}_arm64.deb"
Write-Host "  # Menu: MSCC → MSCC UI   or run:  mscc-ui"
Write-Host "  # Start servers first: MSCC Start"
Write-Host ""
Write-Host "Note: first install may need X11/OpenGL libs (Depends). If missing icons, log out/in once."
