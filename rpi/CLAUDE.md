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
| `swr-meter/ron/HF_SWR_R4_1/` | **Our copy of the WiFi SWR meter firmware** (ESP32 Arduino; copied 2026-11-01 from Stew's `swr-meter/HF_SWR_R4_1/` @ 67fa644, source only, no build/). Claude edits here; Stew's copy is read-only. |
| Rest of `C:\Users\Ron\.grok\worktrees` | Read-only reference unless Ron says otherwise. |

History: Claude worked in a copy `rpi/sdrcore-claude/` (commits fe0c9a1, 0443c21,
9bb4be2, 4cb8b74). On 2026-09-25 Ron had the changes moved back into the originals
(originals unchanged since 8dee14a) and the copy deleted.
Git repo root: `C:\Users\Ron\.grok\worktrees`.

## Open list (Ron, updated 2026-11-01) - read first on resume

- **Commits (Ron, 2026-10-05): too many commits and pushes.** Finish everything that belongs
  to a piece of work first (code, notes, note for Stew), then ONE commit and one push. No
  follow-up "notes: committed xyz" commits.

**ON HOLD (Ron, 2026-11-02): no new changes until Stew catches up on the important Pi work
(automatic NB, spectral NR, NR Low/Med/High, client controls).** When he pushes: pull, short
review of what he ported, say if the Pi needs a matching change. Don't start new work meanwhile.

**Current list (2026-11-01, after 212107e):**
- **Pi runs:** recv 3.152 (automatic NB + spectral NR), trans 3.149 (ALC ceiling 0.89),
  ms-sdr 3.186, pill 3.12. DRM recv 3.152 built in `~/mscc-drm` (switch:
  `mscc stop; ~/utils/recv-switch.sh drm|normal; mscc start`). All server code committed and pushed.
- **Ron:** Option: NB threshold a bit right of middle (~5x, slider ~750) to also
  catch the near misses (5.4-6.0x seen on 20 m).
- **Ron (2026-11-02): update the guides: QRP power, TX IQ and RX IQ cal are now stored in the
  rig/daughter board.** Windows, Linux and Remote drafts (`docs/manuals/*-Release-1-DRAFT.docx`)
  and the Configurations draft: check every calibration / Save settings / reset text says so
  (APPLY stores in the radio, radio values win at start, power_cal.ini only for bands the radio
  doesn't hold).
- **Ron (2026-11-02): store QRO (amplifier) cal in the radio too.** Today ~/amplifier_cal.ini
  (trans owns, ms-sdr amplifier.c reads, 12 bands, int; -99 = none). Room checked (code read only):
  pill 3.12 = flash log sector 7, 128 KB, 16-byte entries, 4 value bytes: new type 8 (not 6 =
  old clear), no layout change. PSoC MKII (ATU/PTT 3.238+) = EEPROM 2 KB = 128 rows; used rows
  0, 1, 8-19 (cal row per band is full, byte 14 only spare); plan: 12 QRO rows 20-31, same
  one-row-per-band write. Then ms-sdr load/send as QRP, trans save flag, Windows ms-sdr port,
  client (Stew).
- **2026-11-02 later: PSoC comb fix handed to Stew (Ron: Stew does the PSoC, Ron does the pill).**
  Stew's answer (`NOTE-FOR-RON-TX-EDGE-COMB-ANSWERS-2026-11-02.md`): comb on BOTH CW edges, not
  audible on air; his better plan = fade the LIVE slot (copy + raised-cosine scale only during
  fades, SetTxBufAddress), all modes, no soft mute, no host change. Ron's go-ahead:
  `NOTE-FOR-STEW-TX-EDGE-COMB-LIVE-FADE-2026-11-02.md` (8-bit gain table, ISR or dummy-Zero for
  timing, latch the slot index, 5 ms). Old idea below for history.
  **Stew, 2026-11-02 afternoon: PARKED on his side** (`NOTE-FOR-RON-FIRMWARE-FOLDERS-AND-RELEASES-
  2026-11-02.md`): 3.244 (Buff TDs re-pointed, slots 1 ms late) = noise + stalled audio; 3.245
  (scale next slot in place) = RX and TX dead; neither committed, FW back at 3.243 / 4.244. He needs
  the real USB vs DAC slot timing. From SyncSOF.v: index steps at each SOF, forced to 1 when the
  player wraps to slot 0, SOF lands mid-slot at a PLL-held point -> USB is 1 ahead for part of
  each ms and 2 ahead for the rest. Ron: leave it to Stew (his Claude can read SyncSOF.v too).
  Same push: PSoC trees renamed `Proficio-Ultimus-Legacy / -MKII-PTT / -MKII-ATU`, Ultimus trees
  removed, releases in each tree's `Release/` named by version (copy-release.py, -b2 if same
  version differs; bump the minor for every test build); Ron: reopen Creator projects from the
  new folders, fix the post-build path.
- **Ron (2026-11-02): PSoC makes its own CW I/Q (fix for Stew's 12 kHz comb).** Comb = PCM3060
  soft mute 0.5 dB per 8/fs staircase at every non-QSK release / CW key-up, ~-65 dBc (Stew's
  note `.mscc-coord/NOTE-FOR-RON-TX-EDGE-COMB-DAC-SOFT-MUTE-2026-11-02.md`). PCM3060 volume
  stepping ruled out (still a staircase, too many I2C writes). Idea (code read only, not tried):
  trans CW carrier = 12 kHz at 96 kHz = 1 ms USB slot is exactly 12 cycles; 8051 grabs one slot
  from `TxI2S[n]`, builds raised-cosine fade buffers (192 multiplies/ms), plays them with
  `PCM3060_SetTxBufAddress()` (exists, unused); DAC stays unmuted. Grab window < 1 ms because the
  Zero DMA erases each slot behind the reader: in CW point the Zero TDs (`DmaTxStart()`, our code,
  not Cypress) at a dummy byte. Checks: trans sends a steady carrier in CW RX (Pi: `#ifdef RON`),
  8051 time, fade vs key delay. Pill already fades in software (5 ms raised cosine, fw 3.6).
  Reply to Stew: `.mscc-coord/NOTE-FOR-STEW-TX-EDGE-COMB-CW-2026-11-02.md` (pushed 7089904);
  waiting on his answer.
- **Pill CW edges on an analyzer: not checked yet** (pill 5 ms raised cosine, should show no comb).
- **Keyer: Farnsworth on the paddles (Ron 2026-11-02, not started; "not yet").** Goal: letters 18
  wpm, overall 13 wpm (W1AW style) so the other station doesn't go QRQ. Already there: Farnsworth
  for memory play (`0x76` SET_MEM_TEXT_WPM, PIC computes `mem_letter_gap` / `mem_word_gap`) and
  autospace for paddles (`SET_SPACING 0x75` -> LETTERSPACE state, 2 extra dits, latches early
  presses; keyer/main.c ~618-655). Change = PIC only, ~10 lines: top bit 0x80 of the 0x76 byte =
  "paddles too"; LETTERSPACE waits `mem_letter_gap` instead of `2 * dot_delay` when set. No radio FW
  (pill / PSoC) or ms-sdr change: 0x76 byte passed through raw (ms-sdr Pi + Windows, pill cw.c state
  6, PSoC usbvend01/cw.c). Autospace must be on. Letter gaps only first (word gaps by the operator).
  Client (Stew): checkbox sets the bit; ms-sdr echoes the stored value at connect (would show 141
  until handled); test with a script meanwhile. To check later: memory-play Farnsworth letter gap
  added on top of the 1-dit element space -> ~1 dit longer than ARRL, overall a bit slow.
  Next when Ron says: write the PIC change, Ron builds in MPLAB X, tries on the paddles.
- **AM receive spectrum is 140 Hz low (Ron 2026-11-03; do when the Pi hold lifts).** On WWV
  20 MHz in AM the carrier sits ~6 px left of the spectrum center line at 4x zoom (measured
  157 +-15 Hz from a PNG; USB, LSB and CW are on center). Cause: ms-sdr sets the AM receive LO
  with `AM_OFFSET_RX (-11860)`; every other mode, and AM transmit, uses -12000, and sdrcore-recv
  always mixes down by exactly 12000 Hz. Likely reason Ron set 11860 years ago (he doesn't
  remember; this is read from the code): the old AM filter was the SSB bandpass (Lo..Hi), which
  blocks 0 Hz, so the carrier had to sit 140 Hz off. The sdrcore-recv 3.147 AM fix (c9850ca)
  made AM a lowpass (0..Hi), so 11860 is no longer needed. Fix: one line,
  `AM_OFFSET_RX (-11860)` -> `(-12000)`, in `rpi/ms-sdr-linux/source/extern.h:58` (Pi) and
  `mscc-ui/windows-work-tree/ms-sdr-MKII/source/extern.h:47` (Windows ms-sdr; tell Stew). Don't
  change it in the old trees (`linux/`, `Solidus/servers/`): their recv still uses the bandpass
  for AM and would lose the carrier. Leave the PSoC `AM_OFFSET 11860` (`si5351.c`) alone unless
  the PSoC turns out to set the LO in AM. Test: WWV in AM, carrier on the center line, audio
  sounds the same.
- **Waiting on Stew (notes in `.mscc-coord/`):**
  1. Port the new NB to Windows recv; client questions (Pulse min 50 us, threshold Low/Med/High
     = 1/500/750, slider 1001..1009 oddity, send NB state on connect).
     `NOTE-FOR-STEW-RECV-NB-AUTOMATIC-2026-11-01.md`
  2. NR control as Off/Low/Med/High = 0/15/50/85 (client only).
     `NOTE-FOR-STEW-NR-LOW-MED-HIGH-2026-11-01.md`
  3. Port the spectral NR to Windows recv. `NOTE-FOR-STEW-RECV-NR-SPECTRAL-2026-10-31.md`
  4. SSB voice + ALC meter on the air, splatter check at the 0.89 ceiling; one ALC ceiling for
     voice and digital (`NOTE-FOR-STEW-ALC-ONE-CEILING-2026-10-31.md`).
  5. Windows Operator's Guide Release 1 draft: Appendix C answers (firmware table, cal survives
     FW update, ALC target, Windows NR, reset-to-factory, AMP CAL target), pictures, guide source.
  6. TX IQ RESET ALL / Reset configuration don't stick (radio values win at next start).
  7. Heat sink temperature 0x06 in the client; Pi-side ports noted in his CLAUDE.md; new
     scripts into utility-scripts.
  8. Review the Windows parts of `MSCC-Configurations-DRAFT.docx` (setups 1, 2) and
     `MSCC-Operators-Guide-Remote-Windows-Release-1-DRAFT.docx` (7 draft items; key: how a Windows
     host keeps servers running with the one-client lock). Part 3 of the same note: Ron's rules
     (2026-11-01) MSCC UI = current Windows client (NB Thr 1..100 vs 1..1009 scale!, ALC switch,
     Remote window), Ubuntu scripts/utilities = Pi (mscc-init 1.0.18 + text wizard, mscc-volume,
     apt kit order). `NOTE-FOR-STEW-USER-DOCUMENTS-2026-11-01.md`
- **Setup 3 (Ron's station), Configurations draft item: ANSWERED 2026-11-03, draft updated (DRAFT box + Appendix A item 2 removed).**
  Read on the Pi: ms-sdr CAT /dev/tnt0, WSJT-X /dev/tnt1 (pair), Rig Kenwood TS-2000, PTT RTS on tnt1;
  MSCC plays VirtualA -> WSJT-X in VirtualA.monitor; WSJT-X out VirtualB -> MSCC VirtualB.monitor; Mono.
  Ron: nothing set in the Windows client for COM ports or audio; mscc-init on the Pi sets them, and the
  Phones devices too. Unknown: VirtualA_TX / VirtualB_TX exist on the Pi, unused by these settings.
- **Rules (Ron 2026-11-01):** assume Stew makes MSCC UI (Avalonia) match the current Windows client,
  and the Ubuntu scripts/utilities match the Pi. No separate Avalonia review; document the Pi
  versions for both Linux hosts. Linux guide Release 0 reviewed (old versions, Appendix G Pi install
  wrong for today's kit, cal now in the radio).
- **2026-11-01: Linux guide rewritten: `docs/manuals/MSCC-Operators-Guide-Linux-Release-1-DRAFT.docx`**
  (39 pages, Word, Pi + Ubuntu as one, Pi kit = standard, MSCC UI = Windows client). Built from
  scratchpad build_linux.py (session-only; edit the .docx from now on). Release 0 WSJT-X audio was
  backwards: correct = Input VirtualA.monitor, Output VirtualB (also fixed in Configurations setup 4).
  11 draft items (Appendix C). In the note for Stew (Linux guide section).
- **Next:** a Pi / Linux remote guide, when Ron says.
- **2026-11-01: Pi ms-sdr 3.187: WiFi SWR meter fault to the client, BUILT on the Pi 15:35 (~/mscc/ms-sdr),
  NOT committed, running only after mscc stop/start.** Meter is UDP unicast: at a Pi station it talks
  to ms-sdr only, which relayed SWR/FWD/REF but never the fault (client got Fault=false; the 0x30
  "SWR METER FAULT" text is dropped by the client). New extended sub 0x0F CMD_SET_SWR_FAULT:
  [fault][online][meter IPv4 4 bytes], sent at once on a fault change, with each update (<= 2/s),
  and offline; not gated by SWR_METER_TO_GUI. Files: swr_wifi_meter.c, usbavrcmd.h, version.h (187,
  synced back from the Pi). Client side = Stew: `.mscc-coord/NOTE-FOR-STEW-SWR-FAULT-TO-CLIENT-2026-11-01.md`.
  Windows ms-sdr has no meter code (client listens directly there).
- **2026-11-01 17:00: Pi ms-sdr 3.188 + trans 3.150 BUILT and RUNNING on the Pi (Ron restarted, old client 10.31.x, no calibration until 11.1.0;
  committed): port of Stew's Windows round 1 (ms-sdr 3.189 / trans 3.154 / client 11.1.0, 49d8c90).**
  0xA2 slider = live only; 0xA2 | 0x8000 (POWER_CAL_SAVE_FLAG) = save: ms-sdr writes the radio, trans
  writes power_cal.ini (trans reads the flag from buf[2] bit 7; value stays buf[1]). Startup copy from
  the radio to trans sent WITH the flag. New 0x2D CMD_SET_CAL_STATUS ms-sdr -> client: int32 kind (1 RX
  IQ, 2 TX IQ, 3 power) | flags<<8 (bit0 store) | mask<<16; sent at Session_Claim, end of
  Radio_cal_load_iq, after each radio write (masks reread). Includes the 3.187 SWR fault (0x0F).
  **Needs client 11.1.0** (old client never sends the save flag = QRP cal not saved). New trans with an
  old ms-sdr never saves power_cal.ini (Stew's trans comment says otherwise; code saves on flag only).
  Files: ms-sdr power_calibration.c, main-controller.c, usbavrcmd.h, extern.h, swr_wifi_meter.c,
  version.h (188); trans udp_thread.c, extern.h (150).
- **2026-11-01 19:41: Pi trans 3.151 (2 TONE, port of Stew's Windows 3.155 on 0x88) + ms-sdr 3.189 (0x2E
  CMD_SWR_METER_RESET: client RESET -> HTTP reset to the meter in a detached thread) BUILT on the Pi, NOT
  running until mscc stop/start, NOT committed.** Stew pulled 11-01: client 11.1.5 (0x0F handled, installer
  R11-1-5), Windows trans 3.157 (2 TONE + voice ALC 0.89; 0.45 was a bad StarTech USB card). Note:
  `.mscc-coord/NOTE-FOR-STEW-2TONE-PI-AND-0x2E-2026-11-01.md`. Running since 19:46. **Fault test PASSED 19:51
  (Ron, client 11.1.5):** meter fault latched -> client showed fault + RESET, Ron pressed the RESET
  button ON THE METER ITSELF (corrected 2026-11-03; this line used to say the client's RESET) -> meter
  cleared, ms-sdr relayed it. The client's own RESET was first tested 2026-11-03 (client HTTP straight
  to the meter, after fixing a typo in MSCC_Client.ini SWR_HF_IP=192.156.12.222 -> 192.168.12.222): worked.
  Not tested yet: client RESET from a remote client (Tailscale) - the client sends the reset straight
  to the meter's LAN IP, and doesn't use ms-sdr's 0x2E reset yet. **2 TONE PASSED 20:12 (Ron, 20 m USB, full power; button is on the RX/TX tab;
  panadapter stops in TX so tones/IMD not seen).** Note: `NOTE-FOR-STEW-2TONE-PI-TESTED-2026-11-01.md`.
  Open: 0x2E RESET path (needs Stew's client send).
- **SWR meter firmware (Ron 2026-11-01):** auto-tune on 30 m runs longer than the meter's time limit, so
  the meter faults. Firmware source is NOT in the tree (only swr-meter/ Python app + summary); Ron asks
  Stew to add it. 11-01 evening tests: trips ~1 s into a 15 m auto-tune (tune power 43: fwd 61 ref 25.00
  swr 4.55; 34: fwd 38 ref 25.00 swr 9.82); 30 m tunes 5.5-7.4 s did not trip. Ron: meter "Fault Trip
  Count" is already at max and is too short for a slow tuner match (firmware limit).
  Stew added the source 11-01 (67fa644, swr-meter/HF_SWR_R4_1). Cause: 1 count = one FWD+REF pair ~2.4 ms,
  max 24 = ~58 ms; REF table tops at 25.00 W (clipped). **FIXED in our copy swr-meter/ron/HF_SWR_R4_1
  (git-excluded via .git/info/exclude until done): TUNE ALLOWANCE** (tuneGraceSec 0..10 s, default 5;
  SWR not counted for the first N s of a new TX = RF after >= 2 s with none; web field + prefs + saved list;
  no touch control). Built with arduino-cli (IDE bundled; esp32 3.3.11, lvgl 9.4.0, TFT_eSPI 2.5.43, Stew's
  User_Setup.h/lv_conf.h copied into the libs) -> swr-meter/ron/build/; Ron OTA-loaded it (http://meter/update)
  20:43. Test PASSED 20:45 (Ron): 20/15/30/15 m tunes, no fault, client showed readings.
  **21:00 allowance now ONLY FOR TUNES (Ron):** meter /api?tune=1|0 flag (RAM, self-clears after
  tuneGraceSec; JSON "tune"); 2 s idle rule removed. **Pi ms-sdr 3.191** (running): Tune_button sends
  tune=1 (waits <= 300 ms for the answer) BEFORE keying, tune=0 after (thread); skipped if meter
  off/offline. Also reset fix: ms-sdr asked /reset (no such page, 404 taken as OK) -> now only
  /api?action=reset, result read from the answer ("fault":0). Test PASSED 21:06 (Ron): 15 m tune 16 s,
  tune=1 OK ~20 ms before key, no fault. Old firmware ignores tune=1 (logged "no tune flag"), harmless.
  ms-sdr committed; firmware still git-excluded until Ron says done. Next: note for Stew then.
  21:18: touch control "Tune Allow (s)" on the meter SETTINGS page (rows respaced to 52 px:
  52/104/156/208/260; -/+ 0..10, 0 = OFF, saved on Back). Ron: fits and works. Client 11.1.5 dropped
  TX 2 ms after the relayed fault (good). Then find the limit and fix. Also pending Ron's decision: should ms-sdr itself stop
  TX on a meter fault (backstop; WSJT-X on the Pi keys via CAT, so the client can't stop it).
- **Parked:** NR warble fix (Ron 2026-11-01: leave it, NR works well; lower level = less
  warble). Sync AM (off the list). Complex Goertzel for FREQ CAL and FM transmit (future ideas,
  see "Ron confirmed" below). CW ramp length by speed (Ron 2026-11-02, just a thought): the key
  delay (20-40 ms) already shows each element's length ahead, so the ramp could be chosen per
  element (longer normally, shorter only for very short elements/spaces), any key, no speed
  needed. Pill: easy (table + step count), only if 5 ms proves a problem on the analyzer. PSoC:
  only after the own-I/Q fix (doesn't fix the staircase).
  Pill TX underrun while keyed (2026-11-02): `audio_i2s_tx_block()` plays a silent block at once
  on a USB underrun (hard drop = click). TESTED 11-02 with `~/utils/audio-stats.py mark` / `since`:
  2.1 min CW non-QSK = out_under 0, out_drop 0, out level steady 1552; 1 underrun in 6.5 h before.
  Fix (repeat the last good block or ramp down while keyed) only if one ever shows up.

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

**2026-11-01: automatic NB, recv 3.152, BUILT on the Pi (last build 08:53, copied to V:), committed and pushed with notes for Stew.** Files: `SDRcore-recv-linux/sources/blanker.c` (rewritten), `sdrcore.h` (blanker_state), `extern.h` (3.152). No client / ms-sdr change: NB button = on/off; threshold slider = sensitivity K 4 (slider max) .. 8 (slider min), default 6 (Ron's slider sat at min -> K 8); width as before. Pulse = |I/Q| > K x running mean (2 ms, pulses left out); a "pulse" longer than the width (max 2 ms) is a signal: un-marked before the gain falls (delay = width/2 + max pulse, 0.47 ms at width 30). Gain: 0 within +/- width/6 of any pulse sample, raised-cosine back to 1 at width/2 (no restart jump). Self-protection: >400 pulses/s raises K (up to 4x), <200/s lowers it. 50 ms learn after each (re)start. Log every 10 s while on: `Blanker . N pulses/s, K ...`. Offline test (scratchpad nbtest/t.c, WSL): noise only 0 false at K 6 / 8 (1.3/s at K 4); 30x pulses -27 dB at K 6; 40 dB SSB with instant start 0 false at K 6 (first version locked up: 60 % blanked; fixed). Next: build on the Pi, Ron listens, tick capture A/B (scratchpad ticks.py from 10-31 is gone; redo). On-air 08:30 (20 m USB, K 5.95, width 40, NB toggled): ticks >18x only with NB off (28-58x), NB on 0.8/s max 18x vs off 1.2-1.5/s; blanker 1-2 pulses/s; no holes. Slider test 08:34: every step restarted the blanker (ring zeroed = up to 4 ms silence at width 400): fixed 08:38, slider steps keep level and audio (K only = no change; width = ring resized, oldest dropped or oldest stretch repeated); log `Blanker . setting: K, width`. Offline 69 steps, 0 holes. Width now read as MICROSECONDS (client slider 10..510 is labelled uS; recv used samples = 10.4x longer, up to 5.3 ms); 200 us default; built 08:42. Threshold range unchanged (K 4..8, Ron OK). Note for Stew: Windows recv also reads width as samples. On-air 08:47 (200 us, K 6): big clicks (>25x) off 0.41/s, on 0.05/s (~9 of 10 removed); one 54x passed with NB on, cause unknown (too long >0.2 ms / busy band, mean 9x higher than 08:18 / not noise). Built 08:53: log counters per interval + summary when NB turned off: `N pulses/s (biggest Nx), too long N/s (biggest), near N/s (0.67K..K, biggest)`. Decide fix from data (maxrun fixed 1 ms if too long; linear-prediction detector if busy band). Ron to restart MSCC. On-air 08:55 (60 s NB on, 200 us, K 6): no audio tick >26x, blanked 1-2/s (biggest 50-78x), too long 0 in every interval (cause 1 ruled out, maxrun change not needed), near 0.3-3/s at 5.4-6.0x (option: threshold to ~5x). DRM copy (git-excluded, local only) ported 2026-11-01: blanker.c + sdrcore.h copied, extern.h 3.152, WSL syntax OK, NOT built on the Pi (V:\SDRcore-recv-DRM still has the old blanker.c / sdrcore.h / extern.h). Notes for Stew: `.mscc-coord/NOTE-FOR-STEW-RECV-NB-AUTOMATIC-2026-11-01.md` (port blanker.c + sdrcore.h; client questions: Pulse min 50 us, threshold Low/Med/High = 1/500/750, 1001..1009 oddity, send NB state on connect) and `.mscc-coord/NOTE-FOR-STEW-NR-LOW-MED-HIGH-2026-11-01.md` (NR slider -> Off/Low/Med/High = 0/15/50/85, client only). DRM BUILT on the Pi 09:08 (`~/mscc-drm/sdrcore-recv`, 3.152). CAUTION: DRM `make clean` without BINDIR deletes ~/mscc/sdrcore-recv (normal recv): happened 09:08, normal recv rebuilt from the same source at once; warning added to DRM-NOTES.md. Always `make clean BINDIR=$HOME/mscc-drm`. NR warble fix: Ron 2026-11-01 leave it (NR works well; lower level = less warble). Then port to `rpi/sdrcore-recv-DRM` (same old blanker.c) and a note for Stew (Windows recv, maybe Low/Med/High labels).

**2026-11-01: Remote guide rewritten (Word, same style as the Windows draft), WINDOWS ONLY (Ron: Windows client + Windows servers; Pi/Linux remote differs and needs its own guide, not written yet): `docs/manuals/MSCC-Operators-Guide-Remote-Windows-Release-1-DRAFT.docx`**, 20 pages, client 10.31.2, server names as Windows .exe; session lock and ports also checked in Windows ms-sdr-MKII / SDRcore-recv / SDRcore-trans; pictures from the Release 0 PDF. Corrections from code: Phones / Digital chosen inside the Remote Audio window (main-window Phones / Digital turn Remote off); Remote off at each start; one client at a time (ms-sdr session lock, freed by Stop or ~45 s); Remote Digital CAT = client SETTINGS COM Port; closing the window keeps audio. 7 DRAFT items in Appendix C (Windows-host steps with the session lock, ipconfig picture, fixed host address, what a refused client shows, retake chapter 3 pictures, mic-level wording vs Windows ALC 0.45, WSJT-X audio picture). Built with python-docx from the Windows draft (scratchpad, session-only); edit the .docx itself from now on. Not committed.

**2026-11-01: `docs/manuals/MSCC-Configurations-DRAFT.docx` (13 pages, Word, same style): client-server explained (ch 1: client, the 3 servers, ship bridge / engine room picture, always client-server even on one PC per Ron), then the five setups in plain English** (Ron): 1 Windows local, 2 Windows client PC1 + Windows servers PC2, 3 Windows client + Pi/Linux servers, 4 Pi/Linux local, 5 Pi/Linux client + Pi/Linux servers. Each: diagram, who for, what runs where, client settings, audio, digital programs, which guide. Key rule (Ron): Windows host = client owns the COM port (SETTINGS → COM Port; setup 2 WSJT-X on the client with Remote Digital); Pi/Linux host = host owns everything (mscc-init sets COM ports /dev/tnt + audio, WSJT-X runs on the Pi, reached by VNC; Windows client only operates + Remote Phones). 4 DRAFT items (setup 2 host keeping servers alive; setup 3 mscc-init port/device names + what the client does for Pi digital audio; setups 4/5 MSCC UI steps, wait for the Avalonia review). Next (Ron): the Avalonia client, then a Pi/Linux remote guide. Not committed.

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

