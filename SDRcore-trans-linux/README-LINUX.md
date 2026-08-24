# sdrcore-trans on Linux / WSL

**This tree is Linux-only.** Windows MSVC build stays in  
`C:\Users\Ron\.grok\worktrees\SDRcore-trans\` (do not mix).

Full **PortAudio** server: Multus/Proficio **I/Q TX** + operator **mic**, UDP on **:9200**.  
Target runtime: **Raspberry Pi / native Linux** (ALSA + PortAudio).  
Multus is used as a **sound device**, not via USB control APIs.

## Build

```bash
sudo apt-get install -y build-essential libportaudio2 portaudio19-dev
cd /mnt/c/Users/Ron/.grok/worktrees/SDRcore-trans-linux
make clean && make
/home/ron/mscc/sdrcore-trans
```

- Log: **`~/sdrcore-trans.log`**
- I/Q out: Multus / Proficio / MSCC (stereo out to radio)
- Mic: match `~/.local/mscc/operator-microphone.ini` (PortAudio name substring)
- Digital mic: `~/.local/mscc/digital-microphone.ini` (same; if missing, D falls back to operator mic)
- Remote operator mic (Phones/P only): `~/.local/mscc/remote-mic.ini`  
  - `ENABLED=1` → MSA1 UDP listen (default **9101**) from Windows MsccRemotePhones  
  - `ENABLED=0` → local operator mic  
  - Digital (D) never uses this path
- ms-sdr: **127.0.0.1:8888**

## Known fix: TUNE + Audio D

TUNE must produce I/Q with Audio **P** or **D**. See **[FIX-TUNE-DIGITAL-AUDIO.md](FIX-TUNE-DIGITAL-AUDIO.md)** for the RPi5 bug (no power in D) and the applied fix.
