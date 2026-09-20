# Status — windows-new-hp

| | |
|--|--|
| **Host** | NEW-HP-LAPTOP (Windows) |
| **Checkout** | `C:\Users\n8vet\OneDrive\Documents\GitHub\mscc-station` |
| **Build** | **cmd-017** |
| **Last command id** | cmd-017 |
| **State** | done |
| **Updated** | 2026-09-20 |

## ACK log

| command id | state | note |
|------------|-------|------|
| cmd-017 | done | WPF 9.20.3 full HF/LF S/W banks + LAST_HF/LF |
| cmd-016 | done | S/W follows band; 2200M/630M last-used kept |
| cmd-015 | done | Band gate from FW major; button label deferred |

## Notes

A) Banks now include SPECTRUM_BASELINE_HF/LF, GRID_MAX/MIN_HF/LF, DB_OFFSET_HF/LF plus waterfall. Live keys remain the active mirror. Panadapter baseline/grid/dB save via SaveLiveWaterfallAndActiveBank.

B) HF ship: -44/-106/g65/z0, baseline 50, grid -20/-125 (unchanged). LF ship: **-40/-100/g40/z0**, baseline **80**, grid -20/-125 (grass via waterfall+baseline).

C) LAST_HF_FREQ default 14000000, LAST_LF_FREQ 475000 (+ MODE). Updated on in-range tune. FW major: illegal freq retunes LAST_* (LF on Proficio-family / HF on Geminus).

WPF **9.20.3** in `C:\mscc-net9`.
