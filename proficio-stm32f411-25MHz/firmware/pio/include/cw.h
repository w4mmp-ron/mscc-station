#ifndef CW_H
#define CW_H

#include <stdint.h>

uint8_t keyer_write(uint8_t byte);
uint8_t Configure_CW(void);
void    Manage_Paddles_Port(void);
void    cw_init(void);
void    cw_on_hold_expired(void);  /* TIM / ISR → clear E_cw_hold */

#endif /* CW_H */
