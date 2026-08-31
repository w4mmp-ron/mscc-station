/**
 * STM32F411 system ROM bootloader entry.
 *
 * Off-the-shelf path (no app help):
 *   Black Pill BOOT0 high + reset → ROM bootloader → STM32CubeProgrammer (DFU/SWD)
 *
 * Firmware also supports PSoC-like flows:
 *   - Mother-board BOOT pin (PA8) held low at power-up → enter ROM bootloader
 *   - USB vendor 0xFE → app jumps to ROM bootloader (CubeProgrammer DFU)
 */
#include "system_boot.h"
#include "board_pins.h"
#include "stm32f4xx_hal.h"

#define SYSMEM_BASE  0x1FFF0000u

static volatile uint8_t s_enter_pending;

void system_boot_jump(void)
{
    typedef void (*pFunction)(void);
    uint32_t jump_addr;
    pFunction jump;

    __disable_irq();

    /* Stop USB if running */
    /* (HAL_PCD_Stop optional; full deinit is safer) */
    HAL_RCC_DeInit();
    HAL_DeInit();

    SysTick->CTRL = 0;
    SysTick->LOAD = 0;
    SysTick->VAL = 0;

    for (int i = 0; i < 8; i++) {
        NVIC->ICER[i] = 0xFFFFFFFFu;
        NVIC->ICPR[i] = 0xFFFFFFFFu;
    }

    __HAL_SYSCFG_REMAPMEMORY_SYSTEMFLASH();

    jump_addr = *(__IO uint32_t *)(SYSMEM_BASE + 4u);
    jump = (pFunction)jump_addr;
    __set_MSP(*(__IO uint32_t *)SYSMEM_BASE);
    jump();

    for (;;) {
    }
}

void system_boot_request_reset(void)
{
    /* Deferred jump so USB EP0 status can finish */
    s_enter_pending = 1;
}

uint8_t system_boot_pending(void)
{
    return s_enter_pending;
}

uint8_t system_boot_check_and_enter(void)
{
    /*
     * Mother-board BOOT (PA8), PSoC-style: jumper/low → bootloader.
     * Pull-up in control_init; open = run app.
     */
    if (HAL_GPIO_ReadPin(BOARD_BOOT_GPIO, BOARD_BOOT_PIN) == GPIO_PIN_RESET) {
        system_boot_jump();
    }
    return 0;
}
