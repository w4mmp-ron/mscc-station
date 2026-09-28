# Questions for Ron (from Stew / Build Commander), 2026-09-28

Ron (or Ron's Claude): please fill in the **Answer:** line under each item. Stew wrote:

Hi Ron, a few things I need your help with. Three are from the new Linux / Raspberry Pi manual, two are code items.

## From the Linux / Pi manual

1. **Pi user permissions.** On Ubuntu the manual has people run `sudo usermod -aG dialout,audio,plugdev "$USER"` and then log out and back in. Should Pi users run the same command, or does the Pi servers package already do that? For now the manual keeps the step and flags it.

   **Answer:**

2. **Upgrades and calibration files.** When someone installs a newer servers package, can it ever overwrite the calibration files in `~/.local/mscc/` (`iq.ini`, `recv-iq.ini`, `power_cal.ini`, `freq_cal.ini`)? The manual currently says settings and calibration are kept on upgrade.

   **Answer:**

3. **CW tab: PHONES and POTENTIA / QSK.** What do these two controls do? The Windows guide left them open too, so one answer covers both manuals.

   **Answer:**

## Code items

4. **cmd-048, Windows factory seeding.** After the power-cal port, trans writes `power_cal.ini`, so the Windows ms-sdr mirror (`Factory_mirror_live_to_cal("power_cal.ini")` in `factory_seed.c`, called from `Update_power_ini_file`) no longer runs on save. Which do you want?
   - (a) ms-sdr does the mirror on the next 0xA1 (band select) or on tab exit (which one?),
   - (b) trans does the mirror, or
   - (c) drop it.
   Details are in `.mscc-coord/briefs/cmd-048.md` ("Ask Ron before choosing: Windows factory seeding").

   **Answer:**

5. **Pi FREQ CAL STOP fix.** Please add the second-STOP branch to `rpi/ms-sdr-linux/source/calibrate.c`. It's the same FREQ CAL STOP fix we did on Windows in `ms-sdr-MKII/source/calibrate.c` (in the repo: `mscc-ui/windows-work-tree/ms-sdr-MKII/source/calibrate.c`), around line 775, in the `CMD_SET_CAL_ABORT` handling.

   **Answer:**

Thanks!
Stew
