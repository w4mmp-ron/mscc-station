# Flash-resident USB DFU bootloader (F411 Black Pill, 25 MHz HSE)

**File:** `bootloader.bin` / `bootloader.hex`  
**Script:** `./flash-bootloader.sh`

This is **not** ST OpenBL middleware (no official F411 OpenBL app exists).  
It is a **Cube USB-DFU IAP bootloader** built for STM32F411CEU6 @ **25 MHz HSE** (same crystal as our 25M pill).

| Item | Value |
|------|--------|
| Bootloader flash | `0x08000000` … `0x08007FFF` (32 KB) |
| **Application must start at** | **`0x08008000`** |
| Enter DFU | Hold **KEY (PA0)** + NRST/power — *not* BOOT0, *not* PA8 |
| DFU active | PC13 fast blink |
| Host | `dfu-util` / CubeProgrammer |

## Burn (you do this)

Flash **only** the bootloader first (ST-Link recommended):

```text
Address 0x08000000  ←  blackpill-f411-dfu-bootloader-25MHz.bin
```

## Critical

Proficio app is built for **`0x08008000`**, named **`proficio-stm32f411-25MHz-YYYY-MM-DD.bin`**.

See **`BURN-BOTH.md`** for the two-image flash order.
