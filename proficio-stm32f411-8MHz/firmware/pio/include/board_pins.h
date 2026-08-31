/**
 * STM32F411CEU6 Black Pill — Proficio MKII mother-board pin map.
 *
 * Mother-board nets = signals that leave the MCU daughter to the MKII radio
 * (same list the old PSoC used). Canonical doc: docs/STEW-DAUGHTER-BOARD-PINOUT.md
 */
#ifndef BOARD_PINS_H
#define BOARD_PINS_H

#include "stm32f4xx_hal.h"

/* ---------- On-module (not mother-board nets) ---------- */
#define BOARD_LED_GPIO          GPIOC
#define BOARD_LED_PIN           GPIO_PIN_13  /* PC13 active-low WeAct LED */
#define BOARD_USB_DM_GPIO       GPIOA
#define BOARD_USB_DM_PIN        GPIO_PIN_11
#define BOARD_USB_DP_GPIO       GPIOA
#define BOARD_USB_DP_PIN        GPIO_PIN_12

/* ---------- I2C1: SDA / SCL ---------- */
#define BOARD_I2C1_SCL_GPIO     GPIOB
#define BOARD_I2C1_SCL_PIN      GPIO_PIN_8   /* PB8 SCL */
#define BOARD_I2C1_SDA_GPIO     GPIOB
#define BOARD_I2C1_SDA_PIN      GPIO_PIN_9   /* PB9 SDA */

/* ---------- KEY_0 / KEY_1 (inputs) ---------- */
#define BOARD_KEY0_GPIO         GPIOB
#define BOARD_KEY0_PIN          GPIO_PIN_0   /* PB0 */
#define BOARD_KEY1_GPIO         GPIOB
#define BOARD_KEY1_PIN          GPIO_PIN_1   /* PB1 */

/*
 * PTT (INPUT — PSoC: pin → inverter → debouncer → Status).
 * Sense as active-low on the pin (common FOOTSWITCH); invert in Status_Read
 * to match PSoC hardware inverter polarity if board differs.
 */
#define BOARD_PTT_GPIO          GPIOA
#define BOARD_PTT_PIN           GPIO_PIN_6   /* PA6 */
#define BOARD_PTT_ACTIVE_LOW    1

/* ---------- BOOT (input → Status) ---------- */
#define BOARD_BOOT_GPIO         GPIOA
#define BOARD_BOOT_PIN          GPIO_PIN_8   /* PA8 — mother-board BOOT */

/* ---------- Band / control outputs ---------- */
#define BOARD_BS0_GPIO          GPIOA
#define BOARD_BS0_PIN           GPIO_PIN_7   /* PA7 (moved off PB12 for I2S) */
#define BOARD_BS1_GPIO          GPIOB
#define BOARD_BS1_PIN           GPIO_PIN_5   /* PB5 */
#define BOARD_BS2_GPIO          GPIOB
#define BOARD_BS2_PIN           GPIO_PIN_3   /* PB3 */
#define BOARD_AMP_GPIO          GPIOB
#define BOARD_AMP_PIN           GPIO_PIN_4   /* PB4 active-low */
#define BOARD_RX_GPIO           GPIOA
#define BOARD_RX_PIN            GPIO_PIN_1   /* PA1 active-low */
/* LED1 on PSoC Control bit0 — use module PC13 (no extra mother-board wire) */

/* ---------- Codec RESET (J5 A28 → mother-board RESET net) ---------- */
#define BOARD_CODEC_RESET_GPIO  GPIOA
#define BOARD_CODEC_RESET_PIN   GPIO_PIN_9   /* PA9 — PCM3060 RST (active-low); Stew rev 6.0 */
#define BOARD_CODEC_RESET_ACTIVE_LOW  1

/*
 * I2S / PCM3060 — REQUIRED for audio.
 * PSoC: one I2S Master; BCK1≡BCK2 and LRCK1≡LRCK2 (same sck/ws, two pins).
 * Daughter board: short BCK1–BCK2, LRCK1–LRCK2, SCK1–SCK2 to the single
 * STM32 driver pin each (or dual-drive later if layout needs split).
 *
 *   DOUT = from PCM3060 → MCU   (I2S2ext_SD)
 *   DIN  = MCU → PCM3060        (I2S2_SD)
 *   BCK  = bit clock            (I2S2_CK)  → BCK1 & BCK2 nets
 *   LRCK = word select          (I2S2_WS)  → LRCK1 & LRCK2 nets  (PB12 ≠ GND)
 *   SCK  = codec sysclk/MCLK    (I2S2_MCK) → SCK1 & SCK2 nets
 *
 * WeAct Black Pill does NOT break out PC6. I2S2_MCK is also available on
 * PA3 / PA6 (F411 AF). Production uses the Black Pill module → MCLK = PA3.
 */
#define BOARD_I2S_LRCK_GPIO     GPIOB
#define BOARD_I2S_LRCK_PIN      GPIO_PIN_12  /* PB12 I2S2_WS → LRCK1/LRCK2 */
#define BOARD_I2S_BCK_GPIO      GPIOB
#define BOARD_I2S_BCK_PIN       GPIO_PIN_13  /* PB13 I2S2_CK → BCK1/BCK2 */
#define BOARD_I2S_DIN_GPIO      GPIOB
#define BOARD_I2S_DIN_PIN       GPIO_PIN_15  /* PB15 I2S2_SD  → DIN (to codec) */
#define BOARD_I2S_DOUT_GPIO     GPIOB
#define BOARD_I2S_DOUT_PIN      GPIO_PIN_14  /* PB14 I2S2ext  ← DOUT (from codec) */
#define BOARD_I2S_MCLK_GPIO     GPIOA
#define BOARD_I2S_MCLK_PIN      GPIO_PIN_3   /* PA3 I2S2_MCK → SCK1/SCK2 (Black Pill) */

/*
 * PSoC Control bits CONTROL_DIN / CONTROL_DOUT gate audio paths in fabric
 * (AND into DIN on TopDesign). They are NOT the I2S DIN/DOUT mother-board pins.
 * Software shadow only until proven otherwise on the schematic.
 */

/* USBV+ sense: U2 B8 -> divider -> this GPIO (placeholder until Stew picks pin) */
#define BOARD_VBUS_SENSE_GPIO   GPIOB
#define BOARD_VBUS_SENSE_PIN    GPIO_PIN_10  /* optional; does not gate USB */

/* Debug UART USART1 (PA3 taken by I2S2_MCK for Black Pill production) */
#define BOARD_UART_TX_GPIO      GPIOA
#define BOARD_UART_TX_PIN       GPIO_PIN_9   /* conflict: PA9 = codec RESET on this board */
#define BOARD_UART_RX_GPIO      GPIOA
#define BOARD_UART_RX_PIN       GPIO_PIN_10  /* PA10 USART1_RX */

/* Aliases matching older names */
#define BOARD_BOOT_SENSE_GPIO   BOARD_BOOT_GPIO
#define BOARD_BOOT_SENSE_PIN    BOARD_BOOT_PIN

#endif /* BOARD_PINS_H */
