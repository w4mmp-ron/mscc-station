/**
 * STM32F411 internal die temperature (ADC1 temp sensor).
 * Reports °C in E_transceiver_temp for CMD_GET_TRANSCEIVER_TEMP (0xBF).
 */
#include "die_temp.h"
#include "radio_state.h"
#include "stm32f4xx_hal.h"

static ADC_HandleTypeDef hadc1;

void die_temp_init(void)
{
    ADC_ChannelConfTypeDef c = {0};

    __HAL_RCC_ADC1_CLK_ENABLE();

    hadc1.Instance = ADC1;
    hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
    hadc1.Init.Resolution = ADC_RESOLUTION_12B;
    hadc1.Init.ScanConvMode = DISABLE;
    hadc1.Init.ContinuousConvMode = DISABLE;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
    hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc1.Init.NbrOfConversion = 1;
    hadc1.Init.DMAContinuousRequests = DISABLE;
    hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
    if (HAL_ADC_Init(&hadc1) != HAL_OK) {
        return;
    }

    c.Channel = ADC_CHANNEL_TEMPSENSOR;
    c.Rank = 1;
    c.SamplingTime = ADC_SAMPLETIME_480CYCLES;
    (void)HAL_ADC_ConfigChannel(&hadc1, &c);
}

void die_temp_poll(void)
{
    uint32_t raw;
    int32_t vsense_mv;
    int32_t t_c;

    if (HAL_ADC_Start(&hadc1) != HAL_OK) {
        return;
    }
    if (HAL_ADC_PollForConversion(&hadc1, 10) != HAL_OK) {
        return;
    }
    raw = HAL_ADC_GetValue(&hadc1);
    HAL_ADC_Stop(&hadc1);

    /* VSENSE = raw * 3300 / 4095 (mV). Typical F4: ~25°C @ 760 mV, slope ~2.5 mV/°C */
    vsense_mv = (int32_t)((raw * 3300u) / 4095u);
    t_c = 25 + ((vsense_mv - 760) * 10) / 25; /* °C */

    E_transceiver_temp = t_c;
}
