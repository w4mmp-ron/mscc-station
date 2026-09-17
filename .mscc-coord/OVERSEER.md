# Overseer intent

**Updated:** 2026-09-17  
**Overseer:** Build Commander

## Status

- Spectrum pumping (clients → Shack): cleared after Shack servers redeployed from current sources (old WFP/exe revert).
- **cmd-002–004:** done (Pi config; Avalonia 3-button audio; 0.6.54 arm64 on Pi).
- **Active — cmd-005 (ubuntu-stew):** Avalonia Remote Digital missing VirtualA/B on Pi because client loads Debian ALSA-only PortAudio instead of `/usr/local/lib` mscc-portaudio (Pulse). Fix PortAudioNative absolute prefer + `mscc-ui` launcher `LD_LIBRARY_PATH` (like `mscc.sh`). Bump to **0.6.55**, ship kits.

## Notes

Overseer writes instructions; Builds code/pack. One Build at a time. Push/pull around real kits/code.
