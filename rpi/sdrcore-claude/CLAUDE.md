# CLAUDE.md — sdrcore (Claude working copy)

Claude's working copy of the Pi RX/TX DSP servers `sdrcore-recv` and
`sdrcore-trans` (Proficio / MSCC station). Owner: **Ron (W4MMP)**.

## Working with Ron

- Short, plain-English answers. Say what is verified and what isn't.
- **"stop" means stop immediately.**
- Ask before large or risky changes. Commit / push **only when asked**,
  `git add` specific paths only (Grok and other tools work in this repo too).

## Folder rules

| Path | Rule |
|---|---|
| `rpi/sdrcore-claude/` | **The working tree. Make changes only here.** |
| `rpi/SDRcore-recv-linux/`, `rpi/SDRcore-trans-linux/`, `rpi/ms-sdr-linux/` | Originals (Pi source of truth). **Never modify.** |
| Rest of `C:\Users\Ron\.grok\worktrees` | Read-only reference. |

Copied 2026-09-25 from mscc-station repo commit `8dee14a` (originals were clean):
SDRcore-recv-linux, SDRcore-trans-linux, ms-sdr-linux (command hub, added later same day).
Git repo root: `C:\Users\Ron\.grok\worktrees`.

## Architecture (per Ron)

WPF client (Windows) <-> **ms-sdr** (controller, UDP :8888) <-> **sdrcore-recv** (:9000)
and **sdrcore-trans** (:9200). ms-sdr is the only interface between the client and the
two DSP cores. ms-sdr is also the **radio controller** (talks to the radio hardware
directly; CAT, USB, keyer code live there). Other ports in ms-sdr `port_defines.h`: 8889 MSCC, 9600 panadapter,
9700 spectrum, 9800 waterfall.

## Build (on the Pi, arm64)

```bash
cd rpi/sdrcore-claude/SDRcore-recv-linux  && make clean && make
cd ../SDRcore-trans-linux                 && make clean && make
cd ../ms-sdr-linux                        && make clean && make
```

See also `rpi/mscc-deb/BUILD-SERVERS-ON-PI.md`.

## Current status

- 2026-09-25: copy made. Goal not yet defined.
- sdrcore-trans reviewed. Ron confirmed (not issues): overdrive code is legacy,
  ignore it; CW carrier is generated continuously (keyed elsewhere); no thread
  locking is fine (works for years).
- 2026-09-25 fixed in `sdrcore-trans/sources/udp_thread.c` (compiled x86 in WSL
  Debian, no warnings; NOT yet built on the Pi or tested on the radio):
  1. `CMD_SET_IQ_BAND`: unknown band is logged and ignored (was `G_iq_stack[200]` write).
  2. `CMD_SET_BAND_POWER_POWER`: new `Get_Power_Mode_Index()` maps wire mode
     (0 AM,1 LSB,2 USB,3 CW,4 TUNE,5 FM) to table slot (USB,LSB,AM,CW,TUNE,FM);
     mode 6 'D' is logged and ignored.
- 2026-09-25 fix "low power in digital" (regression from Stew's remote-audio commit
  3ec263a, 2026-09-16; copied to rpi in c36fbf0). Compiled x86 in WSL, no warnings;
  NOT yet built on the Pi or tested:
  3. `driver.c` (VERIFIED on air 2026-09-25: full power, follows SSB slider; digital mic gain needs turning down): USB/LSB always use USB_POWER/LSB_POWER (digital had switched to TUNE_POWER).
  4. `dsputils.c` `framesToComplex`: local DIGITAL_AUDIO (0) back on the analog gain
     (6.324 stereo / 3.162 mono); REMOTE_AUDIO (2) and REMOTE_DIGITAL_AUDIO (3) stay 2.5.
- 2026-09-25 remote mic (Pi host FT8 "skirt bump", cmd-039 context). Compiled x86,
  10-min simulation at -250..+250 ppm drift: no skips/underruns, trim tracks drift.
  NOT yet built on the Pi or tested on air. Not committed.
  5. `remote_mic.c` fill: old 0.8 %/2.4 % rate steps (pitch shift, toggled ~10 ms near
     thresholds) replaced by smooth trim (max +/-300 ppm) toward 100 ms level.
     Start / underrun: hold and prime to 100 ms. >250 ms queued: skip oldest once.
  6. `remote_mic.c`: no file I/O in the audio callback; callback counts events, receiver
     thread logs them (100 ms recv timeout). EVENTs now: hold_last reprime, resync,
     primed, overflow, udp_gap (removed: adaptive step, low_occ).
  7. `main.c` `sdrIqPlayOnlyCallback`: remote ring drained during TUNE (split streams).
- Syntax check trick: WSL `Debian` has gcc. Use `-iquote sources -idirafter sources`
  (plain `-I sources` pulls in the Windows `pthread.h` and fails).
