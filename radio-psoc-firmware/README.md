# Radio PSoC firmware

PSoC Creator / Keil trees for Multus radio application firmware. Moved here so the repo root stays cleaner.

**Host:** build and flash work on **Shack** (only Keil license). Do not plan PSoC compile on NEW-HP, stew-HP, or Pi.

## Layout

| Path | Radio / role | Current `FIRMWARE_VERSION_MAJOR` (as of 2026-09-20) |
|------|----------------|------------------------------------------------------|
| `Proficio-Legacy/` | Proficio Legacy | **1** |
| `Proficio-MKII-PTT/` | Proficio MKII PTT | **3** |
| `Proficio-MKII-ATU/` | Proficio MKII ATU | **4** |
| `Proficio-bootloader/` | Shared Proficio Creator bootloader project | (LOADER; MiniProg3 only) |
| `Geminus-Legacy/` | Geminus Legacy | **224** (planned → **5**) |
| `Geminus-MKII/` | Geminus MKII | **2** |
| `Ultimus-Legacy/` | Ultimus Legacy — **clone of Proficio-Legacy**; still Proficio project names inside | still **1** until Build retargets → **6** |
| `Ultimus-MKII/PTT/` | Ultimus MKII PTT — clone of Proficio-MKII-PTT | still **3** until → **6** or product plan |
| `Ultimus-MKII/ATU/` | Ultimus MKII ATU — clone of Proficio-MKII-ATU | still **4** until → **7** |

Planned major map (client header / factory cal key): **1** Proficio Legacy · **2** Geminus MKII · **3/4** Proficio MKII PTT/ATU · **5** Geminus Legacy · **6** Ultimus Legacy · **7** Ultimus MKII (exact PTT vs ATU split for Ultimus majors — see `GROK-BUILD-FIRMWARE-MAJORS.md`).

Application `.cyacd` for USB HID upload lives under each tree’s `Release/` after Creator build + `copy-release`.

## Upload tools (not this folder)

| Host | CLI + GUI |
|------|-----------|
| Raspberry Pi | `rpi/psoc-usb-bootload-linux/` |
| Ubuntu x86_64 | `linux/psoc-usb-bootload-linux/` |

Field flash notes: each Proficio tree still has `STEW-FIRMWARE-UPDATE.md`.

## STM32 / other

`proficio-stm32f411-*`, Solidus, etc. stay at **repo root** — not PSoC Creator radio apps.

## Next for Grok Build

See **`GROK-BUILD-FIRMWARE-MAJORS.md`** in this folder. Do **not** change majors until Stew / Build Commander says go after this layout lands.
