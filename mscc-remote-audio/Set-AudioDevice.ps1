<#
.SYNOPSIS
  Inject CMD_SET_AUDIO_DEVICE (0x9B) into a running ms-sdr session.

.DESCRIPTION
  Sends only the audio-device opcode to ms-sdr:8888 — no GUI handshake,
  so an active MSCC client session is left alone.

  Wire format: [opcode u8][int16 LE payload]  (ms-sdr uses the low byte)

  Values:
    0 = Digital (local digi)
    1 = Phones  (local operator mic)
    2 = Remote  (MSA1 UDP mic — Phones + REMOTE AUDIO)

  Does not modify ms-sdr. Optional -DirectTrans sends to sdrcore-trans:9200.

.EXAMPLE
  .\Set-AudioDevice.ps1 -HostName proficio -Mode remote
  .\Set-AudioDevice.ps1 -HostName 192.168.12.199 -Mode phones
  .\Set-AudioDevice.ps1 -Mode digital
  .\Set-AudioDevice.ps1 -Mode remote -DirectTrans
#>
[CmdletBinding()]
param(
    [string]$HostName = "proficio",

    [ValidateSet("digital", "phones", "remote", "0", "1", "2")]
    [string]$Mode = "remote",

    [int]$Port = 0,

    # Send straight to sdrcore-trans:9200 (bypass ms-sdr)
    [switch]$DirectTrans
)

$ErrorActionPreference = "Stop"

$CMD_SET_AUDIO_DEVICE = 0x9B

function Resolve-ModeValue([string]$m) {
    switch ($m.ToLowerInvariant()) {
        "digital" { return 0 }
        "phones"  { return 1 }
        "remote"  { return 2 }
        "0"       { return 0 }
        "1"       { return 1 }
        "2"       { return 2 }
        default   { throw "Unknown mode: $m" }
    }
}

function New-OpcodePacket([byte]$opcode, [int16]$value) {
    $bytes = New-Object byte[] 3
    $bytes[0] = $opcode
    $le = [BitConverter]::GetBytes([int16]$value)
    if (-not [BitConverter]::IsLittleEndian) { [Array]::Reverse($le) }
    $bytes[1] = $le[0]
    $bytes[2] = $le[1]
    return $bytes
}

function Mode-Name([int]$v) {
    switch ($v) {
        0 { "Digital (0)" }
        1 { "Phones (1)" }
        2 { "Remote (2)" }
        default { "Unknown ($v)" }
    }
}

$value = Resolve-ModeValue $Mode
$useMsSdr = -not $DirectTrans.IsPresent
if ($Port -le 0) {
    $Port = if ($useMsSdr) { 8888 } else { 9200 }
}

$targetName = if ($useMsSdr) { "ms-sdr (active session OK)" } else { "sdrcore-trans (direct)" }
Write-Host "Set-AudioDevice → ${HostName}:${Port} ($targetName)"
Write-Host "  CMD_SET_AUDIO_DEVICE 0x9B  data=$(Mode-Name $value)"
Write-Host "  (no GUI handshake — will not steal the client session)"

$udp = New-Object System.Net.Sockets.UdpClient
try {
    $udp.Connect($HostName, $Port)
    $pkt = New-OpcodePacket $CMD_SET_AUDIO_DEVICE ([int16]$value)
    [void]$udp.Send($pkt, $pkt.Length)
    Write-Host "  sent $($pkt[0].ToString('X2')) $($pkt[1].ToString('X2')) $($pkt[2].ToString('X2'))  OK"
    Write-Host ""
    Write-Host "Check Pi logs for CMD_SET_AUDIO_DEVICE: $value"
}
finally {
    $udp.Close()
}
