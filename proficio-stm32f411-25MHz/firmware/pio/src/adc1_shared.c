/**
 * Shared ADC1 instance for internal die sensor and external PA NTC.
 */
#include "adc1_shared.h"

static ADC_HandleTypeDef hadc1;
static uint8_t s_ready;

void adc1_shared_init(void)
{
    if (s_ready) {
        return;
    }

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
    s_ready = 1;
}

uint32_t adc1_shared_read(uint32_t channel)
{
    ADC_ChannelConfTypeDef c = {0};

    if (!s_ready) {
        return 0xFFFFFFFFu;
    }

    c.Channel = channel;
    c.Rank = 1;
    c.SamplingTime = ADC_SAMPLETIME_480CYCLES;
    if (HAL_ADC_ConfigChannel(&hadc1, &c) != HAL_OK) {
        return 0xFFFFFFFFu;
    }
    if (HAL_ADC_Start(&hadc1) != HAL_OK) {
        return 0xFFFFFFFFu;
    }
    if (HAL_ADC_PollForConversion(&hadc1, 10) != HAL_OK) {
        return 0xFFFFFFFFu;
    }
    {
        uint32_t raw = HAL_ADC_GetValue(&hadc1);
        HAL_ADC_Stop(&hadc1);
        return raw;
    }
}
