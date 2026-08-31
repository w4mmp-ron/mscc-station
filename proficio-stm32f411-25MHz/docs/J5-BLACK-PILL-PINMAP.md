# J5 → Black Pill pin-by-pin (production)

**J5:** `PCIE-064-02-F-D-TH` (64-pin). Mother board edge connector.  
**MCU module:** WeAct **STM32F411CEU6 Black Pill** on the daughter.  
**USB jack:** on MKII mother board → J5 → pill (PA11/PA12).

Clock pairs on the daughter are **shorted** to one MCU pin each (same as old PSoC dual-drive nets).

---

## A-side (A01–A32)

| J5 pin | Net | Black Pill | Notes |
|--------|-----|------------|--------|
| A01 | — | — | NC / unused on this drawing |
| A02 | — | — | NC |
| A03 | — | — | NC |
| A04 | **KEY_1** | **PB1** | Key input |
| A05 | — | — | NC |
| A06 | **KEY_0** | **PB0** | Key input |
| A07 | — | — | NC |
| A08 | **SCK2** | **PA3** | Tie to SCK1 → I2S2_MCK |
| A09 | — | — | NC |
| A10 | **SCK1** | **PA3** | Tie to SCK2 → I2S2_MCK |
| A11 | — | — | NC |
| A12 | **BCK1** | **PB13** | Tie to BCK2 → I2S2_CK |
| A13 | — | — | NC |
| A14 | **BCK2** | **PB13** | Tie to BCK1 → I2S2_CK |
| A15 | — | — | NC |
| A16 | **LRCK2** | **PB12** | Tie to LRCK1 → I2S2_WS |
| A17 | — | — | NC |
| A18 | **DIN** | **PB15** | MCU → PCM3060 (I2S2_SD) |
| A19 | — | — | NC |
| A20 | **LRCK1** | **PB12** | Tie to LRCK2 → I2S2_WS |
| A21 | — | — | NC |
| A22 | **DOUT** | **PB14** | PCM3060 → MCU (I2S2ext_SD) |
| A23 | — | — | NC |
| A24 | **SDA** | **PB9** | I2C1 |
| A25 | — | — | NC |
| A26 | **SCL** | **PB8** | I2C1 |
| A27 | — | — | NC |
| A28 | **RESET** | **TBD** | Codec RST — pick free GPIO |
| A29 | — | — | NC |
| A30 | **BS2** | **PB3** | Band bit2 |
| A31 | — | — | NC |
| A32 | **AMP** | **PB4** | Active-low (FW) |

---

## B-side (B01–B32)

| J5 pin | Net | Black Pill | Notes |
|--------|-----|------------|--------|
| B01 | — | — | NC |
| B02 | — | — | NC |
| B03 | — | — | NC |
| B04 | **USB+** | **PA12** | Mother USB jack D+ |
| B05 | — | — | NC |
| B06 | **USB-** | **PA11** | Mother USB jack D− |
| B07 | — | — | NC |
| B08 | **USBV+** | **VBUS / 5V path** | Mother USB VBUS |
| B09 | — | — | NC |
| B10 | **ATU_1** | **TBD** | Not in FW yet |
| B11 | **GND** | **GND** | Ground |
| B12 | **ATU_0** | **TBD** | Not in FW yet |
| B13 | — | — | NC |
| B14 | **5V** | **5V** | Power (pill regulator / rail as designed) |
| B15 | — | — | NC |
| B16 | — | — | NC |
| B17 | — | — | NC |
| B18 | — | — | NC |
| B19 | — | — | NC |
| B20 | — | — | NC |
| B21 | — | — | NC |
| B22 | — | — | NC |
| B23 | — | — | NC |
| B24 | — | — | NC |
| B25 | — | — | NC |
| B26 | **BS0** | **PA7** | Band bit0 |
| B27 | — | — | NC |
| B28 | **BS1** | **PB5** | Band bit1 |
| B29 | — | — | NC |
| B30 | **LED** | **PC13** | Or buffer as Stew designs |
| B31 | — | — | NC |
| B32 | **RX** | **PA1** | Active-low (FW) |

---

## Daughter shorts (required)

| J5 nets | Single Black Pill pin |
|---------|------------------------|
| SCK1 + SCK2 | **PA3** |
| BCK1 + BCK2 | **PB13** |
| LRCK1 + LRCK2 | **PB12** |

**Do not use PC6** — not on WeAct Black Pill headers. **PA3 = I2S2_MCK.**

---

## Still open

| Item | Status |
|------|--------|
| **RESET** (A28) | Need free GPIO assignment |
| **PTT**, **BOOT** | Not on this J5 crop — confirm other connector/sheet |
| **ATU_0 / ATU_1** | On J5; FW later |
| Extra **GND / 3.3V** | Route all grounds / 3.3V per Stew power plan |

---

## Compact net → pin (same as before)

| J5 net(s) | Black Pill |
|-----------|------------|
| **DOUT** | **PB14** |
| **DIN** | **PB15** |
| **BCK1 + BCK2** (short on daughter) | **PB13** |
| **LRCK1 + LRCK2** (short) | **PB12** |
| **SCK1 + SCK2** (short) | **PA3** (I2S2_MCK) |
| **SCL** | **PB8** |
| **SDA** | **PB9** |
| **BS0** | **PA7** |
| **BS1** | **PB5** |
| **BS2** | **PB3** |
| **AMP** | **PB4** |
| **RX** | **PA1** |
| **KEY_0** | **PB0** |
| **KEY_1** | **PB1** |
| **LED** | **PC13** (or as Stew buffers) |
| **RESET** | TBD free GPIO |
| **USB+** (mother jack via J5) | **PA12** |
| **USB-** | **PA11** |
| **USBV+** | VBUS / 5 V path on pill |

**Notes:** No **PC6**. USB connector is on the **MKII mother board** → J5 → pill.
