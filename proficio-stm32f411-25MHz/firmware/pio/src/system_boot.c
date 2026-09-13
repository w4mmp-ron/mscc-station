/**
 * STM32F411 system ROM bootloader entry.
 *
 * 0xFE  → write magic (RTC backup, domain correctly enabled) + SystemReset
 *         → at next boot, jump BEFORE board_init (cold-like; ROM brings up HSE)
 * PA8   → minimal GPIO read BEFORE full clock tree, then same jump
 *
 * Do not live-jump with Multus USB up. Do not rely on a half-init RTC BKP write.
 */
#include "system_boot.h"
#include "board_pins.h"
#include "stm32f4xx_hal.h"

#define SYSMEM_BASE     0x1FFF0000u
#define DFU_BOOT_MAGIC  0xB00710ADu

static volatile uint8_t s_enter_pending;

/* -------------------------------------------------------------------------- */
/* RTC backup domain — must select RTC clock or BKP writes are ignored       */
/* -------------------------------------------------------------------------- */

static void backup_domain_ready(void)
{
    /* PWR clock + disable backup write protection */
    RCC->APB1ENR |= RCC_APB1ENR_PWREN;
    (void)RCC->APB1ENR;
    PWR->CR |= PWR_CR_DBP;

    /* If RTC clock not selected yet, use LSI (no crystal dependency). */
    if ((RCC->BDCR & RCC_BDCR_RTCSEL) == 0u) {
        RCC->CSR |= RCC_CSR_LSION;
        while ((RCC->CSR & RCC_CSR_LSIRDY) == 0u) {
        }
        /* RTCSEL = 10: LSI */
        RCC->BDCR = (RCC->BDCR & ~RCC_BDCR_RTCSEL) | RCC_BDCR_RTCSEL_1;
    }
    RCC->BDCR |= RCC_BDCR_RTCEN;
    (void)RCC->BDCR;
}

static void dfu_magic_set(void)
{
    backup_domain_ready();
    RTC->BKP0R = DFU_BOOT_MAGIC;
    (void)RTC->BKP0R; /* readback barrier */
}

static uint8_t dfu_magic_take(void)
{
    backup_domain_ready();
    if (RTC->BKP0R != DFU_BOOT_MAGIC) {
        return 0;
    }
    RTC->BKP0R = 0;
    return 1;
}

/* -------------------------------------------------------------------------- */

void system_boot_jump(void)
{
    typedef void (*pFunction)(void);
    uint32_t msp;
    uint32_t rv;
    pFunction jump;

    /* Latch system-memory vectors before any remap (absolute address). */
    msp = *(__IO uint32_t *)(SYSMEM_BASE + 0u);
    rv  = *(__IO uint32_t *)(SYSMEM_BASE + 4u);
    jump = (pFunction)rv;

    __disable_irq();

    SysTick->CTRL = 0;
    SysTick->LOAD = 0;
    SysTick->VAL = 0;

    for (int i = 0; i < 8; i++) {
        NVIC->ICER[i] = 0xFFFFFFFFu;
        NVIC->ICPR[i] = 0xFFFFFFFFu;
    }

    /*
     * Reset USB OTG FS block so ROM DFU owns a clean peripheral.
     * (Safe even if USB was never started — still at reset defaults.)
     */
    RCC->AHB2ENR |= RCC_AHB2ENR_OTGFSEN;
    (void)RCC->AHB2ENR;
    RCC->AHB2RSTR |= RCC_AHB2RSTR_OTGFSRST;
    RCC->AHB2RSTR &= ~RCC_AHB2RSTR_OTGFSRST;
    RCC->AHB2ENR &= ~RCC_AHB2ENR_OTGFSEN;

    /* HSE on — ROM USB DFU expects an external crystal (25 MHz on this board). */
    RCC->CR |= RCC_CR_HSEON;
    {
        uint32_t t = 0;
        while (((RCC->CR & RCC_CR_HSERDY) == 0u) && (t++ < 1000000u)) {
        }
    }

    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
    (void)RCC->APB2ENR;
    __HAL_SYSCFG_REMAPMEMORY_SYSTEMFLASH();

    __DSB();
    __ISB();
    __set_MSP(msp);
    jump();

    for (;;) {
    }
}

void system_boot_request_reset(void)
{
    s_enter_pending = 1;
}

uint8_t system_boot_pending(void)
{
    return s_enter_pending;
}

void system_boot_reset_into_dfu(void)
{
    dfu_magic_set();
    /* Confirm magic stuck — if not, spinning is better than reboot-to-app loop */
    backup_domain_ready();
    if (RTC->BKP0R != DFU_BOOT_MAGIC) {
        for (;;) {
            /* magic write failed — do not SystemReset into a hopeless loop */
        }
    }
    __DSB();
    __ISB();
    NVIC_SystemReset();
    for (;;) {
    }
}

/**
 * Call at the very start of main(), BEFORE board_init().
 * Returns 1 if a jump was attempted (does not return on success).
 */
uint8_t system_boot_check_and_enter(void)
{
    /* 1) Armed by USB 0xFE + SystemReset */
    if (dfu_magic_take()) {
        system_boot_jump();
    }

    /*
     * 2) PA8 BOOT low — minimal GPIO only (no SystemClock_Config yet).
     * Same cold-like jump as magic path.
     */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    (void)RCC->AHB1ENR;
    /* PA8 input, pull-up */
    GPIOA->MODER &= ~(3u << (8 * 2));
    GPIOA->PUPDR &= ~(3u << (8 * 2));
    GPIOA->PUPDR |= (1u << (8 * 2)); /* pull-up */

    /* Brief settle */
    for (volatile int i = 0; i < 2000; i++) {
    }

    if ((GPIOA->IDR & (1u << 8)) == 0u) {
        /*
         * PA8 seen low — diagnostic: solid blue LED ~2s, then jump.
         * Solid LED = pin detected. Fast blink after reset = pin NOT seen.
         * PC13 is active-low on WeAct.
         */
        RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;
        (void)RCC->AHB1ENR;
        GPIOC->MODER &= ~(3u << (13 * 2));
        GPIOC->MODER |= (1u << (13 * 2));  /* output */
        GPIOC->BSRR = (1u << (13 + 16));   /* reset PC13 → LED ON */

        /* Busy-wait ~2s on HSI (~16 MHz): solid = PA8 detected */
        for (volatile uint32_t d = 0; d < 6000000u; d++) {
        }

        /* LED off = about to enter ROM; if DFU works, stays out of app blink */
        GPIOC->BSRR = (1u << 13); /* set PC13 → LED OFF */

        system_boot_jump();
    }
    return 0;
}
