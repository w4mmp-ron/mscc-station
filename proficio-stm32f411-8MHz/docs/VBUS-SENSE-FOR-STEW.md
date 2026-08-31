# USBV+ / VBUS sense — note for Stew (STM32 daughter)

**Date:** 2026-08-31  
**Product direction:** PTT daughter (ATU not required).  
**MCU module:** WeAct Black Pill STM32F411.

This note covers **only** host USB VBUS sensing. It does **not** change J5 power, I2S, or other pin tables in existing docs.

---

## Mother board (existing MKII USB section)

From the mother-board USB schematic:

| Connector net | After ferrites / ESD | J5 | Role |
|---------------|----------------------|-----|------|
| **USB_VBUS** | → **USBV+** | **J5 pin 8** | Host VBUS to daughter (“To PSoC” on old drawing) |
| **USB_D+** | → **USB+** | J5 (USB data) | To MCU D+ |
| **USB_D−** | → **USB−** | J5 (USB data) | To MCU D− |

**USBV+ is not the radio 5 V rail.** It is host USB VBUS brought to the daughter for the MCU.

---

## PSoC daughter (reference)

On the PSoC daughter, **USBV+** was attached to **PSoC U2 pin B8** as a **sense** input (host present / gone), not as the MCU’s main 5 V supply.

---

## STM32 / Black Pill daughter — what to implement

### Power (unchanged intent)

- Black Pill **5V** pin is fed by the **on-board 5 V regulator** (daughter power), **not** by USBV+.
- Keep **USBV+** separate from that regulator output.

### VBUS sense (add this — same role as PSoC B8)

1. Take **USBV+** from **J5 pin 8** (mother USB VBUS after ferrite).
2. Route to MCU sense pin **PA9** (STM32F411 OTG FS VBUS pin on the Black Pill — preferred).
3. **Level shift:** USBV+ is **5 V** when a PC is attached. STM32F411 GPIO is **3.3 V only**.  
   Use a **resistor divider** (e.g. aim ~3.0–3.3 V at PA9 when USBV+ = 5 V), optional RC filter, and/or clamp to 3.3 V.  
   **Do not** tie raw 5 V USBV+ straight into PA9.
4. If PA9 routing is awkward, any free GPIO may be used for **GPIO-only** sense; PA9 is preferred for consistency with STM32 USB docs.

### Data (unchanged)

- **USB+ / USB−** → Black Pill **PA12 / PA11** (USB FS PHY), as already planned.

---

## Why this matters (firmware)

With the pill powered from the board regulator, a **PC reboot** can leave the MCU running while the host USB stack dies and returns. Sensing **USBV+** (falling / rising) lets firmware drop and re-init USB without unplugging — same idea as the old PSoC VBUS-to-B8 wiring.

Without sense, hot-plug after the PC is up still works; **auto re-enumerate across reboot** is unreliable.

---

## Stew checklist

- [ ] **USBV+** (J5-8) ≠ short to regulator **5V** / pill 5V power net  
- [ ] **USBV+** → divider → **PA9** (or documented free GPIO)  
- [ ] **USB+ / USB−** → **PA12 / PA11**  
- [ ] ESD / ferrites remain on mother board as today  

**One-liner:** *USBV+ = host VBUS sense (as PSoC U2 B8); divide to 3.3 V → PA9; pill 5V stays on the board regulator.*
