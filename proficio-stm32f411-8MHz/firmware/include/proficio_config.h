/**
 * Proficio-on-STM32F411 configuration (scaffold).
 */
#ifndef PROFICIO_CONFIG_H
#define PROFICIO_CONFIG_H

#define PROFICIO_FW_NAME     "proficio-stm32f411-8MHz"
#define PROFICIO_FW_VERSION  "0.1.0-control"

/* Target MCU */
#define PROFICIO_MCU_STM32F411 1

/* Active port lives in firmware/pio — see RESUME.md */
#define PROFICIO_FEAT_USB_VENDOR  1
#define PROFICIO_FEAT_SI5351      1
#define PROFICIO_FEAT_KEYER_I2C   1
#define PROFICIO_FEAT_USB_AUDIO   0

#endif /* PROFICIO_CONFIG_H */
