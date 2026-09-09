# Proficio firmware update — instructions for Stew

**Audience:** Stew (daughter board / station bring-up)  
**Applies to:** MKII-PTT, MKII-ATU, and Legacy Proficio  
**Date:** 2026-08-22  

There are **two flash regions** on the PSoC. Do not mix them up.

| Region | File type | How often | Tool |
|--------|-----------|-----------|------|
| **Application** (radio firmware) | `.cyacd` (preferred) or app `.hex` | Normal updates | USB host + **BOOT jumper**, or MiniProg3 |
| **Bootloader** (tiny “LOADER” program) | Bootloader project `.hex` only | Rarely / factory | **MiniProg3 / PSoC Programmer only** |

Flashing a **`.cyacd` never replaces the bootloader.** Customers and field updates only change the **application**.

**Supported field path:** BOOT jumper → program `.cyacd` → remove jumper → power cycle.

---

## 1. What you need

| Item | Where |
|------|--------|
| App `.cyacd` | `Release-Proficio-*/Release/` (after Creator build + `copy-release`) |
| Linux / Pi upload | **`bootloader`** (CLI) or **`bootloader-gui`** (Windows-like GUI); MSCC menu **Firmware Upload** |
| Windows USB host | `bootloader.exe` + `Bootloader_Utils.dll` + `CyUSB.dll` (e.g. Migration `utilities\`) |
| On-chip LOADER sources | `bootloader/` — leave alone unless MiniProg3 factory work |

### USB IDs (`lsusb` on the Pi)

| Mode | VID:PID | What you see |
|------|---------|----------------|
| Running app | **`16c0:05dc`** | Product: **Proficio** |
| In bootloader | **`04b4:b71d`** | Cypress / **PSoC3 Bootloader**; Morse **LOADER** on LED |

---

## 2. App update (normal)

### A. Enter LOADER (BOOT jumper)

1. Power off.  
2. Install / short the **BOOT** jumper.  
3. Power on.  
4. LED: Morse **LOADER**.  
5. `lsusb` / Device Manager: **`04b4:b71d`**.

### B. Program the `.cyacd`

**Pi** (ms-sdr **stopped**):

- GUI: `bootloader-gui` (or menu **MSCC → Firmware Upload**) — Load File → Program  
- CLI: `bootloader /path/to/Proficio-MKII-PTT-YYYYMMDD.cyacd`

**Windows:** run `bootloader.exe`, select the `.cyacd`, program.

### C. Back to normal

Power off → **remove BOOT jumper** → power on → **`16c0:05dc` Proficio**.

---

## 3. On-chip bootloader

Not a normal Stew or customer step. The LOADER on the board stays as shipped.  
`.cyacd` uploads do **not** update it. Reflash only with MiniProg3 if directed.

---

## 4. Which `.cyacd`

| Radio | Tree |
|-------|------|
| MKII rear PTT | `Release-Proficio-MKII-PTT` |
| MKII ATU | `Release-Proficio-MKII-ATU` |
| Legacy | `Release-Proficio-Legacy` |

Use that tree’s dated `Release/*.cyacd`.

---

## 5. Checklist

- [ ] Updating **app** only (not on-chip LOADER)  
- [ ] BOOT jumper on → Morse LOADER / `04b4:b71d`  
- [ ] ms-sdr stopped  
- [ ] Correct `.cyacd` for PTT / ATU / Legacy  
- [ ] After program: jumper off → `16c0:05dc` Proficio  

Questions → Ron (W4MMP).
