# Backlog (Build Commander) — from Ron answers 2026-09-29

## BL-001 — Remove CW-tab PHONES control (WPF + Avalonia)

**Priority:** next UI polish after open cmds settle  
**Hosts:** windows-new-hp (WPF), ubuntu-stew (Avalonia)  
**Stew directed:** remove unless another purpose is found — **none found**.

**Ron (QUESTIONS-FOR-RON.md item 3):** **PHONES** on the CW tab is **not used**.
Client sends `0x70`; ms-sdr (Pi and Windows) ignores it. Does nothing.

**Scope (when briefed as a cmd):**

- WPF: `MainWindow.xaml` CW tab — remove `PHONES` CheckBox bound to `CwPhones`
  (~line 905). Keep **POTENTIA / QSK**. Update CW tab tooltip that lists PHONES.
- Avalonia: `MainWindow.axaml` — remove CW `PHONES` CheckBox bound to `CwPhones`
  (~line 463). Keep **POTENTIA / QSK**.
- Wire-up: stop sending `SetCwPhonesAsync` / `0x70` on connect and on toggle
  (`MainViewModel` / WPF equivalents). INI `CW_PHONES` may stay read-ignored or be
  dropped in a later cleanup (Stew's call).
- Manuals: once UI is gone, drop the PHONES bullet (or keep a one-line "removed /
  not used" only if needed for old screenshots).

**Do not** remove operator **Phones** volume / AUDIO Phones path — those are
unrelated.

**Not a cmd yet** — pick next free id when Stew wants it built.

---

## NOTE — Pi FREQ CAL second-STOP (for Stew, before Ron ports)

**Ron (QUESTIONS-FOR-RON.md item 5):** Pi `rpi/ms-sdr-linux/source/calibrate.c`
already matches the **pushed** Windows `calibrate.c` (cmd-045a/b,
`CMD_SET_CAL_ABORT`). There is **no second-STOP branch in the repo yet** — Ron
says it is probably only a **local commit on NEW-HP**. He will port after Stew
**pushes** it (or names the commit).

**Action for Stew (human / Build on NEW-HP):**

1. Locate the Windows second-STOP change in
   `mscc-ui/windows-work-tree/ms-sdr-MKII/source/calibrate.c` (local-only commit,
   stash, or uncommitted work). As of 2026-09-29 evening ET, `main` matches
   `origin/main` and the pushed file has abort/drain (045a/b) but Ron still does
   not see a distinct second-STOP branch.
2. Commit on NEW-HP if needed, then **push** (or tell Ron the commit hash).
3. Only then ask Ron to port to `rpi/ms-sdr-linux/source/calibrate.c`.

Overseer does **not** push. Do not open a Pi brief until Windows is on GitHub.
