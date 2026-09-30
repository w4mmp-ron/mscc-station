# Questions for Ron (from Stew / Build Commander), 2026-09-28

Ron (or Ron's Claude): please fill in the **Answer:** line under each item. Stew wrote:

Hi Ron, a few things I need your help with. Three are from the new Linux / Raspberry Pi manual, two are code items.

## From the Linux / Pi manual

1. **Pi user permissions.** On Ubuntu the manual has people run `sudo usermod -aG dialout,audio,plugdev "$USER"` and then log out and back in. Should Pi users run the same command, or does the Pi servers package already do that? For now the manual keeps the step and flags it.

   **Answer:** No manual step needed on the Pi. The `mscc` package's postinst adds the installing user (the one who ran `sudo apt install`) to `dialout`, `plugdev` and `audio`. Keep only "log out and back in (or reboot) after the first install". Any *other* user who will run MSCC still needs the `usermod` line themselves.

2. **Upgrades and calibration files.** When someone installs a newer servers package, can it ever overwrite the calibration files in `~/.local/mscc/` (`iq.ini`, `recv-iq.ini`, `power_cal.ini`, `freq_cal.ini`)? The manual currently says settings and calibration are kept on upgrade.

   **Answer:** No. On install/upgrade the package seeds `~/.local/mscc/` only if the folder is missing or empty; an existing folder is left untouched (uninstall leaves it too). The servers only create a calibration file when it is missing, never overwrite one. The only things that replace them are the user's own reset actions in the client (FREQ CAL RESET → `freq_cal.ini`, I/Q defaults → `iq.ini`). So "settings and calibration are kept on upgrade" is correct. Note: a single deleted file comes back with factory values at the next server start; an emptied folder gets the whole package seed set on the next install/upgrade.

3. **CW tab: PHONES and POTENTIA / QSK.** What do these two controls do? The Windows guide left them open too, so one answer covers both manuals.

   **Answer:** **POTENTIA / QSK:** for Potentia 50/100 amplifiers (full break-in). On: the radio keys the AMP line and the PA together at key-down. Off (default): AMP line first, then PA, giving a relay-switched amp time to change over. Leave off unless a Potentia is attached. **PHONES:** not used. The client sends 0x70 but ms-sdr (Pi and Windows) ignores it; it does nothing. Manuals: say "not used".

## Code items

4. **cmd-048, Windows factory seeding.** After the power-cal port, trans writes `power_cal.ini`, so the Windows ms-sdr mirror (`Factory_mirror_live_to_cal("power_cal.ini")` in `factory_seed.c`, called from `Update_power_ini_file`) no longer runs on save. Which do you want?
   - (a) ms-sdr does the mirror on the next 0xA1 (band select) or on tab exit (which one?),
   - (b) trans does the mirror, or
   - (c) drop it.
   Details are in `.mscc-coord/briefs/cmd-048.md` ("Ask Ron before choosing: Windows factory seeding").

   **Answer:** (c) drop the mirror, **but** also change `load_iq_or_power` in `factory_seed.c`: when not switching radio lines and the live file exists, copy live → `cal\<line>\` and return (live wins). Use the `cal\<line>\` copy only when switching lines (or the live file is missing), then factory. Reason: today, at every ms-sdr start, the `cal\<line>\` copy overwrites live `power_cal.ini`; without the mirror that copy goes stale and a restart would undo the user's QRP cal. Same rule covers `iq.ini`. QRP Reset (0xAA): WPF never sends it; leave as on the Pi (ignored).

   **Answer (Ron, replaces the above):** QRP cal file management stays in sdrcore-trans; ms-sdr should never have managed `power_cal.ini`. trans creates it (factory values if missing), loads it and saves it, as on the Pi. On Windows, drop the automatic per-model handling in ms-sdr: remove the `Factory_seed_live_inis` startup swap, the `Factory_mirror_live_to_cal` calls and `LAST_LINE.txt`; ms-sdr only reads the file to report values to the client. Switching radio models is rare, so it's handled by a small utility script instead (Windows first): refuses to run while the servers run; reads the current model from `save\CURRENT.txt` (asks on first run); asks for the new model; copies the current cal files to `save\<current model>\`; copies `save\<new model>\` back, or the factory files if none saved; writes `CURRENT.txt`. Files: `power_cal.ini` and `iq.ini` (the two the old auto-swap handled). If a CLI script is not acceptable for Windows users, Build makes a small GUI for it instead (same steps: model pick list, save/restore, factory fallback).

5. **Pi FREQ CAL STOP fix.** Please add the second-STOP branch to `rpi/ms-sdr-linux/source/calibrate.c`. It's the same FREQ CAL STOP fix we did on Windows in `ms-sdr-MKII/source/calibrate.c` (in the repo: `mscc-ui/windows-work-tree/ms-sdr-MKII/source/calibrate.c`), around line 775, in the `CMD_SET_CAL_ABORT` handling.

   **Answer:** Pi `calibrate.c` already matches the pushed Windows `calibrate.c` (cmd-045a/b, `CMD_SET_CAL_ABORT` identical apart from the timer). There is no second-STOP branch in the repo yet — it's probably in a local commit on NEW-HP. Please push it (or say which commit) and I'll port it.

Thanks!
Stew

---

## Follow-ups filed (Build Commander, 2026-09-29 ET) — do not re-ask Ron

| Item | Filed as |
|------|----------|
| Q1–Q3 manuals | .mscc-coord/MANUAL-UPDATES-FROM-RON.md (Stew regenerates PDFs) |
| Q4 factory_seed | **cmd-055** — .mscc-coord/briefs/cmd-055.md |
| Q3 PHONES unused → remove UI | **BL-001** — .mscc-coord/BACKLOG.md |
| Q5 second-STOP | Stew must push NEW-HP Windows change first — .mscc-coord/BACKLOG.md |

Paste-ready summary for Stew: .mscc-coord/NOTE-FOR-STEW-RON-FOLLOWUP.md

