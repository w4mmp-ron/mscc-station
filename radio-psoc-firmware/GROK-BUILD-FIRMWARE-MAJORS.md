# PSoC firmware major versions

Each Multus radio line has its own `FIRMWARE_VERSION_MAJOR`, so the MSCC client header shows which radio is connected and the servers pick the right factory calibration line. Majors were applied and rebuilt on Shack (2026-09-20); shipping `.cyacd` / `.hex` are in `release/<RadioName>/` (also packaged as the optional `mscc-firmware` deb → `/usr/share/mscc/firmware/`).

| Major | Radio | Source tree | Shipped version | Factory cal line |
|------:|-------|-------------|----------------:|------------------|
| 1 | Proficio Legacy | `Proficio-Legacy/` | 1.231 | proficio-legacy |
| 2 | Geminus MKII | `Geminus-MKII/` | 2.151 | geminus-mkii |
| 3 | Proficio MKII PTT | `Proficio-MKII-PTT/` | 3.232 | proficio-mkii |
| 4 | Proficio MKII ATU | `Proficio-MKII-ATU/` | 4.151 | proficio-mkii |
| 5 | Geminus Legacy (was 224) | `Geminus-Legacy/` | 5.120 | geminus-legacy |
| 6 | Ultimus Legacy | `Ultimus-Legacy/` | 6.231 | ultimus-legacy |
| 7 | Ultimus MKII ATU | `Ultimus-MKII-ATU/` | 7.151 | ultimus-mkii |
| 8 | Ultimus MKII PTT | `Ultimus-MKII-PTT/` | 8.232 | ultimus-mkii |

Notes:
- PTT and ATU boards of the same line share factory IQ / freq / power tables; the distinct majors only identify the board.
- Ultimus trees are clones of the Proficio projects; their PSoC Creator project folders are still named `Proficio-*` (rename deferred).
- The bootloader (LOADER) `.hex` is not in `release/`; it is MiniProg3 / factory only.
- Build host: Shack only (Keil license). Field flash: `STEW-FIRMWARE-UPDATE.md` in each Proficio/Ultimus tree.
