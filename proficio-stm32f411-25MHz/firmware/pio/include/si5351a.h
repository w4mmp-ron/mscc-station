#ifndef SI5351A_H
#define SI5351A_H

#include <stdint.h>

#define SI_CLK0_CONTROL  16
#define SI_CLK1_CONTROL  17
#define SI_CLK2_CONTROL  18
#define SI_SYNTH_PLL_A   26
#define SI_SYNTH_PLL_B   34
#define SI_SYNTH_MS_0    42
#define SI_SYNTH_MS_1    50
#define SI_SYNTH_MS_2    58
#define SI_PLL_RESET     177

#define SI_R_DIV_1   0x00
#define SI_CLK_SRC_PLL_A 0x00
#define SI_CLK_SRC_PLL_B 0x10

#define SI5351_XTAL_FREQ 25000000UL

void    si5351a_init(void);
void    si5351aOutputOff(uint8_t clk);
/** Drive LO set-frequency state machine. Returns 0 when idle (done). */
uint8_t si5351aSetFrequency(uint32_t LO_freq);
/** Blocking convenience: poll until done or max steps. Returns 0 on success. */
uint8_t si5351aSetFrequency_wait(uint32_t LO_freq);

uint8_t si5351_write(uint8_t reg, uint8_t val);

#endif /* SI5351A_H */
