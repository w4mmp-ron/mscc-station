# Grok Build — PSoC firmware major version work

**Audience:** Grok Build on **Shack** (Keil / PSoC Creator).  
**Coordinator:** Build Commander.  
**Status:** Layout ready (flat MKII PTT/ATU + `release/` drops). **Do not start major bumps until Stew says go.**

## Goals (when released)

1. **Geminus Legacy:** `FIRMWARE_VERSION_MAJOR` **224 → 5** (keep minor unless told otherwise). File: `radio-psoc-firmware/Geminus-Legacy/Geminus.cydsn/basic-plus.h` (confirm with search; also any duplicate defines).
2. **Ultimus Legacy:** retarget seed from Proficio Legacy → product name Ultimus; set major **6**.
3. **Ultimus MKII:** retarget `Ultimus-MKII-ATU/` and `Ultimus-MKII-PTT/` seeds.
   - `Ultimus-MKII-ATU` → `FIRMWARE_VERSION_MAJOR` **7**
   - `Ultimus-MKII-PTT` → `FIRMWARE_VERSION_MAJOR` **8**
   (Distinct majors so the MSCC client top header shows different versions for ATU vs PTT boards.)
4. Rebuild on Shack; copy shipping **`.cyacd`** and **`.hex`** into `radio-psoc-firmware/release/<RadioName>/` (dated filenames preferred). Do not change TX path / IQ runtime behavior in this task.
5. Leave Proficio majors **1 / 3 / 4** and Geminus MKII **2** alone unless a separate order says otherwise.

## Constraints

- Work **only on Shack** under `C:\Users\n8vet\OneDrive\Documents\github\radio-psoc-firmware\`.
- Orders-on-target-host-first: edit here; push when Stew asks — do not require other hosts to pull before Build can work.
- After path moves, **re-open** `.cywrk` in PSoC Creator; fix any broken absolute paths; run existing `rebuild_keil.py` / `copy-release.bat` per tree.
- Ultimus seeds still contain **Proficio** project folder names — rename `.cydsn` / `.cywrk` carefully (Creator-aware), or document if rename is deferred and only the `#define` + USB product string change in v1.
- Shared factory cal: PTT and ATU of the same product line share IQ/freq/power factory tables; distinct majors are for client header / UI identity.
- Do **not** implement host `iq.ini` factory seeding in this firmware task (separate MSCC client/server polish).
- Bootloader `.hex` stays out of `release/` unless Stew asks (MiniProg3 / factory only).

## Suggested Build steps

1. Confirm layout matches `README.md` in this folder (flat `*-MKII-PTT` / `*-MKII-ATU`, plus `release/`).
2. Grep all `FIRMWARE_VERSION_MAJOR` under `radio-psoc-firmware/`.
3. Geminus Legacy: change 224 → 5; rebuild; copy `.cyacd` + `.hex` → `release/Geminus-Legacy/`.
4. Ultimus Legacy: set major 6; update any “Proficio” user-visible USB/product strings that should say Ultimus (match existing Proficio/Geminus style); rebuild; copy → `release/Ultimus-Legacy/`.
5. Ultimus MKII: set **ATU major 7**, **PTT major 8**; same string cleanup; rebuild both; copy → `release/Ultimus-MKII-ATU/` and `release/Ultimus-MKII-PTT/`.
6. Summarize: old→new majors, paths under `release/`, any Creator path fixes, anything left named Proficio inside Ultimus trees.

## Out of scope (separate todos)

- Windows `ms-sdr` packed `0xB2` double-send  
- Log string `Proficio found` → `Multus radio found`  
- Factory `iq.ini` / `freq_cal.ini` / `power_cal.ini` seeding by major  
- TX IQ phase adjust  

## Reference

- Plan: repo-root `MSCC-POLISH-AND-FACTORY-PLAN.md`  
- Numbers: `MSCC-FACTORY-CAL-NUMBERS.md`  
- Field flash: each Proficio tree `STEW-FIRMWARE-UPDATE.md`  
- Drop folder: `release/README.md`

## Related client work (separate from PSoC compile)

MSCC window title today (WPF + Avalonia):

`MSCC … — MSCC: <client>   Core: <ms-sdr>   FW: <major.minor>`

Stew (2026-09-20): add another title block for **ATU** vs **PTT** so the variant is obvious (majors alone are easy to miss).

Suggested title shape:

`…   FW: 7.232   ATU`   or   `…   FW: 8.232   PTT`

Derive from FW major (same map as factory cal):

| Major | Label |
|------:|-------|
| 3 | PTT (Proficio MKII PTT) |
| 4 | ATU (Proficio MKII ATU) |
| 7 | ATU (Ultimus MKII ATU) |
| 8 | PTT (Ultimus MKII PTT) |
| other | omit the block (or show `—`) |

Implement in both WPF and Avalonia `WindowTitle` (keep `FirmwareText` / `FirmwareVersion` as `major.minor` only). Not part of the PSoC Keil compile task unless Stew bundles it.
