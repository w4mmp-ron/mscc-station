# Backlog (Build Commander) - from Ron answers 2026-09-29

## BL-001 - Remove CW-tab PHONES control (WPF + Avalonia)

**Priority:** WPF briefed as **cmd-057**; Avalonia later on stew-HP
**Hosts:** windows-new-hp (WPF) = **cmd-057**; ubuntu-stew (Avalonia) = later
**Stew directed:** remove unless another purpose is found - **none found**.

**Ron (QUESTIONS-FOR-RON.md item 3):** **PHONES** on the CW tab is **not used**.
Client sends `0x70`; ms-sdr (Pi and Windows) ignores it. Does nothing.

**Scope:**

- **WPF (cmd-057):** `MainWindow.xaml` CW tab - remove `PHONES` CheckBox bound to
  `CwPhones` (~line 905). Keep **POTENTIA / QSK**. Update CW tab tooltip that lists
  PHONES. Stop `SetCwPhonesAsync` / `0x70` from WPF. See `briefs/cmd-057.md`.
- **Avalonia (later):** `MainWindow.axaml` - remove CW `PHONES` CheckBox bound to
  `CwPhones` (~line 463). Keep **POTENTIA / QSK**. Wire-up: stop sending
  `SetCwPhonesAsync` / `0x70` on connect and on toggle. INI `CW_PHONES` may stay
  read-ignored or be dropped in a later cleanup (Stew's call).
- Manuals: once UI is gone, blank PHONES in screenshots / drop the bullet (not
  cmd-057 code work).

**Do not** remove operator **Phones** volume / AUDIO Phones path - those are
unrelated.

---

## NOTE - Pi FREQ CAL second-STOP (for Stew / Ron)

**Linux already has it** (`linux/ms-sdr-linux/source/calibrate.c`, cmd-043 /
`519c682`): when `CMD_SET_CAL_ABORT` and nothing new is owed, `else if
(cal_abort_pending)` logs `nothing new owed, drain still pending` and does **not**
call `Cal_Abort_Drain_Done`.

**Windows** still always calls `Cal_Abort_Drain_Done("nothing owed")` with no
else-if - briefed as **cmd-056** (`briefs/cmd-056.md`). **Pi** (`rpi/.../calibrate.c`)
still lacks the branch; Ron ports **after** Stew ships+pushes Windows.

**Action:**

1. Build cmd-056 on NEW-HP; commit locally; Stew **pushes**.
2. Tell Ron the Windows commit hash.
3. Only then Ron ports `rpi/ms-sdr-linux/source/calibrate.c`. Do **not** open a Pi
   brief from NEW-HP / do not edit `rpi/` here.

---

## NOTE - Avalonia SPECTRUM / PAN RESOLUTION (cmd-058)

**Stew chose:** port WPF PAN RESOLUTION (800/1600/3200) to Avalonia UI on ubuntu-stew.
WPF already has `PanResolutionList` + INI `PAN_RESOLUTION` + Core `SetPanResolutionAsync`
(`0x5F` index 0/1/2). Avalonia Connect hardcodes 800 and S/W has no control — briefed as
**cmd-058** (`briefs/cmd-058.md`). Visible Avalonia label: **SPECTRUM RESOLUTION** (aligns
with cmd-054 rename intent). Out of scope: WPF rename cmd-054, Pi, bling shell, Core edits.

cmd-053 Ubuntu recv spur-notch (FFT-bin width) is separate; smoke optionally verifies spur
blanking at all three resolutions after both land.


## cmd-055 — ACTIVE (windows-new-hp) — Stew v1, Ron approved (2026-10-01)

**Status:** Implementation shipped locally (cmd-055). UI tip / dead-code follow-up is **cmd-059**. Ron Q6 approved detection-driven auto-restore; Stew locked v1 shape.

**Brief:** `.mscc-coord/briefs/cmd-055.md`

**Summary:** LIVE-only while running (trans owns `power_cal.ini`); WPF **Save settings** → parked `cal\<line>\`; on load/detect LAST_LINE + model (same / swap / brand-new); no primary manual switch-radio tool; no auto-park on exit for v1. Commit locally; do not push. One cmd at a time.

## cmd-059 — ACTIVE (windows-new-hp) — cmd-055 follow-up tips + dead helpers

**Status:** Active. Stew: Save settings tip too long; want brief visible on-tab text; drop leftover unused helpers.

**Brief:** `.mscc-coord/briefs/cmd-059.md`

**Summary:** Verify-uncalled then remove `Factory_mirror_live_to_cal` and `Create_power_cal_file`; visible `When done, press Save settings.` on QRP CAL / AMP CAL / TX IQ; shorten Save settings + tab ToolTips. No park/restore behavior change. Commit locally; do not push.
