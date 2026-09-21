# Status — windows-new-hp

| | |
|--|--|
| **Host** | NEW-HP-LAPTOP (Windows) |
| **Checkout** | `C:\Users\n8vet\OneDrive\Documents\GitHub\mscc-station` |
| **Build** | **cmd-019** |
| **Last command id** | cmd-019 |
| **State** | done |
| **Updated** | 2026-09-21 |

## ACK log

| command id | state | note |
|------------|-------|------|
| cmd-019 | done | FREQ Reset + Settings Reset + TX IQ 0x8D use factory by FW major; WPF 9.21.0; ms-sdr 172 |
| cmd-018 | done | factory trees + first-boot seed |

## Notes

A) CMD_SET_CAL_RESET: delete live freq_cal; Factory_reseed freq/<line>/; Init_PPM; push PSoC. PCB Create_PPM_ini only if factory missing (logged).

B) Settings Reset + first-run seed skip iq.ini, recv-iq.ini, freq_cal.ini, power_cal.ini. Extra delete leftovers. Dialog text updated. Next ms-sdr start Factory_seed fills by connected major.

C) TX IQ Reset All: Factory_reseed iq.ini then trans 0x8D **reloads live file** (no longer delete+built-in). RX 0x8D unchanged.

Ordinary restart still keeps tuned live files. Deploy: C:\mscc-net9 (ms-sdr, trans, WPF, factory/).
