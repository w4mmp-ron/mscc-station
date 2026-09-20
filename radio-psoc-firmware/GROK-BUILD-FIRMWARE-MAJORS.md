# Grok Build — PSoC firmware major version work

**Audience:** Grok Build on **Shack** (Keil / PSoC Creator).  
**Coordinator:** Build Commander.  
**Status:** Layout-only completed. **Do not start this checklist until Stew says go.**

## Goals (when released)

1. **Geminus Legacy:** `FIRMWARE_VERSION_MAJOR` **224 → 5** (keep minor unless told otherwise). File: `radio-psoc-firmware/Geminus-Legacy/Geminus.cydsn/basic-plus.h` (confirm with search; also any duplicate defines).
2. **Ultimus Legacy:** retarget seed from Proficio Legacy → product name Ultimus; set major **6**.
3. **Ultimus MKII:** retarget PTT and ATU seeds; set majors per plan (**6** Legacy vs **7** MKII line — confirm with Stew before flash): historically Proficio used **3** PTT / **4** ATU; Ultimus may use one major per product line with PTT/ATU sharing cal, or distinct majors for UI. **Ask Build Commander / Stew if unclear.**
4. Rebuild `.cyacd` on Shack; do not change TX path / IQ runtime behavior in this task.
5. Leave Proficio majors **1 / 3 / 4** and Geminus MKII **2** alone unless a separate order says otherwise.

## Constraints

- Work **only on Shack** under `C:\Users\n8vet\OneDrive\Documents\github\radio-psoc-firmware\`.
- Orders-on-target-host-first: edit here; push when Stew asks — do not require other hosts to pull before Build can work.
- After path moves, **re-open** `.cywrk` in PSoC Creator; fix any broken absolute paths; run existing `rebuild_keil.py` / `copy-release.bat` per tree.
- Ultimus seeds still contain **Proficio** project folder names — rename `.cydsn` / `.cywrk` carefully (Creator-aware), or document if rename is deferred and only the `#define` + USB product string change in v1.
- Shared factory cal: PTT and ATU of the same product line share IQ/freq/power factory tables; distinct majors are for client header / UI identity.
- Do **not** implement host `iq.ini` factory seeding in this firmware task (separate MSCC client/server polish).

## Suggested Build steps

1. Confirm layout matches `README.md` in this folder.
2. Grep all `FIRMWARE_VERSION_MAJOR` under `radio-psoc-firmware/`.
3. Geminus Legacy: change 224 → 5; rebuild; note new `.cyacd` name/date under `Geminus-Legacy/` Release or Archive.
4. Ultimus Legacy: set major 6; update any “Proficio” user-visible USB/product strings that should say Ultimus (match existing Proficio/Geminus style); rebuild.
5. Ultimus MKII PTT + ATU: set agreed majors; same string cleanup; rebuild both.
6. Summarize: old→new majors, `.cyacd` paths, any Creator path fixes, anything left named Proficio inside Ultimus trees.

## Out of scope (separate todos)

- Windows `ms-sdr` packed `0xB2` double-send  
- Log string `Proficio found` → `Multus radio found`  
- Factory `iq.ini` / `freq_cal.ini` / `power_cal.ini` seeding by major  
- TX IQ phase adjust  

## Reference

- Plan: repo-root `MSCC-POLISH-AND-FACTORY-PLAN.md`  
- Numbers: `MSCC-FACTORY-CAL-NUMBERS.md`  
- Field flash: each Proficio tree `STEW-FIRMWARE-UPDATE.md`
