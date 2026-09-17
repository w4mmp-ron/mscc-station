# Status — ubuntu-stew

| | |
|--|--|
| **Host** | stew-HP-Notebook (Ubuntu) |
| **Checkout** | `/home/stew/Documents/GitHub/mscc-station` |
| **Build** | cmd-003 |
| **Last command id** | cmd-003 |
| **State** | done |
| **Updated** | 2026-09-17 |

## ACK log

| command id | state | note |
|------------|-------|------|
| cmd-003 | done | Avalonia Phones\|Digital + Remote button like WPF. Commit `0efc752`. Build Release 0 errors. |

## Notes

- Left rail: **Phones | Digital** one row, **Remote** underneath (selected style). Checkbox gone.
- Commands: `SelectPhonesAudio`, `SelectDigitalAudio`, `ToggleRemoteAudio`.
- `0x9B` still 0/1 local, 2/3 with Remote; Remote AF window + CAT helpers unchanged.
- Remote **disabled** when Host is `127.0.0.1` / localhost (local CAT), same as WPF.
- Smoke: `dotnet build` Release succeeded. Run `./linux-build/run-avalonia.sh` to click Phones/Digital/Remote (Remote needs a non-loopback host).
- No UI `.deb` this pass. Norman: push/pull other hosts; pack amd64/arm64 if you want menu kits.

## Blocked

(none)
