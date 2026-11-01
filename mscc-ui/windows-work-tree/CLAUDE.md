# Windows work tree: working notes (Stew's Claude)

Notes from Stew's Claude sessions on the Windows side, kept here so Ron's Claude sees them on
every pull. Newest first. Detailed write-ups for Ron go in `.mscc-coord/NOTE-FOR-RON-*.md`.

## Where things are

- Servers: `SDRcore-trans/`, `SDRcore-recv/`, `ms-sdr-MKII/` (VS 2022; trans/recv config
  **MFC | x86**, ms-sdr **MFC | Win32**). Output: `mscc-ui/Release/windows-wpf/`.
  Installed by copying exe + pdb to `C:\mscc-net9`.
- Client (WPF): `mscc-mscc/mscc-new/MSCC.sln` (Release | Any CPU). The build copies
  `MSCC.Wpf.*` / `MSCC.Core.*` to `C:\mscc-net9` itself (`CopyToMsccNet9` target).
  Version in `MSCC.Wpf/ClientVersion.txt` (bumped per build).
- Radio firmware: `radio-psoc-firmware/` (PSoC Creator 4.4, Keil; builds on Shack only).
- Machines: **Shack** = build PC (VS, PSoC Creator). **NEW-HP** = test PC with the radio
  (Proficio MKII PTT, FW 3.242), Signal Hound SA, good mic.

## Versions in the field (2026-10-31)

| Part | Version | Main changes |
|---|---|---|
| trans | 3.153 | ALC rework (\|I+jQ\| envelope), ceiling 0.45 x drive for voice, **1.0 x drive for digital (3.152)**, voice ALC level meter, 6 kHz audio LPF before the SSB/AM mixer (LSB image fix), TX I/Q phase, TX I/Q from the radio (0x2C), **digital (VAC) key-up gate 200 ms + 10 ms fade (3.153)** |
| recv | 3.151 | RX stream kept running at PTT (muted, not stopped), Ron's AM lowpass + DC block and noise blanker fix, **RX I/Q from the radio (0x2C)** |
| ms-sdr | 3.188 | PTT poll `r == 1`, power cal in the radio (0xC8), CW + TUN stuck TX, 0xFE 0 release, 0x30 messages forwarded, firmware type in log, PPM cal start point, TX I/Q phase, **TX/RX I/Q stored in and loaded from the radio**, **TX I/Q from CW mode fix** |
| client | 10.31.2 | TX I/Q PHASE slider; LSB/USB/DIG-U/AM filter buttons send the index (Ron's LSB note 10-31); cuts pushed once at connect |
| FW MKII PTT | 3.242 | DAC soft mute on TUN/PTT release, mode-change drain |
| FW Proficio Legacy | 1.236 | Ron's 1.235 (CW key delay + fades, 0xC8 cal store, SSB-after-CW fix) + TUN/PTT DAC fade and mode-change drain (= MKII 3.241/3.242). In `Proficio-Legacy/`; `Proficio-Legacy - Copy` kept as reference. Tested on a Legacy rig 10-31 |
| FW Proficio MKII ATU | 4.243 | = MKII PTT 3.242 app code (cal store, CW key delay + fades, TUN/PTT DAC mute, mode drain) + Elecraft T1 port kept (`t1.c`, `T1_Main()` in the beater, `STATUS_ATU_0`). Schematic: `Key_Delay_Timer` (FF 16-bit, CW_Clock, period 10, continuous, reset tied 0) + `Key_Delay_isr` added on the CW Management sheet. Built 10-31, not flashed yet; T1 untested (no T1 here) |
| FW Geminus MKII | 2.153 | **0xC8 calibration store (2.152)**, **DAC soft-mute shaping on TUN/PTT and CW (2.153)** |

## Ultimus firmware built from the Proficio trees (2026-10-31)

- Ultimus = hand-built (through-hole) Proficio: same code, major 6/8/7 and USB product
  "Ultimus". The `Ultimus-*` trees were frozen 9/20 copies (only basic-plus.h major and a
  hand-edited generated USBFS_descr.c differed).
- `radio-psoc-firmware/build-ultimus.py` (+ `.bat`): after a Creator build of the Proficio tree,
  copies the .cydsn to git-ignored `_ultimus-build/`, recompiles the app with `DF(ULTIMUS)`
  (`#ifdef ULTIMUS` in basic-plus.h picks the major), patches the USB product string, links and
  runs CyHexTool with Creator's options, writes `release/Ultimus-<radio>/...-YYYYMMDD.cyacd/.hex`.
  Trees without the switch are skipped.
- Done: Legacy (Ultimus 6.236 confirmed in MSCC on Stew's Ultimus Legacy), MKII ATU (Ultimus
  7.243 built 10-31, not flashed yet), MKII PTT (Ultimus 8.242 built 10-31, not flashed yet).
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

## Earlier today (details in `.mscc-coord/NOTE-FOR-RON-SSB-TX-RESULTS-WINDOWS-3.149-2026-10-30.md`)

- Broadband SSB splatter = recv `Pa_StopStream` at PTT on the shared USB audio device.
- LSB image = no audio low-pass before the DSB mixer (21-24 kHz mic hiss -> opposite sideband).
- ALC ceiling 0.45 x drive; meter shows the peak level against the ceiling.

## Open / next

- Avalonia client: same LSB filter-index bug likely.
- Source tags for `power_cal.ini`, `iq.ini`, `recv-iq.ini`; reset to factory (needs the 0xC8
  clear back: why was it removed in 3.239?).
- recv: no speaker found -> use the first non-Multus output instead of stopping.
- MKII PTT 3.243: RIT retune on `TX_Idle()`, QSK release mute (Ron's answers 10-29).
- Legacy 1.236, MKII ATU 4.243, Ultimus Legacy 6.236 / MKII PTT 8.242 / MKII ATU 7.243: done.
  Next: flash/test MKII ATU and Ultimus MKII PTT/ATU, retire the old `Ultimus-*` trees (Stew's OK).
- recv/trans side of the 0x30 server messages (needs the client to show them).
- Pi/Ubuntu: audio LPF, ALC ceiling/meter, recv stream-stop check, phase in client.
