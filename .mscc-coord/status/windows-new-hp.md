# Status — windows-new-hp

| | |
|--|--|
| **Host** | NEW-HP-LAPTOP (Windows) |
| **Checkout** | `C:\Users\n8vet\OneDrive\Documents\GitHub\mscc-station` |
| **Build** | **cmd-016** |
| **Last command id** | cmd-016 |
| **State** | done |
| **Updated** | 2026-09-20 |

## ACK log

| command id | state | note |
|------------|-------|------|
| cmd-016 | done | WPF 9.20.2 S/W follows band; last-used keeps 2200M/630M |
| cmd-015 | done | Band gate from FW major; button label deferred |
| cmd-014 | done | owner re-ack 0xB2+0xB3 |

## Notes

C) GetValidBandPrefixes always includes 2200M, 630M — SaveLastUsed no longer refuses/wipes LF keys.

A) CurrentBand → ApplyWaterfallBankForActiveBand: 2200/630 LF bank; 160–10 and GEN HF bank. RADIO_MODEL sticky tracks the bank. RadioModelButton label unchanged; ApplyRadioModelBandGating not called (FW major owns gray). Next band change re-follows S/W (manual button is override until then).

WPF **9.20.2** in `C:\mscc-net9`. Stew: Geminus 630↔2200 last-used in MSCC_LastUsed.ini survives HF save; S/W look tracks band; gray still from FW; RadioModelButton still flips S/W.
