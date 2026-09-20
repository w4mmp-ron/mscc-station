# Overseer intent

**Updated:** 2026-09-20  
**Overseer:** Build Commander

## Status

- **Active — cmd-009 (windows-new-hp):** Windows ms-sdr send **one packed 0xB2** FW word (like Linux); log **Multus radio found**; WPF WindowTitle correct FW + **ATU/PTT** suffix from major. Avalonia / Linux / Pi follow later.
- Firmware majors done on Shack (`radio-psoc-firmware/`, Geminus Legacy 5.120, Ultimus 6/7/8).
- cmd-008 (ubuntu-stew 0.6.56 amd64 kit) still pending when Stew schedules it.

## Coord workflow

Write orders on the **target host** first; push later to sync others. Call the user **Stew**.