#ifndef I2C_BUS_H
#define I2C_BUS_H

#include <stdint.h>
#include "stm32f4xx_hal.h"

extern I2C_HandleTypeDef hi2c1;

void    i2c_bus_init(void);
/** Write one byte to 7-bit addr. Returns 1 on ACK success. */
uint8_t i2c_write_byte(uint8_t addr7, uint8_t data);
/** Write reg+val to device (SI5351 style). Returns 1 on success. */
uint8_t i2c_write_reg(uint8_t addr7, uint8_t reg, uint8_t val);
/** Write buffer. Returns 1 on success. */
uint8_t i2c_write_buf(uint8_t addr7, const uint8_t *buf, uint16_t len);

#endif /* I2C_BUS_H */
