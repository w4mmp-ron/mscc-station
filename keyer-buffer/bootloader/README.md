# Keyer I2C bootloader

```
build.bat
```

Builds `build\keyer-boot.hex`. Lets the Pi update the keyer PIC through the pill or
PSoC, so the PICkit is only needed once, to put the bootloader on.

## Flash layout

| Words | What |
|---|---|
| 0x0000–0x03FF | bootloader (about 550 words used) |
| 0x0400–0x3FFF | keyer app, built with code offset 0x400 |
| EEPROM 0x00–0xFE | keyer settings and CQ memories (bootloader never touches them) |
| EEPROM 0xFF | 0x5C = stay in the bootloader |

The interrupt vector (0x0004) jumps to the app's interrupt at 0x0404.

## When it runs

At reset it starts the keyer app at once, unless:

- EEPROM 0xFF is 0x5C (the app got opcode 0x9F 0x5C, or an update was cut short), or
- flash 0x400 is blank (no app; the host writes that row last).

In the bootloader the PIC answers on I2C **0x5C** (the app is on 0x40). Key lines and
sidetone are held off, the same as the app's idle state.

## Commands

One I2C write each, at most 18 bytes (about 2 ms at 100 kHz). It is acted on at STOP.

| Bytes | What |
|---|---|
| `01 aH aL` | set word address |
| `05 off d0..d15` | put 16 bytes in the row buffer at byte offset `off` (0, 16, 32, 48) |
| `02 sum` | erase and write the 32-word row at the address from the row buffer (low byte first), read it back, address += 32. `sum` = 8-bit sum of the 64 buffer bytes. Row must be 32-aligned, 0x400–0x3FE0 |
| `03` | the next reads return flash bytes from the address (low, high, ...); carries on across reads |
| `04 A5` | leave: clear the EEPROM flag and reset |

Reading (not after `03`) returns: status, version (2), address high, address low.
Version 1 (in Ron's rig 2026-11-12 only) took the whole row in one 66-byte `02` write.

Status: 00 OK, 01 bad address, 02 bad sum, 03 bad length, 04 read-back differs,
05 unknown command.

A row write takes about 5 ms; the PIC holds SCL meanwhile. Wait about 10 ms before
reading the status.

## Update order (host)

1. Send `9F 5C` to the app at 0x40; wait for the PIC to answer on 0x5C.
2. Write row 0x400 as blank (all 0x3FFF), so a cut-short update stays in the bootloader.
3. Write rows 0x420 upwards, then row 0x400 with its real data. Each row: `01 aH aL`,
   four `05` pieces, `02 sum`, wait about 10 ms, read the status.
4. Read back and compare, then `04 A5`.

Skip the hex file's config words (0x8007–0x800A) and EEPROM data (0xF000 up): the
bootloader owns the config, and the EEPROM keeps the user's settings.
