# Overseer intent

**Updated:** 2026-09-20  
**Overseer:** Build Commander

## Status

- **Active — cmd-012 (rpi):** ms-sdr owner re-ack re-sends packed FW (`0xB2`) + Core (`0xB3`) so client reconnect keeps header radio identity.
- Follow-on: same fix on Ubuntu `linux/ms-sdr-linux`, then Windows ms-sdr. Client “ask if missing” in next UI block.
- **Done — cmd-011:** Avalonia 0.6.57 arm64 kit.

## Coord workflow

Write orders on the **target host** first; push later. Call the user **Stew**.
