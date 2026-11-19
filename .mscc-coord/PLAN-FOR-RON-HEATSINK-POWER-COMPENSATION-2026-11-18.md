# Plan for Ron: heat sink power compensation (written 2026-11-18, start when the thermistor arrives)

## The problem
On the Black Pill rig, output power drifts down as the radio warms up from cold. The heat sink
thermistor is there so the servers can make up for that. The thermistor is not fitted yet; the
part is on order from Digi-Key.

## What the Pi servers do today
- **The pill** (since 3.12) reports the heat sink temperature on radio command 0x06
  (CMD_GET_POTENTIA_TEMPERATURE) in whole degrees C. It sends -99 when the thermistor is open and
  999 when it is shorted.
- **ms-sdr** (`rpi/ms-sdr-linux/source/proficio-temperature.c`, `Check_temperature()`) reads it every
  3 s on STM32 radios. It sends the value to the client (`Gui_send_param`) and logs it every 30 s.
  It does **not** send it to trans.
- **trans** already has a receiver for it (`udp_thread.c`, `case CMD_GET_POTENTIA_TEMPERATURE`) that
  averages the temperature (`driver.c`, `Average_Temperature()`). The average is never used.
  - The receiver decodes the old Potentia packed format: `upper_byte * 16 + lower_byte / 16`.
    That is not the pill's plain degrees C.
- **The old drive tables** (cold / low / mid / high by temperature) are in `driver-old.c`, from the
  Potentia days. They are not built and don't fit the current power calibration.

**So there is no power compensation at all today.** Fitting the thermistor alone changes nothing
except the log line and the client display.

## Step 1: measure (no code changes)
Once the thermistor is in:
1. Check that the ms-sdr log shows `heat sink: NN C`, not OPEN, and that the value makes sense cold.
2. Start from a cold radio. On one band (15 m or 17 m), run a steady carrier (WSJT-X Tune or MSCC TUN)
   at a fixed setting into the dummy load or the antenna, at your usual QRP level.
3. Every 1 to 2 minutes write down the power (SWR meter and ATU-100) and the heat sink temperature
   (log or client). Go on until the temperature stops rising. Rest the radio for duty cycle as you
   normally would.
4. Repeat on one high band and one low band, and once at QRO, so we can see whether the drift
   depends on band or level.

What we want from it is **power lost per degree C** and whether it is a straight line.
Claude can pull the temperatures from the log, so you only need to note times and watts.

## Step 2: forward the temperature (small server change)
- **ms-sdr:** in `Check_temperature()`, send the heat sink value to trans as well
  (`SDRcore_trans_send_param(CMD_GET_POTENTIA_TEMPERATURE, heat_sink)`). Skip it when the reading is
  -99 or 999, and only send when the value changes.
- **trans:** on STM32, decode the value as plain degrees C, not the Potentia packed format. Keep the
  averaging, and treat a missing or faulted reading as "no compensation".

## Step 3: compensate in trans
- Where trans works out the drive (`driver.c`, `Driver_Get_QRP_Power()` / `Driver_Get_QRO_Power()`
  feeding `Drive_Manager`), multiply the drive by a temperature factor:
  `factor = 1 + k * (T - T_cal)`.
  - `T_cal` = the heat sink temperature when that band was calibrated. Save it with the
    calibration so the calibration stays right at the temperature it was done at.
  - `k` = drive change per degree, from Step 1. It could be one value, or per band if Step 1
    shows the bands differ.
- Clamp the factor (for example 0.9 to 1.2) so a bad reading can't drive the radio hard.
- Change it slowly (the average already does most of this) so the power doesn't step during a
  transmission.
- Log the factor when it changes, and add an on/off setting (default on once tested).

## Step 4: test
- Cold start, then the same run as Step 1. Power should stay within about 5% from cold to warm.
- Unplug the thermistor while the radio is running: compensation must stop cleanly (factor 1.0)
  and the log must say so.
- Check that QRP and QRO calibration still save and load the same way.

## Notes
- This is Pi-only for now (Windows hold). If it ships, add it to Stew's
  Pi-changes-not-on-Windows note at the "finished".
- PSoC radios don't answer 0x06, so all of this applies to the Black Pill only.
