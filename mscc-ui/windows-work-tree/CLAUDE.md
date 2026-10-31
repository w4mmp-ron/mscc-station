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

## Versions in the field (2026-10-30)

| Part | Version | Main changes |
|---|---|---|
| trans | 3.150 | ALC rework (\|I+jQ\| envelope, ceiling 0.45 x drive), voice ALC level meter, 6 kHz audio LPF before the SSB/AM mixer (LSB image fix), **TX I/Q phase** |
| recv | 3.150 | RX stream kept running at PTT (muted, not stopped: broadband splatter fix), Ron's AM lowpass + DC block and noise blanker fix (Pi c9850ca) |
| ms-sdr | 3.186 | PTT poll `r == 1`, power cal in the radio (0xC8), CW + TUN stuck TX, 0xFE 0 release, 0x30 messages forwarded, firmware type in log, PPM cal start point, TX I/Q phase pass-through |
| client | 10.31.1 | TX I/Q PHASE slider; LSB/USB/DIG-U/AM filter buttons send the index (Ron's LSB note 10-31); cuts pushed once at connect |
| FW MKII PTT | 3.242 | DAC soft mute on TUN/PTT release, mode-change drain |

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
- Geminus MKII firmware (2.151) has no 0xC8 yet: port tomorrow (2026-10-31).

## Earlier today (details in `.mscc-coord/NOTE-FOR-RON-SSB-TX-RESULTS-WINDOWS-3.149-2026-10-30.md`)

- Broadband SSB splatter = recv `Pa_StopStream` at PTT on the shared USB audio device.
- LSB image = no audio low-pass before the DSB mixer (21-24 kHz mic hiss -> opposite sideband).
- ALC ceiling 0.45 x drive; meter shows the peak level against the ceiling.

## Open / next

- Avalonia client: same LSB filter-index bug likely.

- Source tags for `power_cal.ini`, `iq.ini`, `recv-iq.ini`.
- Geminus MKII: calibration storage (0xC8) in firmware.
- MKII PTT 3.243: RIT retune on `TX_Idle()`, QSK release mute (Ron's answers 10-29).
- Legacy firmware from `Proficio-Legacy - Copy` (Ron: people waiting).
- recv/trans side of the 0x30 server messages (needs the client to show them).
- Pi/Ubuntu: audio LPF, ALC ceiling/meter, recv stream-stop check, phase in client.
