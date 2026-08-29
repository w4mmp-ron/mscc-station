/**
 * Black Pill board stubs — replace with real HAL GPIO/SysTick (Phase 0).
 * Pin names: board_pins.h / docs/STEW-DAUGHTER-BOARD-PINOUT.md
 */
#include "board.h"
#include "board_pins.h"

void board_init(void)
{
    /*
     * TODO Phase 0 (Cube/HAL):
     *  - HAL_Init, SystemClock_Config → 96 MHz, USB 48 MHz ready
     *  - GPIO: BOARD_LED_PIN (PC13) output
     *  - Optional: USART2 PA2/PA3 log
     */
    (void)BOARD_LED_PIN;
}

void board_led_toggle(void)
{
    /* TODO: HAL_GPIO_TogglePin(BOARD_LED_PORT, 1u << BOARD_LED_PIN); */
}

void board_delay_ms(uint32_t ms)
{
    /* TODO: HAL_Delay(ms); */
    (void)ms;
}
