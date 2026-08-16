#ifndef DIE_TEMP_H
#define DIE_TEMP_H

#include <stdint.h>

void    die_temp_init(void);
/** Read STM32 internal sensor; updates E_transceiver_temp (°C, integer). */
void    die_temp_poll(void);

#endif
