# CLAUDE.md — sdrcore (Pi servers)

Claude's notes for the Pi servers `sdrcore-recv`, `sdrcore-trans` and `ms-sdr`
(Proficio / MSCC station). Owner: **Ron (W4MMP)**. Grok follows `AGENTS.md`; this
file is for Claude only.

## Working with Ron

- Short, plain-English answers. Say what is verified and what isn't.
- **"stop" means stop immediately.**
- Ask before large or risky changes. Commit / push **only when asked**,
  `git add` specific paths only (Grok and other tools work in this repo too).
- Ron does **not** `git pull` on the Pi. He copies the changed source files from
  Windows to the Pi and builds there. Give him the file list, not git steps.
- Ubuntu (`linux/`) work is Stew's; don't track or report it. After a pull, report
  only Stew's **Windows** changes and whether the Pi needs a matching port.

## Folder rules

| Path | Rule |
|---|---|
| `rpi/SDRcore-recv-linux/`, `rpi/SDRcore-trans-linux/`, `rpi/ms-sdr-linux/` | **Working trees** (Pi source of truth). Claude edits here. |
| Rest of `C:\Users\Ron\.grok\worktrees` | Read-only reference unless Ron says otherwise. |

History: Claude worked in a copy `rpi/sdrcore-claude/` (commits fe0c9a1, 0443c21,
9bb4be2, 4cb8b74). On 2026-09-25 Ron had the changes moved back into the originals
(originals unchanged since 8dee14a) and the copy deleted.
Git repo root: `C:\Users\Ron\.grok\worktrees`.

## Open list (Ron, updated 2026-09-29) - read first on resume

- **Commits (Ron, 2026-10-05): too many commits and pushes.** Finish everything that belongs
  to a piece of work first (code, notes, note for Stew), then ONE commit and one push. No
  follow-up "notes: committed xyz" commits.

Old history (resume points before 2026-10-31 afternoon, dated change logs, closed items) is
in `rpi/CLAUDE-history.md`. Grep it for earlier work on a topic; don't read it whole.

**2026-10-31 (16:39-16:43 local): LSB filter fix CONFIRMED on Ron's rig (client 10.31.1, Pi recv 3.150).** recv log: in LSB, USB and AM every LO CUT press arrived (75 / 100 / 200 / 300 / 500) and every HI CUT press (5500 / 2400 / 2700 / 3000 / 4000), each cut moving alone. Ron: LSB and USB on an AM station now sound the same. LSB / USB recheck CLOSED. Open: Ron's I/Q test (TX I/Q from CW, PHASE slider, APPLY; server code ms-sdr 3.185 / trans 3.147 / recv 3.150 not committed until then); SSB voice / ALC meter test.
**2026-10-31:** Ron will do the TX I/Q test later (needs setting up). Test: (1) CW mode, start TX I/Q calibration on a band: it must transmit; (2) move PHASE: phase changes; (3) APPLY; (4) Claude checks ms-sdr.log for `Radio_cal_store_iq ... queued` then `Radio_cal_send_iq ... sent`. Then commit ms-sdr 3.185 / trans 3.147 / recv 3.150.
**2026-10-31 (12:56 local): Pi ms-sdr 3.186 = power start order as Windows 3.185 (built, running, NOT committed, goes with the I/Q-in-radio port after Ron's TX I/Q test).** `Radio_cal_load_power`: bands the radio does not hold now KEEP the power_cal.ini value (before: factory value, which overwrote a calibrated file on the first start with a radio holding nothing). Radio values still win where held. `Factory_power_cal_values()` no longer called (left in factory_seed.c, as Stew did). Log on Ron's rig: 160-10 m `radio N, file N, same`, 630 / 2200 m `not in radio, file 0 kept`, 0 changed; I/Q 22 bands from radio; no errors. Stew's open item #3 (power start order) DONE on the Pi. Pi now: recv 3.150, trans 3.147, ms-sdr 3.186, pill 3.12.
**2026-10-31 (13:00-13:10 local): recv NR (noise reduction) found defective; Ron: replace it with a spectral-subtraction NR (option B), PARKED for now.** Ron: "the audio becomes really distorted" with NR on. Code: `denoise()` in `SDRcore-recv-linux/sources/dsputils.c` = LMS adaptive line enhancer at 96 kHz after the AGC (48 taps, 32-sample delay, mu 0.002, wet 0.35-1.0 by level, step normalised by ONE sample's power averaged over ~12 samples, no leakage, no output limit). Measured: Ron listens on Remote Phones (UDP to the PC, port 9100, 48 kHz mono s16 after a 16-byte "MSA1" header; local C-Media output is muted then, so recording its monitor gives silence). Captured with `sudo tcpdump -i any -w X.pcap "udp dst port 9100"` and decoded (scratchpad pcap2wav.py). Fair A/B on one rag chew, NR toggled during a 21 s capture (recv log times are UTC): with NR ON (level 50) a ~3.8 kHz whistle at the top of the 4 kHz filter (strongest component every second), >3 kHz energy from about -15 dB to -4 dB of total, peaks 0.33-0.37 vs 0.20 off (0.366 repeated = clipping), speech band 300-1000 Hz down up to 10 dB. Cause: the ALE boosts anything correlated over the delay; noise through recv's sharp audio filter is strongly correlated at the filter edge -> whistle; per-sample normalisation over-corrects at quiet moments -> overshoot / spikes. A Python copy of the algorithm on an upsampled NR-off capture did NOT reproduce it (the earlier NR-on / NR-off captures were at different moments; only the same-moment A/B shows it). Options: A = repair (NLMS over all taps, leakage, output never above input), B = replace with frequency-domain spectral subtraction (as most SDRs). **Ron chose B, parked.** When done: same change in the local copy (sync rule), Stew's Windows recv likely has the same NR, retest with the same A/B capture.
**2026-10-31 (13:15 local): recv NB (noise blanker) checked: WORKS, makes no ticks of its own.** Same Remote Phones A/B capture (21 s, NB toggled; switch times from recv log `CMD_GET_SET_NB_ENABLE`), ticks = samples far outside their 10 ms neighbourhood after a first difference (scratchpad ticks.py): NB off 3.8 ticks/s (biggest 18-415x local, usually 30-50x), NB on 1.1/s (12-22x), two seconds with none; ticks back at once each time NB went off (real band impulse noise, irregular spacing 14 ms to hundreds of ms); no clicks at the switch moments; threshold 0.00999 throughout (3.147 thread fix holds). Ron finds NB hard to adjust. Why (blanker.c): threshold is an ABSOLUTE amount above the running envelope (envelope + threshold), so the same setting means different things on quiet / noisy bands; the envelope attack / release is 0.05 ms, so it nearly follows the pulse itself; no feedback on what it does; width is in raw 96 kHz samples (30 = 0.3 ms). **PARKED (Ron): automatic NB.** recv only: running level + spread over ~50-100 ms, blank only short samples far outside the spread (~5-6x), self-protection (less sensitive if blanking more than ~100/s); NB = on/off for the user (threshold knob could become Low / Medium / High: Stew). Includes "relative threshold" and "slower envelope"; pulse blanking (gain ramp) unchanged. Optional "blanking meter" (recv sends blanks per second, client shows it) needs Stew. When done: same change in the other recv copy, retest with the tick capture.
**2026-10-31 (evening): SSB / WSJT output power low (about 40 W of 100), fixed in trans 3.149 (BUILT on the Pi over ssh, NOT committed).** Cause: the 3.146 ALC ceiling 0.45 x drive (from Stew's Windows 3.149) held peaks to 0.45 / 0.8 (TUNE level) = about 32% of TUNE power. `ALC_CEILING` moved to `extern.h` (udp_thread.c CMD_SET_ALC_MULTIPLIER uses it, was a hard 0.45). 3.148 = 0.80: WSJT tone 89 W vs TUNE 109 W (ATU-100); 3.149 = 0.89: 95 W (PA near compression). Ron then recalibrated power (QRP and QRO): output power now right. Open (Stew, Ron 2026-10-31: no SSB voice test by Ron): SSB voice + splatter check on his analyzer with the 0.89 ceiling (ask, worded as a question).
**2026-10-31 (22:30): TX I/Q test PASSED (Ron).** CW, PHASE slider 11 -> 1 (TX), APPLY: ms-sdr.log (~/.local/mscc/ms-sdr.log) `Radio_cal_store_iq . TX band 4: offset -26, phase 1 (queued)`, 10 s later `Radio_cal_send_iq ... sent (radio stores it)`, client read back phase 1, no errors. Server code committed (ms-sdr 3.186, trans 3.149, recv 3.150), not pushed. Ron's open list is now empty; SSB voice + splatter check is Stew's.
**2026-10-31 (late): one ALC ceiling for voice and digital (Ron).** Stew's Windows trans 3.152 (e7e4056) uses 1.0 x drive for digital, 0.45 for voice. Ron: WSJT and voice are both just audio feeding the rig; the ALC only stops overdrive, so the ceiling = full calibrated power and mic gain sets the drive. Pi stays on 3.149 (0.89 for both; a 3.150 port of Stew's split was made and undone, never built). Note for Stew: `.mscc-coord/NOTE-FOR-STEW-ALC-ONE-CEILING-2026-10-31.md` (question + options, asks for a 0.89 voice check on his Signal Hound).
**2026-10-31 (late night): NR replaced (option B), recv 3.151, BUILT on the Pi over ssh 23:16 (~/mscc/sdrcore-recv, no warnings), NOT COMMITTED. ssh from git-bash can't resolve `proficio`; PowerShell `ssh -i $HOME\.ssh\proficio_ed25519 ron@192.168.12.199` works.** `denoise()` in dsputils.c = spectral subtraction: 2048-pt FFT at 96 kHz (21 ms, 47 Hz bins), 50 % overlap, sqrt-Hann, per-bin noise = smoothed power min-tracking (rises 3 dB/s), Wiener gain with decision-directed SNR (anti musical noise), gain 0..1 only (never louder). Level 1..100: floor -6..-20 dB, oversubtraction 1..2. Passes audio unchanged for the first ~0.1 s after ON. Adds 21 ms delay while ON. `nr_delay()` removed. Offline test (scratchpad nrtest/t.c, white noise + 1 kHz tone 0.3 s on/off): noise-only -5.8 / -12.1 / -17.3 dB at level 1 / 50 / 100, tone kept (corr 0.98-1.00), output peak never above input. On-air A/B 23:25 local (40 m rag chew, level 50, NR toggled 3x in a 39 s Remote Phones capture, scratchpad nrtest/nrab.py): background between words -40.8 -> -53.6 dB (about -12.7 dB, as the offline test), talk level -29.6 -> -30.7 dB (about 1 dB less), talk-to-gap 11 -> 23 dB, >3 kHz share -13.6 -> -19 to -23 dB, peaks 0.07-0.10 on vs 0.11-0.20 off (no overshoot / clipping), no whistle (top tone 316-469 Hz = voice). Ron: "just a touch of warble, not bad ... much better than it was". 3.151 stays on the Pi. PARKED (Ron): warble fix = smooth the gain over neighbouring bins (+/-2 bins, ~+/-100 Hz), floor unchanged; alt: higher floor. Committed and pushed with note for Stew `.mscc-coord/NOTE-FOR-STEW-RECV-NR-SPECTRAL-2026-10-31.md` (Windows recv has the identical old ALE). Windows recv (mscc-ui/windows-work-tree/SDRcore-recv) has the same old ALE -> note for Stew after the on-air test. sdrcore-recv-DRM has a different NR, already at 3.151 (version clash only if both run; not touched).
**Resume point 2026-10-31 (night), the list:** Pi runs recv 3.150, trans 3.149 (ALC ceiling 0.89, voice and digital), ms-sdr 3.186, pill 3.12 (22 I/Q bands stored). All server code committed and pushed (1e3ecb4, note 836c33f); local `rpi/si5351-drive.md` edits not needed (Ron 2026-10-31: pill runs fine), left out. Ron's open list: empty (TX I/Q test passed, power fixed). Parked: (3) NR: DONE as recv 3.151 (spectral subtraction, on the Pi); warble tweak (gain smoothing across bins) parked; (4) automatic NB (blanking meter optional); (5) sync AM (off the list); (5a) Windows Operator's Guide rebuild: the 60-page PDF reads out of order and mixes terms; no editable source in the repo. Proposed order: what MSCC is + glossary, before you start, install and first start, operating, calibration as one chapter (APPLY stores I/Q in the radio), settings, digital modes, appendices. Open questions: Windows guide first? write it for the client as it is, with a note to Stew about confusing names? Stew: (2) SSB voice + ALC meter on the air, splatter check with a 0.89 ceiling; (2a) one ALC ceiling question (NOTE-FOR-STEW-ALC-ONE-CEILING-2026-10-31.md); (6) blanking meter / NB Low-Med-High display if wanted; (7) heat sink temperature 0x06 in the client; (8) Pi-side ports noted in his CLAUDE.md, new scripts into utility-scripts.

**2026-11-01: Windows Operator's Guide reviewed in full (high effort): `rpi/GUIDE-REVIEW-WINDOWS-2026-11-01.md`** (read it, not this line). Main points: cal now lives in the radio (QRP / TX IQ / RX IQ), so the Save settings text is wrong; TX IQ RESET ALL and Reset configuration don't stick (radio values win at next start: Stew code issue); TX IQ PHASE missing; RX IQ RESET ALL is now APPLY ALL; POTENTIA / QSK described wrongly (Ron's 9/29 text never applied); firmware table old; pictures have no number callouts; source (WeasyPrint HTML/MD) not in repo, ask Stew. Not committed.
**2026-11-01: new Windows guide written (Ron chose Word): `docs/manuals/MSCC-Operators-Guide-Windows-Release-1-DRAFT.docx`**, 43 pages, for client 10.31.2, reorganized as in the review (sections F), pictures reused from the Release 0 PDF, control lists keyed by the printed label (no number callouts needed). Gray DRAFT boxes + Appendix C list what to confirm (firmware table, cal survives FW update, ALC target, Windows NR, reset-to-factory, AMP CAL target) and pictures to retake. Built with python-docx from a light-markup text (scratchpad, session-only); from now on edit the .docx itself. TOC prefilled from a LibreOffice render; Word asks to update fields on open (Yes). Not committed.

## Architecture (per Ron)

WPF client (Windows) <-> **ms-sdr** (controller, UDP :8888) <-> **sdrcore-recv** (:9000)
and **sdrcore-trans** (:9200). ms-sdr is the only interface between the client and the
two DSP cores. ms-sdr is also the **radio controller** (talks to the radio hardware
directly; CAT, USB, keyer code live there). Other ports in ms-sdr `port_defines.h`:
8889 MSCC, 9600 panadapter, 9700 spectrum, 9800 waterfall.
Remote audio: client mic -> sdrcore-trans UDP 9101 (MSA1); sdrcore-recv -> client
phones UDP 9100 (MSA1). CW is not part of remote audio.

## Build (on the Pi, arm64)

```bash
cd rpi/SDRcore-recv-linux  && make clean && make
cd ../SDRcore-trans-linux  && make clean && make
cd ../ms-sdr-linux         && make clean && make
```

`make` installs to `~/mscc/` (replaces the running binary; `BINDIR=...` to avoid).
See also `rpi/mscc-deb/BUILD-SERVERS-ON-PI.md`.
Syntax check on Windows: WSL `Debian` has gcc. Use `-iquote sources -idirafter sources`
(plain `-I sources` pulls in the Windows `pthread.h` and fails).

## Ron confirmed (not issues)

Overdrive code is legacy, ignore it. CW carrier is generated continuously (keyed
elsewhere). No thread locking is fine (works for years).
CW snap (ms-sdr `cw-snap.c`, recv `CMD_CW_SNAP_*`) is not in the client yet and never
runs during FREQ CAL (Ron 2026-09-27). Image check can't overlap FREQ CAL either (client
blocks it). So the `doRxCalibrate` static-index overrun (count change mid-fill) can't happen.
FREQ CAL saturation fix (2026-09-27; mscc 1.0.49 installed on the Pi, FREQ CAL works - Ron): Goertzel mags were stored as int(mag*1e6),
capped at INT_MAX (~-24 dBFS raw IQ) -> ties, first (low) step won, 3-sum wrapped. Now
`calMag*F` floats in `sdrcore.h`, `cal_data` floats, average in double (same 1e6 units vs
Calibration_Low_Limit). Image check keeps the scaled ints (`cal_mag_scaled`, clamped).
WSL syntax OK. Windows recv has the same issue (not touched).
#5 fixes (same batch, in 1.0.49): main.c freq_low init typo; cal log %ld -> %u/%d (udp_thread
217, 299, 755, 1395, 1404); cal_data index clamped to MAX_CALIBRATION_ELEMENT-1.
Future idea (Ron: keep in mind, not now): `goertzel_mag` uses only I (`data[i].real`) -> no
+600/-600 side check, ~3 dB SNR loss. Complex Goertzel (~10 lines) fixes both; level numbers
change, so recheck CALIBRATION_LOW_LIMIT / LOOSE. Mirror is 1200 Hz away, outside the sweep.
  Limit explained (Ron OK'd this wording 2026-09-29): the limit is a bar - FREQ CAL measures how
  loud the tone is (average of low/center/high peaks, udp_thread.c:289); above the bar = found,
  below = fail (freq 0). Normal 5,000,000, LOOSE 1,000,000. Complex Goertzel = a better ear: tone
  reads louder (x2, +6 dB), noise also louder but less (x1.41, +3 dB), so the tone stands out
  more (the 3 dB gain). Everything reads louder, so the bar must go UP: x2 (10,000,000 / 2,000,000)
  = same as today; x1.41 (~7,070,000 / ~1,414,000) = a 3 dB weaker tone passes, noise still
  doesn't (the benefit). Left at 5,000,000 -> noise could pass as a tone.
Future item (Ron 2026-09-29): FM transmit. trans already has it: wire mode 5 -> MODE_FM
(`udp_thread.c:1277`), `main.c:327` path (skips fastconv), `fm_modulate` (`dsputils.c:434`),
fixed `FM_PEAK_DEV_HZ` 5000. Never tested on air (as far as known). Plan:
1. trans: deviation becomes a setting via a new opcode (CMD_SET_FM_DEVIATION, 3000/5000),
   default 3000 (safe). US rule: below 29.0 MHz modulation index <= 1 -> 3 kHz; 29.0-29.7 -> 5 kHz.
2. trans: mic audio is x2 then hard-clipped -> harmonics above 3 kHz break index <= 1. Add a
   limiter + 3 kHz low-pass AFTER the clip. Optional pre-emphasis.
3. ms-sdr: forward the new opcode to trans. Mode letter: 'F' goes to the rig unchanged and
   that is fine - firmware only cares 'C' vs not-'C'; si5351.c A/U/L/C switch only sets
   E_tune_freq, which nothing reads (dead); LO = E_current_LO_freq for all non-C modes; TUNE
   already goes as 'T' and works. display.c is legacy, not in the build (Ron). No 'U' mapping,
   no firmware change.
4. Client: picks 3 or 5 kHz from TX freq vs 29.000 MHz, sends on FM select and on crossing 29.0.
5. Test (Stew, spectrum analyzer): deviation (Bessel null) at 3 and 5 kHz, occupied BW.
6. Windows / Ubuntu trans + ms-sdr same. Ron: comment the code heavily.
DONE 2026-10-04 with a wider blocker (0.98), see Open list 13. Old note: real fix for the -12 kHz spectrum spur =
DC blocker on raw I/Q. Spur is I/Q DC (ADC offset + LO leakage); `complex_shift` runs
`doPanadapter` on raw I/Q, then shifts by fixed 12 kHz (`loFreq`), so DC sits at VFO-12k,
display only (audio never hears it). Add `y = x - x_prev + a*y_prev`, a ~0.9999, on I and Q
right after `framesToComplex` (main.c:342), before `complex_shift`. Caveat: notch needed
+/-6 bins (pure DC would be ~+/-2), so part may be non-steady skirt; test with notch kept,
then `MP_HALF_BINS` 0 to see what's left. Windows/Ubuntu recv same path.

