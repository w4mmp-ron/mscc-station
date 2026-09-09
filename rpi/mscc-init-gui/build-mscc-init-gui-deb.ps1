# Build mscc-init-gui_*_all.deb on Windows (no WSL/dpkg-deb required).
# Usage:
#   powershell -NoProfile -ExecutionPolicy Bypass -File build-mscc-init-gui-deb.ps1

$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $Root

$PkgTemplate = Join-Path $Root "packaging"
$ControlFile = Join-Path $PkgTemplate "DEBIAN\control"
if (-not (Test-Path $ControlFile)) { throw "Missing $ControlFile" }

$Version = (Select-String -Path $ControlFile -Pattern '^Version:\s*(.+)$').Matches[0].Groups[1].Value.Trim()
if ([string]::IsNullOrWhiteSpace($Version)) { throw "No Version in control" }

$OutDeb = Join-Path $Root "mscc-init-gui_${Version}_all.deb"
$Stage = Join-Path $env:TEMP ("mscc-init-gui-deb-" + [Guid]::NewGuid().ToString("N"))
$Pkg = Join-Path $Stage "pkg"

Write-Host "=== mscc-init-gui deb builder ===" -ForegroundColor Cyan
Write-Host "  version: $Version"
Write-Host "  out:     $OutDeb"
Write-Host "  stage:   $Stage"

if (Test-Path $Stage) { Remove-Item -Recurse -Force $Stage }
New-Item -ItemType Directory -Force -Path $Pkg | Out-Null
Copy-Item -Recurse -Force (Join-Path $PkgTemplate "*") $Pkg

# Python package
$Share = Join-Path $Pkg "usr\share\mscc-init-gui"
New-Item -ItemType Directory -Force -Path $Share | Out-Null
Copy-Item -Recurse -Force (Join-Path $Root "mscc_init_gui") $Share

# Launcher (LF, executable)
$Bin = Join-Path $Pkg "usr\bin"
New-Item -ItemType Directory -Force -Path $Bin | Out-Null
$launcherText = @"
#!/usr/bin/env python3
import sys
sys.path.insert(0, "/usr/share/mscc-init-gui")
from mscc_init_gui.app import main
if __name__ == "__main__":
    main()
"@
[System.IO.File]::WriteAllText(
    (Join-Path $Bin "mscc-init-gui"),
    ($launcherText -replace "`r`n", "`n"),
    [System.Text.UTF8Encoding]::new($false))

# Docs
$Doc = Join-Path $Pkg "usr\share\doc\mscc-init-gui"
New-Item -ItemType Directory -Force -Path $Doc | Out-Null
$readme = Join-Path $Root "README-MSCC-INIT-GUI.md"
if (Test-Path $readme) { Copy-Item $readme $Doc -Force }

# Installed-Size
$sizeKb = [int]((Get-ChildItem -Recurse $Pkg -File | Measure-Object -Property Length -Sum).Sum / 1KB)
$ctrl = Get-Content (Join-Path $Pkg "DEBIAN\control") -Raw
if ($ctrl -match 'Installed-Size:') {
    $ctrl = $ctrl -replace 'Installed-Size:\s*\d+', "Installed-Size: $sizeKb"
} else {
    $ctrl = $ctrl.TrimEnd() + "`nInstalled-Size: $sizeKb`n"
}
$ctrl = $ctrl -replace "`r`n", "`n"
if (-not $ctrl.EndsWith("`n")) { $ctrl += "`n" }
[System.IO.File]::WriteAllText(
    (Join-Path $Pkg "DEBIAN\control"), $ctrl, [System.Text.UTF8Encoding]::new($false))

foreach ($script in @("postinst", "postrm", "prerm", "preinst")) {
    $p = Join-Path $Pkg "DEBIAN\$script"
    if (Test-Path $p) {
        $t = [System.IO.File]::ReadAllText($p) -replace "`r`n", "`n" -replace "`r", "`n"
        [System.IO.File]::WriteAllText($p, $t, [System.Text.UTF8Encoding]::new($false))
    }
}

$ControlTar = Join-Path $Stage "control.tar.gz"
$DataTar = Join-Path $Stage "data.tar.gz"
$DebianBinary = Join-Path $Stage "debian-binary"
[System.IO.File]::WriteAllText($DebianBinary, "2.0`n", [System.Text.UTF8Encoding]::new($false))

$packPy = Join-Path $Stage "pack_tar.py"
@"
import tarfile, os, sys
pkg = sys.argv[1]
control_out = sys.argv[2]
data_out = sys.argv[3]
debian = os.path.join(pkg, 'DEBIAN')

with tarfile.open(control_out, 'w:gz', format=tarfile.GNU_FORMAT) as tar:
    for name in sorted(os.listdir(debian)):
        path = os.path.join(debian, name)
        if not os.path.isfile(path):
            continue
        info = tar.gettarinfo(path, arcname=name)
        info.uid = 0; info.gid = 0; info.uname = 'root'; info.gname = 'root'
        info.mode = 0o755 if name in ('postinst', 'postrm', 'preinst', 'prerm') else 0o644
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
            if arc.endswith('usr/bin/mscc-init-gui') or arc.endswith('.py'):
                # launcher must be executable; .py stay readable
                info.mode = 0o755 if arc.endswith('usr/bin/mscc-init-gui') else 0o644
            else:
                info.mode = 0o644
            with open(full, 'rb') as f:
                tar.addfile(info, f)

with tarfile.open(data_out, 'w:gz', format=tarfile.GNU_FORMAT) as tar:
    for top in ('usr',):
        p = os.path.join(pkg, top)
        if os.path.isdir(p):
            add_tree(tar, pkg, top)
print('tarballs ok')
"@ | Set-Content -Encoding utf8 $packPy
python $packPy $Pkg $ControlTar $DataTar
if (-not (Test-Path $ControlTar)) { throw "Failed to create control.tar.gz" }
if (-not (Test-Path $DataTar)) { throw "Failed to create data.tar.gz" }

function New-DebFile {
    param([string]$DebPath, [string]$DebianBinaryPath, [string]$ControlTarPath, [string]$DataTarPath)
    $fs = [System.IO.File]::Create($DebPath)
    try {
        $bw = New-Object System.IO.BinaryWriter $fs
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
            $hdr = $nameField + $mtime + $uid + $gid + $mode + $size
            $bw.Write([System.Text.Encoding]::ASCII.GetBytes($hdr))
            $bw.Write([byte]0x60)
            $bw.Write([byte]0x0A)
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
Remove-Item -Recurse -Force $Stage -ErrorAction SilentlyContinue

$len = (Get-Item $OutDeb).Length
Write-Host ""
Write-Host "OK: $OutDeb" -ForegroundColor Green
Write-Host ("  size: {0:N1} KB" -f ($len / 1KB))
Write-Host ""
Write-Host "On the Pi:" -ForegroundColor Cyan
Write-Host "  sudo apt install -y ./mscc-init-gui_${Version}_all.deb"
