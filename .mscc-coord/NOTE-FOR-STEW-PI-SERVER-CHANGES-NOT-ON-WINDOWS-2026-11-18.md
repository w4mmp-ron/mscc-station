# Note for Stew: Pi server changes the Windows servers don't have yet (2026-11-18)

Stew, while you work on the remote audio problem, Ron is changing only the Pi servers. We are not
touching the Windows servers or the WPF client, so nothing gets in your way. This note lists
what the Pi servers have that Windows doesn't, so the two can be brought back in step later.

Each release with Pi server changes will add to this list.

## R11-18-1: Pi ms-sdr 3.211

**What it does:** tells the operator when a Black Pill radio had to fix its receive by itself.
The client shows a pop-up:

    RECEIVE FIXED ITSELF
    The radio's receive was upset 3 times (921 frame errors) and the radio fixed it by itself.
    Most likely RF from the transmitter is getting into the radio.
    Check the SWR, the antenna and the station grounding.

**Why:** pill 3.22 fixes the receive when RF from the transmitter upsets it (see
`NOTE-FOR-STEW-DAUGHTER-BOARD-CLOCK-RESISTORS-2026-11-18.md`). The fix is so quick that the operator
never notices, so without this message nobody would know RF was getting into the radio.

**How it works:**
- Every 3 seconds, in the temperature loop (STM32 radios only), ms-sdr reads the pill's fix
  counter: radio command 0xC9 (CMD_AUDIO_STATS), wValue 0, wIndex 4, 32 bytes. That's the same
  read as `audio-stats.py`. Word 0 = layout (1 | 8 << 16), word 2 = fix count, word 4 = frame errors.
- When the count goes up, it waits until the radio has been receiving for 10 seconds, then sends
  one message with `Gui_Add_Message()`. It sends at most one every 10 minutes, and fixes in
  between are added to the next message.
- A pill older than 3.18 answers with a different layout word: one log line, and no messages.
  PSoC radios are skipped (STM32 only).
- Each fix also goes into the ms-sdr log as an `Rx_resync.` line.

**File:** `rpi/ms-sdr-linux/source/proficio-temperature.c`, the new function `Rx_resync_tick()`
just above `Check_temperature()`, and one call in the STM32 branch of the loop.

**Windows port:** the same function in the Windows ms-sdr `proficio-temperature.c`, using its own
USB read call. Both clients already show server messages, so neither needs a change.

## R11-18-3: Pi trans 3.161

**What it does:** the QRP and QRO calibration files are written once per calibration, not again
and again.

- **Save delay:** `power_cal.ini` and `amplifier_cal.ini` are saved once the slider has been still
  for 5 seconds. It was half a second, so every short pause while dragging the slider rewrote
  the file (6 writes for one 17 m calibration).
- **Saved on stop:** a value still waiting is saved when trans gets CMD_SET_STOP.
- **No rewrite when unchanged:** the client re-sends the power settings (TUNE / SSB / CW / AM / FM)
  at every connect and cal tab change, and each one rewrote `power.ini` and `amplifier_cal.ini`.
  Now the file is compared first and left alone when nothing changed (log: "Unchanged, not written").

**Files:** `rpi/SDRcore-trans-linux/sources/`
- `extern.h`: `POWER_CAL_SAVE_DELAY` 500 -> 5000 (Drive_Manager loops, about 1 ms each), version 3.161.
- `udp_thread.c`: `Flush_pending_cal_saves()` in `CMD_SET_STOP`.
- `power.c`: new `File_content_same()`, used by `Update_Proficio_User_Power_ini()`.
- `amplifier.c`: `Update_amplifier_calibration()` builds the text first and skips the write when it
  matches.

**Tested on Ron's rig:** a 75-step slider run gave one write, 5 seconds after the last step. Ending
on the same value gave no write. Clicking Main reloaded the tables with the new value.

**Windows port:** the same three changes in the Windows trans.

## R11-19-0: Pi ms-sdr 3.214, Pi trans 3.162, Black Pill 3.24

### CAT S-meter read (ms-sdr)

**What it does:** ms-sdr answers the Kenwood TS-2000 S-meter read `SM0;` / `SM1;` with `SM0nnnn;`
(0-30). Before, it didn't answer `SM` at all.

- Scale from Hamlib's TS-2000 table: S9 = 15, S9+60 dB = 30; below S9 0 = S9-54 dB, 3 = -48,
  6 = -36, 9 = -24, 12 = -12. Values between the points are interpolated. S9 = -73 dBm, as the
  client meter. Answers 0 while transmitting (`G_PTT`).
- Tested on Ron's rig: on WWV the CAT read gave 17-18, the client showed S9+10.

**Files:** `rpi/ms-sdr-linux/source/comm-port.c` (new `SM` block after `RM`), `usbavrcmd.h`
(`#define SM 14`).

### QRO (amplifier) calibration stored in the radio (ms-sdr, trans, pill)

**What it does:** the QRO cal per band is now kept in the radio too, as the QRP and I/Q cal already
are. **The radio is the master** (Ron):
- At startup, a band the radio holds uses the radio's value, and trans rewrites `amplifier_cal.ini`.
- A band the radio doesn't hold keeps the file's value, and that value is copied into the radio.
  A -99 (not calibrated) is never copied.
- QRO slider: the band's value goes to the radio once the slider has been still for 5 seconds
  and the rig is out of transmit (same as QRP / I/Q).
- Calibration reset of the amplifier also writes -99 to the radio, so its old values can't win at
  the next start.

**Black Pill only for now (Ron).** ms-sdr sends the new requests only to a pill at 3.24 or newer.
How it tells the radios apart: at startup ms-sdr reads the firmware version (`srGetVersion` in
`main.c`) and looks at the minor number:
- **PSoC:** minor 200 or more (your MKII PSoC is 3.240).
- **Pill (STM32):** minor 1 to 199 (the pill restarted at 3.1 on 2026-10-25).
- QRO check (`Radio_has_qro_store()` in `power_calibration.c`): `G_firmware_type == FIRMWARE_TYPE_STM32`
  and `G_minor_version >= 24` (`PILL_QRO_CAL_MINOR_MIN`).
- Older pills and every PSoC get no QRO request; they use `amplifier_cal.ini` only, as before.
  The log says so: "Radio is not a Black Pill 3.24+ ... amplifier_cal.ini only".
- When the PSoC gets QRO storage, the check needs a second line: PSoC and minor >= the new PSoC
  version.

**Radio side (pill 3.24, same `CMD_CALIBRATION` 0xC8, wValue = type << 8 | band, MSCC band order):**
- Type 8 = QRO value, int16, 2 bytes, little endian (read / write).
- Type 9 = QRO valid mask, uint16, 2 bytes, bit N = band N set (read only; a write stalls).
- Type 5 (valid masks) stays 6 bytes, so older ms-sdr builds still work with pill 3.24.
- For a PSoC version later: the same types and sizes, so ms-sdr talks to both radios the same way.

**Server side:**
- New opcode `CMD_SET_AMPLIFIER_CAL_RECORD` 0xF8, ms-sdr -> trans only (0xF8 was the unused,
  reclaimed `CMD_SET_AMPLIFIER`): int32 = record (bits 0-7, 0 = 160 m ... 9 = 10 m, 10 = 630 m,
  11 = 2200 m) | int16 value << 8. Trans sets that band and saves `amplifier_cal.ini`. Before,
  trans could only take the QRO value of the band in use.

**Tested on Ron's rig:** 12 file values copied into the empty radio; a restart read them back;
a 20 m recalibration (-37) went to the radio once; with the old file put back (-26) the radio's
-37 won and trans rewrote the file; after a rig power cycle all 12 came back from the pill's flash.

**Files:**
- `rpi/ms-sdr-linux/source/`: `power_calibration.c` (`Radio_has_qro_store`, `Radio_cal_store_qro`,
  `Radio_cal_send_qro`, `Radio_cal_load_qro`; calls in `Radio_cal_startup_load`,
  `Radio_cal_save_tick`, `Radio_cal_reset_all`), `amplifier.c` (`Amplifier_cal_value`,
  `Amplifier_cal_set_value`, store call in `CMD_SET_POTENTIA_CALIBRATION`), `extern.h`, `usbavrcmd.h`.
- `rpi/SDRcore-trans-linux/sources/`: `commands.h`, `udp_thread.c` (new case), `extern.h` (3.162).
- Pill: `firmware/pio-async/` `calibration.c/.h`, `usb_vendor.c`, `proficio_config.h` (3.24).

**Windows port:** the same ms-sdr and trans changes, using the Windows USB calls. No client change.

### SWR meter fault stops transmit in ms-sdr (backstop)

**What it does:** when the WiFi SWR meter reports a fault, ms-sdr itself drops transmit, whoever
keyed it (client, WSJT-X through CAT, footswitch, TUN, CAT tune), and refuses new transmits until
the fault is reset. Before, only the client dropped transmit, so a WSJT-X transmit on the Pi
(CAT, the client never started it) kept going.

- Released when the meter reports the fault clear (RESET on the meter or in the client), or when
  the meter goes offline (a dead meter can't be reset).
- Pop-up: "SWR METER FAULT ... Transmit stopped and blocked until the fault is reset."
- Log: "Swr_wifi. FAULT latched ... TX blocked", "Set_PTT . TX refused: SWR meter fault not reset",
  "Swr_wifi. fault cleared. TX allowed again".
- The meter's tune allowance is unchanged.

**How it works:** new flag `G_swr_fault_tx_block` (`swr_wifi_meter.c`, set on the fault edge, cleared
on the clear edge or offline). `Set_PTT` and `Tune_button` (`main.c`) and the CAT tune start
(`comm-port.c`) refuse to turn transmit ON while it's set; turning transmit off always works. It is
not `G_TX_Inhibit`: that one belongs to the Solidus QRP/QRO switch, which clears it by itself.
The CAT and footswitch paths now tell the client (and, for the footswitch, recv / trans) the real
state `G_PTT` after `Set_PTT`, not the request, so a refused transmit shows as receive.

**Tested on Ron's rig:** WSJT-X keyed through the CAT port's PTT line on 15 m; meter fault at SWR 3.32
(86.6 W forward, 25 W reflected); ms-sdr dropped transmit 3 ms later. WSJT-X's next cycle was
refused. The client's RESET (0x2E through ms-sdr) cleared it, and TUN worked after.

**Files:** `rpi/ms-sdr-linux/source/swr_wifi_meter.c`, `main.c`, `comm-port.c`, `extern.h`.

**Windows port:** the Windows ms-sdr has the same `swr_wifi_meter.c` (since 2026-11-10), so the same
four changes apply there.

## R11-19-1: Pi ms-sdr 3.215, recv 3.157, trans 3.163

**What it does:** each server carries its version as text inside the program file, for example
`MSCC-VERSION sdrcore-recv 3.157`, and writes the same line to its log at start. `mscc status`
reads it from the file without running the server. Details and the `mscc status` part:
`NOTE-FOR-STEW-MSCC-STATUS-AND-RELEASES-REPLY-2026-11-19.md`.

**Files:** `main.c` in `rpi/ms-sdr-linux/source`, `rpi/SDRcore-recv-linux/sources` and
`rpi/SDRcore-trans-linux/sources` (the tag above `main()`, one log line after the start-up line);
recv and trans `extern.h` (version).

**Windows port:** the same few lines in each Windows server's `main.c`, when the hold ends.

## R11-25-1: Pi ms-sdr 3.216, recv 3.158, trans 3.164 (stored CW carrier)

**What it does:** with a Black Pill 3.25 or later, the radio makes the CW carrier itself. trans
captures one 1 ms block of its CW carrier I/Q (96 frames, 384 bytes, the 12 kHz tone at 96 kHz),
ms-sdr loads it into the pill, and in CW the pill plays that block instead of the USB transmit
audio. The servers also stop the I/Q streams while they are not needed: trans stops its output
stream once the pill has the block (and when idle in voice / digital), recv stops receive during
voice, digital and TUNE transmit when Monitor is off.

**Switch:** `CW_CARRIER=1;` in the Pi's `~/.local/mscc/mscc.ini`, read by ms-sdr at start. Off by
default: without the line (or with `CW_CARRIER=0;`) the servers work exactly as before. A PSoC
radio or a pill older than 3.25 logs "off" and is not touched. Pi install guide has the how-to.

**How it works:**
- Radio command `CMD_SET_CW_CARRIER 0x15` (pill side shipped in 3.25): OUT wValue 0-5 = the six
  64-byte pieces of the block, wValue 0xFF = clear; IN = status byte (bit 0 feature present,
  bit 1 block loaded).
- Server opcode 0x15 between ms-sdr, trans and recv: ms-sdr -> trans / recv value 1 = on (capture
  a block), 0 = off, 2 = the radio has the block; trans -> ms-sdr = `[0x15][384 bytes]`.
- trans takes a new block after any command from ms-sdr except keep-alive, TX on / off, 0x15 and the
  WiFi SWR meter power (band, power, mode and offset changes all count); ms-sdr checks the pill status every few seconds in CW and asks trans again if the block
  is gone. At stop, ms-sdr clears the pill's block.
- Same code as Ron's cw-carrier test repo (`w4mmp-ron/cw-carrier`, release cw-R11-20-1).

**Files:** new `rpi/ms-sdr-linux/source/cw_carrier.c`, `rpi/SDRcore-trans-linux/sources/cw_carrier.c/.h`,
`rpi/SDRcore-recv-linux/sources/cw_carrier_rx.c/.h`; small hooks in each server's `main.c`,
`udp_thread.c` (recv / trans), ms-sdr `main-controller.c` (0x15 case, `CW_CARRIER=` parse, stop),
`extern.h`, `commands.h`, the Makefiles, and `usbavrcmd.h` (0x15).

**Tested (Ron, proficio, Black Pill 3.28 / 3.29):** CW on 17 m and 20 m (power the same as TUNE),
band change in CW (new block each time), TUNE (uses the normal stream, back to the stored block
after), FT8 on 20 m (recv pauses at key-down, back about 1 s after key-up). An A/B with
`CW_CARRIER=0` gave the same receive, so nothing changed there.

**Windows / Ubuntu port:** not done. The Windows ms-sdr, recv and trans would need the same five
files and hooks with their own USB and socket calls; without the port they work as before
(the pill keeps using the USB transmit audio).

**Stats:** with `CW_CARRIER=1` recv stops reading receive during transmit, so the pill's audio
counters saw those seconds as drops. Black Pill 3.29 counts them apart (see the release notes);
`audio-stats.py` shows `tx 52.9 s (5x, in_drop_tx ..., gaps_tx ...)` on each line.

## FYI: Black Pill 3.23 uses your USB names

The Black Pill now uses the same USB names as your PSoC images: "Multus IQ", "Multus IQ Sound",
"Multus IQ Receive", "Multus IQ Transmit". The servers find the radio by "Multus", so the
Windows servers need no change for it.
