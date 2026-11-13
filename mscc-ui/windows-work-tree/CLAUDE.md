# Windows work tree: working notes (Stew's Claude)

Notes from Stew's Claude sessions on the Windows side, kept here so Ron's Claude sees them on
every pull. Newest first. Detailed write-ups for Ron go in `.mscc-coord/NOTE-FOR-RON-*.md`.

**Shared rules for Ron and Stew (keywords like "finished", the release checklist, keeping trees in step): `CLAUDE.md` at the repo root.** Added by Ron's Claude 2026-11-13; this file stays Stew's.

## Where things are

- Servers: `SDRcore-trans/`, `SDRcore-recv/`, `ms-sdr-MKII/` (VS 2022; trans/recv config
  **MFC | x86**, ms-sdr **MFC | Win32**). Output: `mscc-ui/Release/windows-wpf/`.
  Installed by copying exe + pdb to `C:\mscc-net9`.
- Client (WPF): `mscc-mscc/mscc-new/MSCC.sln` (Release | Any CPU). The build copies
  `MSCC.Wpf.*` / `MSCC.Core.*` to `C:\mscc-net9` itself (`CopyToMsccNet9` target).
  Version in `MSCC.Wpf/ClientVersion.txt` (bumped per build).
- Radio firmware: `radio-psoc-firmware/` (PSoC Creator 4.4, Keil; builds on Shack only).
  **Since 11-02:** one tree per family, `Proficio-Ultimus-Legacy/`, `Proficio-Ultimus-MKII-PTT/`,
  `Proficio-Ultimus-MKII-ATU/` (Creator projects keep their `Proficio-*` names); the separate
  `Ultimus-*` trees are deleted. Releases go to each tree's `Release\` as
  `<Radio>-<major>.<minor>.cyacd/.hex` (Proficio from the Creator post-build via `copy-release.bat`
  -> `copy-release.py`, Ultimus from `build-ultimus.bat`); never overwritten (a different build of
  the same version becomes `-b2`). **Bump FIRMWARE_VERSION_MINOR for every test build.** Geminus
  trees use the same `copy-release.bat` post-build (folders `Geminus-MKII\release`, `Geminus-Legacy\release`). Old
  top-level `release\` = archive. Post-build lines use the full path (`${ProjDir}` fails).
- Machines: **Shack** = build PC (VS, PSoC Creator). **NEW-HP** = test PC with the radio
  (Proficio MKII PTT, FW 3.242), Signal Hound SA, good mic.

## Versions in the field (2026-11-01)

| Part | Version | Main changes |
|---|---|---|
| trans | 3.157 | **voice ALC ceiling 0.89 x drive, as on the Pi (3.157)**, **2 TONE: 0x88 puts 700 + 1900 Hz in place of the mic, USB/LSB, through the voice path (3.155)**, **QRP cal: power_cal.ini written only on the save flag (3.154)**, ALC rework (\|I+jQ\| envelope), **1.0 x drive for digital (3.152)**, voice ALC level meter, 6 kHz audio LPF before the SSB/AM mixer (LSB image fix), TX I/Q phase, TX I/Q from the radio (0x2C), **digital (VAC) key-up gate 200 ms + 10 ms fade (3.153)** |
| recv | 3.151 | RX stream kept running at PTT (muted, not stopped), Ron's AM lowpass + DC block and noise blanker fix, **RX I/Q from the radio (0x2C)** |
| ms-sdr | 3.189 | **QRP save step (0xA2 bit 15 = save; radio written only then), radio cal status to the client on 0x2D at connect / after each radio write (3.189)**, PTT poll `r == 1`, power cal in the radio (0xC8), CW + TUN stuck TX, 0xFE 0 release, 0x30 messages forwarded, firmware type in log, PPM cal start point, TX I/Q phase, **TX/RX I/Q stored in and loaded from the radio**, **TX I/Q from CW mode fix** |
| client | 11.1.5 | **WiFi SWR meter fault relayed by Pi ms-sdr 3.188 (extended 0x0F: fault, online, meter IP) handled; fault kept with every relayed reading; RESET falls back to the 0x0F IP (11.1.5)**; **2 TONE button next to Tune: arms two-tone, Tune then sends it (switches to USB if needed), disarmed on Stop (11.1.3)**; **FREQ CAL: waits for the previous cal tab's restore before CW (USB bug fixed), "must be in CW" note, opens at 4× left-panel zoom and restores it on leave (11.1.2)**; **QRP CAL "Save this calibration?" Yes/No (Reject gone), lamps follow the radio's power mask (0x2D), I/Q APPLY renamed SAVE, Start / Save settings / POTENTIA tooltips, Full Power renamed AMP (11.1.0)**; TX I/Q PHASE slider; LSB/USB/DIG-U/AM filter buttons send the index (Ron's LSB note 10-31); cuts pushed once at connect |
| FW MKII PTT | 3.243 | DAC soft mute on TUN/PTT release, mode-change drain; **3.243: DAC kept muted in non-QSK RX (no effect on the comb, see 11-02)** |
| FW Proficio Legacy | 1.236 | Ron's 1.235 (CW key delay + fades, 0xC8 cal store, SSB-after-CW fix) + TUN/PTT DAC fade and mode-change drain (= MKII 3.241/3.242). In `Proficio-Ultimus-Legacy/` (was `Proficio-Legacy/`); `Proficio-Legacy - Copy` kept as reference. Tested on a Legacy rig 10-31 |
| FW Proficio MKII ATU | 4.244 | **4.244: `Key_Delay_isr` RISING_EDGE (was DERIVED: ms tick fast, PA hot-switched), DAC muted in non-QSK RX.** = MKII PTT 3.242 app code (cal store, CW key delay + fades, TUN/PTT DAC mute, mode drain) + Elecraft T1 port kept (`t1.c`, `T1_Main()` in the beater, `STATUS_ATU_0`). Schematic: `Key_Delay_Timer` (FF 16-bit, CW_Clock, period 10, continuous, reset tied 0) + `Key_Delay_isr` added on the CW Management sheet. Built 10-31, **tested on a radio 11-01**; T1 untested (no T1 here) |
| FW Geminus MKII | 2.153 | **0xC8 calibration store (2.152)**, **DAC soft-mute shaping on TUN/PTT and CW (2.153)** |
| FW Geminus Legacy | 5.153 | **11-02: Geminus MKII 2.153 app code ported (same schematic/pins); major 5, `E_keyer_installed = FALSE` (no onboard PIC). Old Legacy CW (DIN-gated, no fades) replaced. Bootloadable dependency fixed to `..\bootloader\...\Release\bootloader.hex`. Built, not yet tested on a radio.** Trees stay separate (Stew: different radios) |

## Ultimus firmware built from the Proficio trees (2026-10-31)

- Ultimus = hand-built (through-hole) Proficio: same code, major 6/8/7 and USB product
  "Ultimus". The `Ultimus-*` trees were frozen 9/20 copies (only basic-plus.h major and a
  hand-edited generated USBFS_descr.c differed).
- `radio-psoc-firmware/build-ultimus.py` (+ `.bat`): after a Creator build of the Proficio tree,
  copies the .cydsn to git-ignored `_ultimus-build/`, recompiles the app with `DF(ULTIMUS)`
  (`#ifdef ULTIMUS` in basic-plus.h picks the major), patches the USB product string, links and
  runs CyHexTool with Creator's options, writes `<tree>\Release\Ultimus-<radio>-<major>.<minor>.cyacd/.hex` (since 11-02).
  Trees without the switch are skipped.
- Done: Legacy (Ultimus 6.236 confirmed in MSCC on Stew's Ultimus Legacy), MKII ATU (Ultimus
  7.243, tested 11-01), MKII PTT (Ultimus 8.242, tested 11-01).
  All three Proficio trees have the switch; `build-ultimus.bat` builds all three.
- MKII ATU notes: the T1 data line (ATU_0, status bit 0x10) is the PTT input on MKII PTT, so the
  ATU firmware reads no PTT input (`E_PTT` stays 0, as in 4.151). Nothing ever sets `T1_Tune_Request` (old code too):
  the T1 only gets band data, no tune command from MSCC. The ATU tree's CW smooth-tune
  `si5351a-cw.c` was replaced by the PTT one. Post-build now runs the tree's own `copy-release.bat`.

## Digital TX fixes, trans 3.152 / 3.153 (2026-10-31)

- 3.152: the 3.149 ALC ceiling (0.45 x drive) also applied to digital audio. A WSJT-X tone is
  constant envelope, so it was held to 0.45 of the drive = 0.2 of the QRP CAL power (Proficio
  5 W cal -> 1 W, ALC meter pegged red; same on every radio and PC). Digital audio
  (`DIGITAL_AUDIO`, `REMOTE_DIGITAL_AUDIO`) now uses 1.0 x drive (`ALC_DIGITAL_CEILING` in
  dsputils.c); voice keeps 0.45. Full power confirmed.
- 3.153: 2.8 kHz-wide bump about 50 dB down at every key-up, fixed at the TX passband centre.
  With one VB-Cable, recv's RX audio goes into CABLE Input and comes back on CABLE Output
  (trans' digital input); recv mutes at TX on, but ~100 ms of buffered band noise reached the
  modulator past the 35 ms gate. Digital gate now `TX_MIC_GATE_DIGITAL_MS` 200 ms, then
  `TX_MIC_FADE_MS` 10 ms linear fade-in (main.c callback); mic stays 35 ms. Confirmed gone.
  (Two virtual cables would also avoid it.)

## Ron's answers 10-31 (`.mscc-coord/NOTE-FOR-STEW-CAL-*`, `...GEMINUS-1MS-TICK...`)

- I/Q slot layout matches the Black Pill (`cal_iq_t`: int16 gain, int16 phase); 0x2C kept, the
  Pi will use the same number.
- 0xC8 clear came out in 3.239 because it was a no-data control request (same shape as the
  save that hung USB). Bring it back for reset-to-factory as a control READ: type 6, band or
  0xFF, handler clears valid bits + marks dirty + returns `E_cal_status`; Cal_Main rewrites the
  rows. No zero-length OUT, no EEPROM work in the handler. Tell Ron when the host side is
  ready (pill gets the same handler).
- Geminus main-loop 1 ms tick: fine for now (it only runs slow). Later: hardware 1 ms timer ISR
  as on the Proficio; first scope a spare pin per main-loop pass (must stay well under 0.5 ms).

## Calibration travels with the radio (2026-10-31)

- 0xC8 slots: TX I/Q and RX I/Q = int16 gain (IQ_OFFSET steps) + int16 phase (IQ_PHASE steps,
  0 for RX), little endian; power = uint16. Band index = MSCC record order (0 = 10 m ... 9 =
  160 m, 10 = 630 m, 11 = 2200 m).
- ms-sdr: APPLY (0x57) also queues the band's I/Q for the radio (`Radio_cal_store_iq`), sent
  out of TX by the Flusher (`Radio_cal_save_tick`). At start `Radio_cal_load_iq()` (after
  `Radio_cal_load_power()`) reads the valid masks and sends each band in the radio to trans
  (TX) / recv (RX) as **0x2C CMD_SET_IQ_RECORD**, int32 = record (bits 0-7) | offset int12
  (bits 8-19) | phase int12 (bits 20-31). The cores update their ini file and apply it if
  that band is in use. Bands not in the radio keep the ini value.
- Tested 10-31: Proficio (15 m TX I/Q) and Geminus (630/2200 m power + TX I/Q) restart from
  the radio; the Geminus on a fresh PC loaded its cal from the radio.
- ms-sdr 3.188: `G_iq_cal_active` lets mode changes reach the radio during an I/Q session
  (power cal still blocks them). Before, TX I/Q started from CW left the radio in CW, which
  ignores TX on (Geminus TX_Main only runs out of CW): no RF.

## Geminus MKII firmware 2.152 / 2.153 (2026-10-31)

- 2.152: calibration.c from the Proficio appended to settings01.c (no new source file),
  0xC8 handlers in usbvend01.c, `Cal_Init` / `Cal_Main` in main.c. EEPROM rows 8-19.
- 2.153: no 1 ms timer ISR on the Geminus, so `E_ms_tick` counts in the main loop from
  STATUS_BEAT (changes every 0.5 ms). TUN/PTT (tx.c): DIN blank + DAC mute, T/R + BS2 + AMP
  on, 25 ms, DIN on, unmute; release: mute, 18 ms, AMP + BS2 off, 10 ms, T/R off, unmute.
  Mode-change drain as Proficio 3.242. CW (cw.c): relays latched per session (as before),
  elements keyed by the DAC soft mute with a fixed 30 ms key delay (`CW_Key_Delay_Tick` once
  per ms from main.c), 14 ms rise lead, DAC muted in CW idle. Differences from Ron's Legacy
  copy: fixed 30 ms delay (Legacy: TX hold / 16, 20-40 ms) and DIN stays open for the session
  (Legacy also gates DIN per element).
- Stew's scope: ~34 ms LED on to full carrier, trailing edge follows the hold timer, clean on
  the Signal Hound.
- Watch out: after a reflash Windows may make the radio ("N- Multus IQ Sound") its default
  playback device; recv then finds no speaker (falls back to the default output, skipping
  Multus) and stops. Set the default output back. (recv should pick the first non-Multus
  output instead: to do.)

## LSB filter buttons (client 10.31.x, 2026-10-31)

- Bug (Ron): the client turned button indexes into RF edges in Hz and back; in LSB the edges
  are negative/swapped, so every LO/HI CUT went out as index 0 (LSB always 500 Hz low cut).
- Fix: `SetLowCutIndexAsync` / `SetHighCutIndexAsync` send the index in LSB/USB/DIG-U/AM;
  Hz edges are display only. CW/FM unchanged. Client pushes LO/HI CUT once at connect
  (ms-sdr restores its own last indexes from user_controls.ini, which could differ).
- Tested on Stew's MKII; reply in `.mscc-coord/NOTE-FOR-RON-LSB-FILTER-FIXED-2026-10-31.md`.
  Avalonia client not checked yet.

## TX I/Q phase (Windows, 2026-10-30)

- Ported from Ron's Pi trans (dcdebbc): `IQ_PHASE=` per line in `iq.ini` (read by name,
  missing = 0), steps of 0.05 deg, applied as Q' = Q cos + I sin on the TX output.
- Opcodes: client -> ms-sdr -> trans **0x2A** (int32 steps). trans reports the value in
  use to ms-sdr as **0x2B** + int32 (after `CMD_SET_IQ_BAND` 0x58 and after each 0x2A);
  ms-sdr forwards it to the client. (On the Pi, 0x2B replies to the sender with a 10-byte
  packet; Windows uses the ms-sdr path like 0x8B.)
- 0x57 (commit) saves offset + phase, also outside a TX I/Q session.
- Client: PHASE slider on the TX I/Q tab, ±100 steps (±5 deg), mouse wheel / arrows = 1 step.
- Stew's MKII results: 40 m 0, 20 m +1, 15 m -4, 10 m -8 (Ron's MKII 10 m: -6).
  "Image is buried now."

## Calibration in the radio (0xC8) and factory values: decisions (2026-10-30)

- The radio stores **only real calibration** (per band, valid flag = calibrated). Factory
  values never go into the radio; they live only in `C:\mscc-net9\factory` (shipped by the
  MSCC installer), so new factory numbers can go out without flashing.
- Startup order: radio values win; bands the radio does not hold keep the local file
  value. (ms-sdr 3.185 changed this from the Pi code, which put factory values there and
  overwrote a calibrated `power_cal.ini` on the first start after the upgrade.)
- Firmware EEPROM row per band: RX I/Q 4 bytes, TX I/Q 4 bytes, power 2 bytes, 1 spare.
  The firmware does not interpret the bytes. Proposal: TX I/Q slot = int16 offset +
  int16 phase, little endian (same for the Black Pill). **To agree with Ron.**
- Planned: source tags in the cal files (`SOURCE=U` user / `F` factory; untagged = user,
  so nobody re-calibrates), a factory set version, TX/RX I/Q stored in the radio,
  reset to factory later (needs the 0xC8 clear back, removed in 3.239: ask Ron why).
- Geminus MKII firmware: 0xC8 added in 2.152 (2026-10-31).

## TX edges: ATU timer fix and the 12 kHz comb (2026-11-02)

Details and pictures: `.mscc-coord/NOTE-FOR-RON-TX-EDGE-COMB-DAC-SOFT-MUTE-2026-11-02.md`,
`.mscc-coord/RESULTS/2026-11-02-tx-edges/`.

- **ATU 4.244 / Ultimus 7.244:** `Key_Delay_isr` was DERIVED (level on the timer TC) in the ATU
  TopDesign, RISING_EDGE in the PTT. The ms tick ran fast, every delay was too short, and the PA
  switched with RF up (wide-band noise at key-down and release). Set to RISING_EDGE by Stew;
  tested clean.
- **Comb 12 kHz apart, about -65 dBc, ~16 ms at every non-QSK ramp-down** (TUN, 2 TONE, SSB/DIG
  release, each CW key-up; all firmwares): the PCM3060 soft mute steps 0.5 dB every 8/fs = a 12 kHz
  staircase. QSK skips the ramp: no comb, but hot switching (wide-band edges). Not fixed. Options
  sent to Ron: leave it, or fade in trans and delay radio PTT off until trans's queued audio is out.
- PTT 3.243 / ATU 4.244 also keep the DAC muted in non-QSK RX (unmute only after PA on). No
  effect on the comb; harmless; Ron to say keep or revert.
- Beep in Phones at the end of 2 TONE / click at the end of TUN: guess = recv unmutes during the
  radio's 28 ms ramp-down. Not checked yet.

## SWR meter fault from the Pi, client 11.1.5 (2026-11-01)

- Ron's Pi ms-sdr 3.188 relays the WiFi meter state as extended sub **0x0F**
  `[fault][online][IP a.b.c.d]` (`swr_wifi_meter.c`, not gated by `SWR_METER_TO_GUI`). Windows
  ms-sdr has no meter code (the Windows client listens on 6999 itself).
- Client: `UdpRadioService` keeps `_radioSwrFault` and sends it with every relayed F/R/SWR reading
  (they had `Fault = false` hard-coded, which would clear a fault at the next reading). online = 0
  zeroes the meters and clears the fault. `RadioSwrMeterIp` (memory only) is the last RESET fallback.
- Proposed to Ron: **0x2E `CMD_SWR_METER_RESET`** client -> ms-sdr -> `Swr_wifi_request_reset()`
  (exists, no caller), so RESET works from a remote client that can't reach the Pi's LAN
  (`NOTE-FOR-RON-SWR-FAULT-CLIENT-AND-RESET-2026-11-01.md`). Client side after his OK.
- trans `Set_QRP_calibration` comment fixed (Ron): an old ms-sdr never sets the save flag, so
  nothing saves. Source only, not rebuilt.

## 2 TONE and the ALC ceiling, trans 3.155-3.157 / client 11.1.3 (2026-11-01)

- 2 TONE reuses **0x88 `CMD_SET_TWO_TONE`** (both ms-sdr trees already pass it to trans; no new
  opcode; 0x2E/0x2F still free). trans `twoToneAudio()` (dsputils.c) replaces the mic samples
  (700 + 1900 Hz, 0.8 each, 5 ms fade-in) before the audio LPF / SSB modulator / ALC; no speech
  processor. Level follows the SSB slider. Client: `TwoToneArmed`, `StartTwoToneTx` /
  `StopTwoToneTx` in `OnTuneModeChanged`.
- **StarTech USB sound card = 1 kHz comb.** Used as the Phones mic it put a comb exactly 1 kHz
  apart, ~48 dB under the carrier, plus ~20 dB more close-in noise, on every transmission (TUN
  too, although trans zeroes the mic in TUN/CW) whenever trans had it open. Followed the device
  between Phones and Digital; gone on a virtual cable and on the PC's own sound card. Device thrown
  out. Guide note: comb on TUN -> try the mic on a virtual cable. The 10-30 "voice splatter" that
  set the 0.45 ceiling was most likely this device.
- 3.156 (20 ms ALC envelope peak hold) changed nothing: not committed as such, removed in 3.157.
- **3.157: voice ceiling 0.89** (dsputils.c `ALC_CEILING` and udp_thread.c
  `CMD_SET_ALC_MULTIPLIER`). Signal Hound, 630 m, SSB 100, each tone vs TUN carrier:
  Digital (1.0) -7.3 dBc, IMD3 ~33 dB under the tones; Phones at 0.45 -14.6 dBc; **Phones at 0.89
  a little under -8 dBc, IMD3 -33.85 dB** (PA still fine). ~6 dB more voice peak power: that was
  Ron's missing SSB power. The ALC settles ~2 dB under its ceiling, so 0.89 puts voice PEP near
  the calibrated power. 0.8 vs 0.89 settled: 0.89.

## Earlier today (details in `.mscc-coord/NOTE-FOR-RON-SSB-TX-RESULTS-WINDOWS-3.149-2026-10-30.md`)

- Broadband SSB splatter = recv `Pa_StopStream` at PTT on the shared USB audio device.
- LSB image = no audio low-pass before the DSB mixer (21-24 kHz mic hiss -> opposite sideband).
- ALC ceiling 0.45 x drive (0.89 since 3.157); meter shows the peak level against the ceiling.

## Open / next

- **Round 1 done 11-01** (ms-sdr 3.189, trans 3.154, client 11.1.0; tested by Stew on several
  radios, incl. a fully calibrated one showing green lamps): QRP save step, 0x2D status masks,
  APPLY -> SAVE, tooltip fixes. Pi ms-sdr + trans side is Ron's (`NOTE-FOR-RON-ROUND1-QRP-SAVE-0x2D-2026-11-01.md`).
  Open from it: RX IQ APPLY ALL (0x8D) writes only the PC files, never the radio (radio values win at
  the next start): decide later. RX / TX IQ lamps from the 0x2D masks: later. Avalonia client: same
  changes when the Avalonia catch-up happens. Opcodes free in all trees: 0x2E, 0x2F (2 TONE next).
- **Agreed with Ron 11-01** (`.mscc-coord/NOTE-FOR-STEW-ALC-089-TWO-TONE-2026-11-01.md`):
  - QRP save step: slider live only; "Save this calibration?" Yes = 0xA2 | 0x8000, No = previous
    value; Reject removed. New servers need the new client (no fallback; release note).
  - Status masks ms-sdr -> client on **0x2D**: kind (1 RX IQ, 2 TX IQ, 3 power), flags (bit 0 =
    store present), uint16 band mask. At connect, after each radio write, after a clear.
  - Resets = clear, never factory into the radio. Type 6 clear (control READ, data stage):
    wValueHi 6, wValueLo band or 0xFF, **wIndex kind** (1 RX IQ, 2 TX IQ, 3 power, 0 all).
    Per-kind resets incl. RX IQ; SETTINGS Reset configuration = client settings only.
    Firmware: PSoC lines + Geminus + Black Pill need the handler.
  - Split: Ron = Pi ms-sdr + trans; Stew's side = Windows ms-sdr, trans, client (WPF + Avalonia).
  - **2 TONE is ours on Windows AND Pi** (trans, ms-sdr, client). Any opcode free in both trees;
    name it in the push note.
  - ALC 0.89 on purpose: Ron's rig (ATU-100 meter), steady WSJT-X tone: 0.45 ~40 W, 0.80 89 W,
    0.89 95 W, TUN 109 W (PA near compression, so not the 124 % the square law suggests). No
    splatter check yet (no analyzer): settle 0.8 vs 0.89 with 2 TONE + Signal Hound (PEP, IMD).
- **2 TONE and ALC 0.89 done 11-01** (trans 3.157, client 11.1.3; see the section above). Pi
  trans needs the same `twoToneAudio()` change and 0x88 handling (note to Ron 11-01).
- FREQ CAL items **done in client 11.1.2** (11-01, tested): race fixed (EnterFreqCalTab awaits
  `_calTabRestoreTask` from Leave TX IQ / QRP CAL / AMP CAL), CW note on the tab, left-panel
  SPECTRUM ZOOM drives the FREQ CAL spectrum, set to 4× on entry and restored on leave.
- Ron 11-01 notes, queued until the firmware tests are done (no code yet): QRP CAL lamps from
  the radio's valid masks (`NOTE-FOR-RON-CAL-STATUS-LAMPS-2026-11-01.md`), ALC one ceiling
  (0.89 on the Pi), recv NR spectral subtraction + new noise blanker ports, NR Off/Low/Med/High,
  guide questions. Cal after reflash: **holds** (Stew, 11-01: calibrations survived a .cyacd
  reflash; the bootloader does not touch the EEPROM rows).
- Avalonia client: same LSB filter-index bug likely.
- Source tags for `power_cal.ini`, `iq.ini`, `recv-iq.ini`; reset to factory (needs the 0xC8
  clear back: why was it removed in 3.239?).
- recv: no speaker found -> use the first non-Multus output instead of stopping.
- MKII PTT 3.243: RIT retune on `TX_Idle()`, QSK release mute (Ron's answers 10-29).
- **Firmware tests done on all radios (Stew, 11-01)**: Legacy 1.236, Geminus MKII 2.153, MKII PTT
  3.242, MKII ATU 4.243, Ultimus Legacy 6.236 / MKII PTT 8.242 / MKII ATU 7.243. Calibration in the
  radio survives a reflash. Code work on the open list can start. Still to do: retire the old
  `Ultimus-*` trees (Stew's OK); rebuild the Linux firmware package (mscc-firmware, still 1.0.2
  with older images) with the current image for **every** radio: Linux supports all radios, not
  just Geminus.
- recv/trans side of the 0x30 server messages (needs the client to show them).
- Pi/Ubuntu: audio LPF, ALC ceiling/meter, recv stream-stop check, phase in client.
