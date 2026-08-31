#include "board.h"
#include "control.h"
#include "i2c_bus.h"
#include "stm32f4xx_hal.h"

static void SystemClock_Config(void);
static void Error_Handler(void);

void board_init(void)
{
    HAL_Init();
    SystemClock_Config();
    control_init();
    i2c_bus_init();
}

void board_led_on(void)
{
    HAL_GPIO_WritePin(BOARD_LED_GPIO, BOARD_LED_PIN, GPIO_PIN_RESET);
}

void board_led_off(void)
{
    HAL_GPIO_WritePin(BOARD_LED_GPIO, BOARD_LED_PIN, GPIO_PIN_SET);
}

void board_led_toggle(void)
{
    HAL_GPIO_TogglePin(BOARD_LED_GPIO, BOARD_LED_PIN);
}

void board_delay_ms(uint32_t ms)
{
    HAL_Delay(ms);
}

uint32_t board_millis(void)
{
    return HAL_GetTick();
}

/**
 * Black Pill clock — same recipe as TinyUSB stm32f411blackpill BSP (known-good USB).
 * HSE 25 MHz: PLLM=25, N=336, P=/4 → 84 MHz SYSCLK; Q=7 → 48 MHz USB.
 * (HSE_VALUE must match crystal; we define 25000000 in platformio.ini.)
 */
static void SystemClock_Config(void)
{
    RCC_OscInitTypeDef osc = {0};
    RCC_ClkInitTypeDef clk = {0};

    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

    osc.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    osc.HSEState = RCC_HSE_ON;
    osc.PLL.PLLState = RCC_PLL_ON;
    osc.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    osc.PLL.PLLM = (uint32_t)(HSE_VALUE / 1000000U);
    osc.PLL.PLLN = 336;
    osc.PLL.PLLP = RCC_PLLP_DIV4;
    osc.PLL.PLLQ = 7;
    if (HAL_RCC_OscConfig(&osc) != HAL_OK) {
        Error_Handler();
    }

    clk.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                  | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    clk.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    clk.AHBCLKDivider = RCC_SYSCLK_DIV1;
    clk.APB1CLKDivider = RCC_HCLK_DIV2;
    clk.APB2CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_2) != HAL_OK) {
        Error_Handler();
    }
}

void SysTick_Handler(void)
{
    HAL_IncTick();
}

void NMI_Handler(void) { Error_Handler(); }
void HardFault_Handler(void) { Error_Handler(); }
void MemManage_Handler(void) { Error_Handler(); }
void BusFault_Handler(void) { Error_Handler(); }
void UsageFault_Handler(void) { Error_Handler(); }
void SVC_Handler(void) {}
void DebugMon_Handler(void) {}
void PendSV_Handler(void) {}

static void Error_Handler(void)
{
    __disable_irq();
    for (;;) {
    }
}
