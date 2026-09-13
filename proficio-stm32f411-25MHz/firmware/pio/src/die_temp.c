/**
 * STM32F411 internal die temperature (ADC1 temp sensor).
 * Reports °C in E_transceiver_temp for CMD_GET_TRANSCEIVER_TEMP (0xBF).
 */
#include "die_temp.h"
#include "adc1_shared.h"
#include "radio_state.h"
#include "stm32f4xx_hal.h"

void die_temp_init(void)
{
    adc1_shared_init();
}

void die_temp_poll(void)
{
    uint32_t raw;
    int32_t vsense_mv;
    int32_t t_c;

    raw = adc1_shared_read(ADC_CHANNEL_TEMPSENSOR);
    if (raw > 4095u) {
        return;
    }

    /* VSENSE = raw * 3300 / 4095 (mV). Typical F4: ~25°C @ 760 mV, slope ~2.5 mV/°C */
    vsense_mv = (int32_t)((raw * 3300u) / 4095u);
    t_c = 25 + ((vsense_mv - 760) * 10) / 25; /* °C */

    E_transceiver_temp = t_c;
}
