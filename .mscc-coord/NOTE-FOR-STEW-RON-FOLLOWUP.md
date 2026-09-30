# For Stew — Ron follow-up coordination (2026-09-29 ET)

Paste-ready summary. Files are on NEW-HP at
`C:\Users\n8vet\OneDrive\Documents\GitHub\mscc-station`. **Not pushed.**

## Written

| Path | What |
|------|------|
| `.mscc-coord/briefs/cmd-055.md` | REWRITTEN: drop ms-sdr per-model cal swap; manual switch-radio tool (Ron Q4 final) |
| `.mscc-coord/COMMANDS.yaml` | cmd-055 entry added |
| `.mscc-coord/MANUAL-UPDATES-FROM-RON.md` | Paste-ready Linux + Windows manual replacements (Ron Q1–Q3) |
| `.mscc-coord/BACKLOG.md` | BL-001 remove CW PHONES (WPF+Avalonia); second-STOP push note |
| `docs/manuals/README.md` | Points at Ron answers + paste-ready updates |
| `AGENTS.md` | Current work: cmd-055 + backlog pointers |

## Manuals (your PDF / source refresh)

Use `.mscc-coord/MANUAL-UPDATES-FROM-RON.md`. Short version:

1. **Pi permissions:** no `usermod` for the installer user (postinst did it); keep
   "log out after first install"; other users still need `usermod` + logout.
2. **Upgrade / cal:** "settings and calibration kept on upgrade" is **correct**;
   clear the yellow; optional seed-only-if-empty note.
3. **CW POTENTIA / QSK:** Potentia full break-in timing (default off).
4. **CW PHONES:** say **not used** (and/or drop from docs once BL-001 removes UI).

## Code for you next

- **cmd-055** (NEW-HP): with or right after cmd-048 Windows — see brief.
- **BL-001** (later): remove CW PHONES checkbox WPF + Avalonia.
- **Second-STOP:** find/push the NEW-HP Windows `calibrate.c` change Ron does not
  see on GitHub, then he ports Pi. Details in `.mscc-coord/BACKLOG.md`.

## Do not

- Push from overseer.
- Touch `rpi/` from NEW-HP.
