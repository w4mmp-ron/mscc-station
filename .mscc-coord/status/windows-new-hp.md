# Status - windows-new-hp

| | |
|--|--|
| **Host** | NEW-HP-LAPTOP (Windows) |
| **Checkout** | `C:\Users\n8vet\OneDrive\Documents\GitHub\mscc-station` |
| **Build** | **cmd-024** |
| **Last command id** | cmd-024 |
| **State** | done |
| **Updated** | 2026-09-21 |

## ACK log

| command id | state | note |
|------------|-------|------|
| cmd-024 | done | idle VFO 0 / no band-mode; last-used gated; WPF 9.21.4 |
| cmd-023 | reviewed | 9.21.3; DIG-U stick blocked by idle 40m/USB default |
| cmd-022 | done | WindowTitle product line |

## Notes

Idle: VfoA/B FrequencyHz=0, Mode=None, CurrentBand="". No gold band, no mode button.
SaveLastUsed + RememberLastPersonalityFreq require IsRadioRunning and freq>0.
Poisoned 40M_* in INI not auto-wiped — re-set 40m DIG-U once after this build.
Stew pushes.