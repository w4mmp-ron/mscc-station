/**
 * Proficio PA / board NTC on PA4 (ADC1_IN4).
 * USB: CMD_GET_POTENTIA_TEMPERATURE (0x06) — Solidus retired; opcode reused.
 *
 * Default circuit (tune to Stew's schematic):
 *   3.3V -- Rfixed(10k) -- ADC node -- NTC(10k@25C, B=3950) -- GND
 * NTC to ground; pull-up to 3V3. Any solid board GND is fine for NTC return.
 */
#include "pa_ntc.h"
#include "adc1_shared.h"
#include "board_pins.h"
#include "radio_state.h"
#include "stm32f4xx_hal.h"
#include <math.h>

#ifndef PA_NTC_R_FIXED_OHMS
#define PA_NTC_R_FIXED_OHMS 10000.0f
#endif
#ifndef PA_NTC_R25_OHMS
#define PA_NTC_R25_OHMS     10000.0f
#endif
#ifndef PA_NTC_BETA
#define PA_NTC_BETA         3950.0f
#endif
#ifndef PA_NTC_VREF_MV
#define PA_NTC_VREF_MV      3300.0f
#endif

void pa_ntc_init(void)
{
    GPIO_InitTypeDef g = {0};

    adc1_shared_init();

    __HAL_RCC_GPIOA_CLK_ENABLE();
    g.Pin = BOARD_PA_NTC_PIN;
    g.Mode = GPIO_MODE_ANALOG;
    g.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(BOARD_PA_NTC_GPIO, &g);
}

void pa_ntc_poll(void)
{
    uint32_t raw = adc1_shared_read(BOARD_PA_NTC_ADC_CHANNEL);
    float v_mv;
    float r_ntc;
    float t_k;
    float t_c;

    if (raw > 4095u) {
        return;
    }
    /* Avoid divide-by-zero / open / short extremes */
    if (raw < 8u || raw > 4087u) {
        return;
    }

    v_mv = (PA_NTC_VREF_MV * (float)raw) / 4095.0f;
    /* V = Vref * Rntc / (Rfixed + Rntc)  =>  Rntc = Rfixed * V / (Vref - V) */
    r_ntc = PA_NTC_R_FIXED_OHMS * v_mv / (PA_NTC_VREF_MV - v_mv);

    /* Beta: 1/T = 1/T25 + (1/B)*ln(R/R25) */
    t_k = 1.0f / ((1.0f / 298.15f) + (logf(r_ntc / PA_NTC_R25_OHMS) / PA_NTC_BETA));
    t_c = t_k - 273.15f;

    if (t_c < -40.0f) {
        t_c = -40.0f;
    }
    if (t_c > 150.0f) {
        t_c = 150.0f;
    }

    E_pa_temp = (int32_t)(t_c + (t_c >= 0.0f ? 0.5f : -0.5f));
}
