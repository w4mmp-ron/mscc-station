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
