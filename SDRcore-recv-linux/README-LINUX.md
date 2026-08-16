# sdrcore-recv on Linux / WSL

**This tree is Linux-only.** Windows MSVC build stays in  
`C:\Users\Ron\.grok\worktrees\SDRcore-recv\` (untouched).

Full **PortAudio** server: Multus/Proficio **I/Q capture** + operator **speaker**, UDP on **:9000**.  
Target runtime: **Raspberry Pi / native Linux** (ALSA + PortAudio).  
Multus is used as a **sound device**, not via USB control APIs.

## Build

```bash
sudo apt-get install -y build-essential g++ libportaudio2 portaudio19-dev
# Prefer MSCC PortAudio at /usr/local (Pulse+ALSA) — see mscc-portaudio package
cd /path/to/SDRcore-recv-linux
make clean && make
# Binary: $HOME/mscc/sdrcore-recv
ldd $HOME/mscc/sdrcore-recv | grep portaudio   # expect /usr/local/lib on Pi
```

### Operator sample rate (96k phones vs not)

- Proficio **I/Q is always 96 kHz** (DSP / SAMPLERATE).
- If the operator play device supports 96 kHz → full duplex (or dual ring) at 96 kHz, **no resampler**.
- If it does not (typical cheap USB phones at 48 kHz) → **dual stream**: I/Q @ 96k + play @ device rate, with freestanding **Oboe MultiChannelResampler** (`sources/resampler/`, Quality Medium) converting AF 96k → play rate.
- Digi (Pulse VirtualA) usually accepts 96 kHz → same as before (dual if mixed host API, ring bypass).

Log lines to confirm:

- `manage_stream. rate plan: iq=96000 play=96000 dual=0 resample=0` — 96k adapter path
- `... play=48000 dual=1 resample=1 ... (Oboe resample)` — non-96k phones path

- Log: **`~/sdrcore-recv.log`** (or path under `$HOME/.local/mscc` depending on packaging)
- I/Q in: device name containing Multus / Proficio / MSCC (stereo in)
- Speaker: match `~/.local/mscc/operator-speaker.ini` (substring), else default ALSA output
- Digital: digi speaker ini (VirtualA)
