/**
 * STM32F411CEU6 Black Pill — Proficio MKII pin map (scaffold copy).
 * Active build uses firmware/pio/include/board_pins.h
 * Canonical: docs/STEW-DAUGHTER-BOARD-PINOUT.md
 */
#ifndef BOARD_PINS_H
#define BOARD_PINS_H

/* On-module */
#define BOARD_LED_PORT          GPIOC
#define BOARD_LED_PIN           13u
#define BOARD_USB_DM_PORT       GPIOA
#define BOARD_USB_DM_PIN        11u
#define BOARD_USB_DP_PORT       GPIOA
#define BOARD_USB_DP_PIN        12u

/* I2C1 */
#define BOARD_I2C1_SCL_PORT     GPIOB
#define BOARD_I2C1_SCL_PIN      8u
#define BOARD_I2C1_SDA_PORT     GPIOB
#define BOARD_I2C1_SDA_PIN      9u

/* KEY / PTT / BOOT inputs */
#define BOARD_KEY0_PORT         GPIOB
#define BOARD_KEY0_PIN          0u
#define BOARD_KEY1_PORT         GPIOB
#define BOARD_KEY1_PIN          1u
#define BOARD_PTT_PORT          GPIOA
#define BOARD_PTT_PIN           6u
#define BOARD_BOOT_SENSE_PORT   GPIOA
#define BOARD_BOOT_SENSE_PIN    8u

/* Band / control outputs */
#define BOARD_BS0_PORT          GPIOA
#define BOARD_BS0_PIN           7u
#define BOARD_BS1_PORT          GPIOB
#define BOARD_BS1_PIN           5u
#define BOARD_BS2_PORT          GPIOB
#define BOARD_BS2_PIN           3u
#define BOARD_AMP_PORT          GPIOB
#define BOARD_AMP_PIN           4u
#define BOARD_RX_PORT           GPIOA
#define BOARD_RX_PIN            1u

/* Codec RESET — J5 A28 → PA2 (PCM3060 RST, active-low) */
#define BOARD_CODEC_RESET_PORT  GPIOA
#define BOARD_CODEC_RESET_PIN   2u

/* I2S PCM3060 */
#define BOARD_I2S_LRCK_PIN      12u  /* PB12 */
#define BOARD_I2S_BCK_PIN       13u  /* PB13 */
#define BOARD_I2S_DOUT_PIN      14u  /* PB14 from codec */
#define BOARD_I2S_DIN_PIN       15u  /* PB15 to codec */
#define BOARD_I2S_MCLK_PIN      3u   /* PA3 I2S2_MCK — Black Pill (not PC6) */

#define BOARD_VBUS_SENSE_PORT   GPIOB
#define BOARD_VBUS_SENSE_PIN    10u  /* PB10 spare */
#define BOARD_UART_TX_PORT      GPIOA
#define BOARD_UART_TX_PIN       9u   /* PA9 USART1_TX */
#define BOARD_UART_RX_PORT      GPIOA
#define BOARD_UART_RX_PIN       10u  /* PA10 USART1_RX */

#endif /* BOARD_PINS_H */
