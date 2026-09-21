# Status - windows-new-hp

| | |
|--|--|
| **Host** | NEW-HP-LAPTOP (Windows) |
| **Checkout** | `C:\Users\n8vet\OneDrive\Documents\GitHub\mscc-station` |
| **Build** | **cmd-021** |
| **Last command id** | cmd-021 |
| **State** | done |
| **Updated** | 2026-09-21 |

## ACK log

| command id | state | note |
|------------|-------|------|
| cmd-021 | done | last-used band/freq sanity; geminus IQ factory 1/4; WPF 9.21.1 |
| cmd-020 | done | cal/<line>/ IQ+QRP user cache; ms-sdr 173 at C:\mscc-net9 |
| cmd-019 | done | Reset paths → FW-major factory |
| cmd-018 | done | factory trees + first-boot seed |

## Notes

WPF LoadLastUsed ignores stored f when GetBandNameForFrequency(f) ≠ requested band (poisoned 2200M_FREQ=475000 → default 136 kHz). SaveLastUsed writes under freq's band when CurrentBand is stale.
Factory geminus-mkii + geminus-legacy LF IQ_OFFSET BAND10=1 BAND11=4; copied to C:\mscc-net9\factory. TX IQ Reset reseeds 1/4. Stew pushes.