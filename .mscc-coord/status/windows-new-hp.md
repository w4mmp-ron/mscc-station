# Status — windows-new-hp

| | |
|--|--|
| **Host** | NEW-HP-LAPTOP (Windows) |
| **Checkout** | `C:\Users\n8vet\OneDrive\Documents\GitHub\mscc-station` |
| **Build** | **cmd-020** |
| **Last command id** | cmd-020 |
| **State** | done |
| **Updated** | 2026-09-21 |

## ACK log

| command id | state | note |
|------------|-------|------|
| cmd-020 | done | cal/<line>/ IQ+QRP user cache; ms-sdr 173 at C:\mscc-net9 |
| cmd-019 | done | Reset paths → FW-major factory |
| cmd-018 | done | factory trees + first-boot seed |

## Notes

Per-line IQ/QRP cache under `%LocalAppData%\MSCC-NET9\cal\<line>\`. Factory stays `C:\mscc-net9\factory` (ship/Reset). No freq user cache.
On FW major: stash active → cal/<prev>/; load cal/<line>/ else factory → active+cal. First upgrade bootstraps live → cal (keeps tuned HF).
Write-through on IQ commit / QRP save. TX IQ Reset All and QRP Reset: factory → live + cal/<line>/ then trans reload.
Freq Reset unchanged (cmd-019). Settings Reset still wipes cal/ so next start seeds factory.
Verify: HF IQ/QRP survives a Geminus session and restores; TX IQ Reset restores factory. Logs: `cal stash`, `cal load`, `cal seed factory`, `cal bootstrap`, `cal write-through`.
Stew pushes.
