# For Stew — Windows servers vs Pi servers, what is still to port

From Ron, 2026-10-28. Ron moved the Black Pill radio to his Windows PC (Windows servers +
WPF) and it runs clean. While setting it up we compared the Windows server source with the
Pi server source. This is the list of Pi changes the Windows servers do not have yet.

## How it was checked

- Source compared, not just notes: `mscc-ui/windows-work-tree/{ms-sdr-MKII, SDRcore-recv,
  SDRcore-trans}` against `rpi/{ms-sdr-linux, SDRcore-recv-linux, SDRcore-trans-linux}`.
  Every Pi server commit since 2026-09-07 was checked against the Windows code, plus a diff
  of the opcode headers (`usbavrcmd.h`, both `commands.h`).
- Versions today: ms-sdr Windows 3.183 / Pi 3.179 (Pi 3.181 in the mscc 1.0.72 package),
  recv Windows 3.147 / Pi 3.145, trans Windows 3.147 / Pi 3.144. The numbers are separate
  counters, so a higher Windows number does not mean it has the Pi changes.
- The last Windows server commit is 2026-10-13 (cmd-074). Everything the Pi got after that
  is missing on Windows.

## Already done on Windows (the old Phase A list, `PHASE-A-WINDOWS-SERVER-RECONCILE.md`)

Checked in the code, nothing to do: keyer memory 0x9C, memory text speed 0x76 (`CW_Mem_Text_WPM`
is read from cw.ini), GUI egress / keep-alive / 4 MiB receive buffer, REMOTE_AUDIO 2 and the
remote mic / phones, and A5 (TUNE / CW make I/Q with no mic input: Windows feeds zeros,
same effect as the Pi).

## Still to port, most important first

### 1. PTT polling: failed USB reads taken as a PTT change (ms-sdr, Pi b5074a0)

Windows `main.c` `Manage_MKII_PTT_Switch` and `Get_Key_Status` read with
`r = usbControlMsgIN(...)` and act on `if (r)`. A USB error or timeout returns a negative
number, which also passes `if (r)`. On the Pi this turned junk reads into a PTT change; the
SET_PTT that followed timed out and Stop_all shut the servers. Pi fix:

- accept only a good 1-byte read (`r == 1`), log read failures once per run of failures;
- `Get_Key_Status` thread not started (it only logged CMD_GET_KEY_DOWN; nothing used it);
- PTT poll every 20 ms instead of every 1 ms.

On Ron's PC the Black Pill sees about 55 control requests a second from the Windows ms-sdr,
so the poll rate is already moderate there (Windows `Sleep` granularity). The `if (r)` part
still matters.

### 2. TX I/Q phase correction (sdrcore-trans, Pi dcdebbc; your note IQ-PHASE-ANSWERS-2026-10-13)

Pi trans corrects the transmit I/Q phase per band: `IQ_PHASE=` in `iq.ini` (read by name,
old files load as 0), opcodes `CMD_SET_IQ_PHASE` 0x2A / `CMD_GET_IQ_PHASE` 0x2B, 0x57 saves
also outside a TX IQ session. Windows trans has none of it and reads `IQ_OFFSET` only, so a
Pi `iq.ini` copied to Windows loses its phase values (the offsets still work). Better
opposite-sideband suppression on Windows needs this port.

### 3. Power calibration stored in the radio (ms-sdr, Pi 6680c11; note FIRMWARE-3.238-CALIBRATION)

Pi ms-sdr 3.179: radio opcode 0xC8 (`RADIO_CMD_CALIBRATION`, type 3 power, type 5 valid
masks) in `power_calibration.c`. 5 s after the power-cal slider stops (and out of TX) each
changed band is sent to the radio; at startup the radio's values win over `power_cal.ini`.
Older firmware: file as before. Windows ms-sdr has no 0xC8 code. Works with PSoC MKII 3.238+
and the Black Pill (both write each value as it arrives, no save command).

### 4. CW + TUN left the rig in TX (ms-sdr, Pi 3ad8ce0; note CW-TUNE-STUCK-TX-2026-10-14)

Pi ms-sdr, `CMD_SET_MAIN_MODE`: if TUNE is still on and the new mode is not TUNE, turn
TUNE and TX off (`Tune_button(FALSE, FALSE)`, CMD_SET_TX_ON FALSE to trans and recv, 20 ms)
before the mode change. Windows does not have it. Firmware since PSoC MKII 3.233 and the
Black Pill drop TX on entering CW themselves, so this mainly protects older PSoC firmware.

### 5. Client leaving frees the session at once (ms-sdr, Pi 840b648; note CLIENT-LEAVING-2026-10-14)

0xFE value 0 from the session owner releases the session immediately instead of the ~45 s
keep-alive timeout. Windows ms-sdr has `Session_Release()` but not this branch. Only useful
once WPF sends 0xFE 0 at Stop (your option).

### 6. recv / trans warnings to the client (Pi 7453e0d; note SERVER-MESSAGES-2026-10-19)

Pi recv and trans send their startup warnings (digital speaker not found, I/Q device not
found, mic failed) to ms-sdr on the keep-alive link; ms-sdr passes them to the client as
`[0x0B][0x30][text]`. Windows ms-sdr already sends its own 0x30 messages; Windows recv and
trans have no `CMD_SET_SERVER_ERROR` / keep-alive source ids. Needs the WPF 0x30 handler to
show anything (your option, same note).

### 7. Small ones

- **Firmware type in the log** (ms-sdr, Pi 68ac537): `G_firmware_type` from the version
  MINOR right after `srGetVersion` (200 and up = PSoC, 1-199 = STM32 Black Pill), logged as
  "Firmware type: STM32 (minor N)". Nothing uses it yet.
- **NUL byte in ms-sdr.log** (Pi a901ffd): Windows `main.c` line ~648 logs
  `G_previous_mode` with `%c`; it is 0 until the first mode change, so a NUL lands in the
  log and grep treats it as binary. Pi: `G_previous_mode ? G_previous_mode : '-'`.
- **Pi only, no port needed:** keep-alive / SWR lines logged once (SD card wear, 42413f3).

## Not a gap

- The Pi stops reading receive I/Q while transmitting; Windows keeps reading. The radio is
  fine either way (Black Pill counters clean on both).
- `REMOTE_RX_CTRL_ENABLE` / `_MONITOR` bit names exist only in the Pi header; Windows has
  `CMD_SET_REMOTE_RX_CTRL` 0x28 itself. Not checked further.

## Ron's settings copy

Ron copied the radio settings that are safe to share from the Pi to the Windows servers
(`%LOCALAPPDATA%\MSCC-NET9`): recv-iq, iq, power_cal, amplifier_cal, amplifier, power, cw.
Tool and list: `psoc-replacement-stm32/.../troubleshooting/windows-tools/copy-pi-ini.*`.
