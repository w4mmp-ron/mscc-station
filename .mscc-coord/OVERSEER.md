# Overseer intent

**Updated:** 2026-09-17  
**Overseer:** Build Commander

## Standing goal

Coordinate Multus SDR **MSCC** client/server debugging across Windows, Ubuntu, and Raspberry Pi via this folder — not live chat between Grok Build instances.

## Current focus

**Pi radio as remote-audio host**, same Win11 WPF client (`windows-new-hp`).

Already proven:

- Win11 client ↔ Win11 host: Phones + Digital remote, clean RF, ~CAL power, ALC dB scale.
- Win11 client ↔ Ubuntu host: same, both modes look good.

`rpi/` trans + ms-sdr now have the Ubuntu recipe (line gain 2.5, USB=TUNE for digital, 48→96 lerp, ALC dB, remote MIC 100). Recv already had 9100/9101 HOST/CTRL. **Do not copy Ubuntu ELFs into `rpi/`.**

Next: Pi Build pulls, rebuilds `rpi/` servers, starts them. Then human/WPF on `windows-new-hp` tests Phones+Remote then Digital+Remote.

## Notes for Builds

- Read `handoff.md` before editing or building.
- Canonical repo: `https://github.com/w4mmp-ron/mscc-station`
- Trees: Ubuntu → `linux/`; Pi → `rpi/`; this WPF client → `mscc-ui/`. Do not mix arch.
- When blocked, set status `blocked` and a short reason.
