# bandplan-us.csv — source and license note

- Source: ke9ns PowerSDR v2.8.0 (based on FlexRadio PowerSDR v2.7.2),
  `Console/database.cs`, the US-region BandText table: `AddBandTextTable()` data array
  (lines 2487–3164) plus `AddBandTextSWB()` (lines 189–410, SW broadcast, time stations,
  LW/MW, maritime). Copy on NEW-HP: `C:\Users\n8vet\Documents\ke9ns-PowerSDR`.
  Upstream: https://github.com/ke9ns/PowerSDR_KE9NS_v2.8.0
- PowerSDR is Copyright (C) 2003-2013 FlexRadio Systems, with ke9ns changes, and is licensed
  under the GNU General Public License, version 2 or later.
- Extracted 2026-11-27 by Build Commander: commented-out rows dropped; MHz converted to whole Hz
  (`round(MHz × 1e6)`); rows sorted by low_hz; only rows with high_hz ≤ 54 MHz kept
  (431 of 659 rows). Names unchanged.
- Columns: `low_hz,high_hz,name,tx`. `low_hz == high_hz` is a spot frequency (95 rows).
  `tx` is PowerSDR's TX-allowed flag (1/0); MSCC does not use it yet.
- License note: frequency ranges and band names are facts, but this table is a compilation taken
  from GPL code. Keep this note and the attribution with the file. If MSCC is ever released under
  terms that are not GPL-compatible, Stew and Ron should decide whether to keep this table or
  rebuild it from the ARRL band plan.
