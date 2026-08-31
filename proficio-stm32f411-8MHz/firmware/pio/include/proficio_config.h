/**
 * Proficio-on-STM32F411 configuration.
 */
#ifndef PROFICIO_CONFIG_H
#define PROFICIO_CONFIG_H

#define PROFICIO_FW_NAME        "proficio-stm32f411-8MHz"
/* Firmware version reported to host (CMD_GET_VERSION).
 * Major 5 = STM32 port; minor bumps as features land. */
#define FIRMWARE_VERSION_MAJOR  5
#define FIRMWARE_VERSION_MINOR  1
#define FIRMWARE_VERSION ((((FIRMWARE_VERSION_MINOR) << 8) & 0xff00) | \
                          ((FIRMWARE_VERSION_MAJOR) & 0x00ff))

/* Set each PIO build by extra_tinyusb.py → include/build_info.h */
#include "build_info.h"

#define PROFICIO_MCU_STM32F411  1

/* Feature gates */
#define PROFICIO_FEAT_USB_VENDOR  1
#define PROFICIO_FEAT_SI5351      1
#define PROFICIO_FEAT_KEYER_I2C   1
#define PROFICIO_FEAT_CW          1
#define PROFICIO_FEAT_BAND        1
#define PROFICIO_FEAT_USB_AUDIO   1
#define PROFICIO_FEAT_PCM3060     1

/* USB audio via TinyUSB UAC2 (see tusb_config.h / usb_audio_uac2.c) */

/* CW keying style: 1 = MKII PIN-diode (key RX per element), 0 = legacy DIN */
#ifndef PROFICIO_CW_MKII
#define PROFICIO_CW_MKII          1
#endif

/* SI5351 I2C 7-bit address */
#define SI5351_I2C_ADDR           0x60
/* PIC keyer I2C 7-bit address (PSoC used 0x40 as 8-bit write addr → 0x20) */
#define KEYER_I2C_ADDR            0x20

#define KEYER_MEM_Q_SIZE          80
#define CW_DEFAULT_HOLD_TIME      50  /* units × 10 ms ≈ hold (matches PSoC) */

#define TRUE  1
#define FALSE 0

#endif /* PROFICIO_CONFIG_H */
