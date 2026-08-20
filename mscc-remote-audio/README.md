# MSCC Remote Phones (standalone)

Standalone **Windows** app for **operator remote audio**:

- **RX:** play phones AF streamed from a Pi (or test sender) — UDP **9100**  
- **TX:** capture mic and send MSA1 UDP (**9101** default) — **Windows only for now** (sdrcore-trans ingest TBD)  

**Digital stays on the Pi** — this path is operator phones/mic only.

## Layout

| Project | Role |
|---------|------|
| **MsccRemotePhones** | WinForms player: UDP listen → jitter buffer → WASAPI |
| **MsccAudioTestSender** | Console: 440 Hz tone in **MSA1** packets (no Pi needed) |

Solution: `MsccRemoteAudio.sln`  
Path: `C:\Users\Ron\.grok\worktrees\mscc-remote-audio\`

## Protocol (v1) — UDP

Header **16 bytes**, little-endian:

| Offset | Type | Field |
|--------|------|--------|
| 0 | u32 | Magic `0x3141534D` (`MSA1`) |
| 4 | u16 | Sequence |
| 6 | u16 | Frame count (samples per channel) |
| 8 | u8 | Channels (1 or 2) |
| 9 | u8 | Format (0 = s16le) |
| 10 | u32 | Sample rate (e.g. 48000) |
| 14 | u16 | Reserved |
| 16… | s16le | Interleaved PCM |

Default RX listen port: **9100**  
Default TX send port: **9101** (mic → Pi when trans is ready)

## Build / run (Windows)

```powershell
cd C:\Users\Ron\.grok\worktrees\mscc-remote-audio
dotnet build MsccRemoteAudio.sln -c Release
dotnet run --project MsccRemotePhones -c Release
```

Test tone (second terminal):

```powershell
dotnet run --project MsccAudioTestSender -c Release -- 127.0.0.1 9100 20
```

You should hear a 440 Hz tone for ~20 seconds.

## Playback EQ (local)

3-band EQ on **phones RX only** (not mic TX, not Pi local speaker):

| Band | Default freq |
|------|----------------|
| Low shelf | ~120 Hz |
| Mid peak | ~2 kHz |
| High shelf | ~5 kHz |

Enable + sliders ±12 dB; **Reset EQ**.

## Client settings (INI)

Windows client settings (ports, devices, volume, mute, EQ, TX host):

`%LocalAppData%\MSCC-NET9\MsccRemotePhones.ini`

Example: `C:\Users\<you>\AppData\Local\MSCC-NET9\MsccRemotePhones.ini`

**Not** `remote-phones.ini` — that file is for **sdrcore-recv** on the Pi (`~/.local/mscc/remote-phones.ini`).

## User steps

### RX (phones)
1. Start **MSCC Remote Phones**  
2. Listen port **9100** (or match Pi)  
3. Play device + phones volume (+ optional EQ)  
4. **Start RX**  
5. Run test sender or Pi stream  

### TX (mic) — Windows side only
1. Set **TX host** (Pi IP, or `127.0.0.1` for loopback)  
2. TX port **9101**  
3. Choose microphone + mic volume  
4. **Start Mic TX**  

**Loopback test (no Pi):** Start RX with listen port **9101**, Mic TX to `127.0.0.1:9101` — speak and hear yourself.

## Pi sender (`sdrcore-recv`)

Config: `$HOME/.local/mscc/remote-phones.ini`

```ini
ENABLED=1
HOST=192.168.x.x
PORT=9100
```

`HOST` = Windows PC running **MsccRemotePhones**.  
Rebuild/install `sdrcore-recv` from `SDRcore-recv-linux` (includes `remote_phones.c`).

Log line when active:
```text
remote_phones: ENABLED → <host>:<port> (MSA1 48k mono)
```

AF is post-DSP (AGC/NR/AN), before local volume.

## Later migration

Same protocol can be embedded in the MSCC WPF client; this app stays a thin reference player.
